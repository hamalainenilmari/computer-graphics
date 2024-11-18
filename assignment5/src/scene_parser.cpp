// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

#include "scene_parser.h"

#include "vec_utils.h"
#include "camera.h" 
#include "light.h"
#include "material.h"
#include "object.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>

#define DegreesToRadians(x) ((FW_PI * x) / 180.0f)

using namespace std;

SceneParser::SceneParser( const string& filename )
{
	// initialize some reasonable default values
	background_color = Vector3f(0.5,0.5,0.5);
	ambient_light = Vector3f(0,0,0);
	num_lights = 0;

	// Jaakko: added support for loading from different directories
	//char acDir[_MAX_DIR];
	//char acCWD[_MAX_DIR];
	//_getcwd( acCWD, sizeof(acCWD) );

	//_splitpath( filename.c_str(), 0, acDir, 0, 0);

	// parse the file
	assert(!filename.empty());
	file = fopen(filename.c_str(), "r");
	if ( file == 0 )
	{
		::printf( "FATAL: Could not open %s!\n", filename.c_str() );
		exit(0);
	}

    // change into directory in order to access files relative to it.
    filesystem::path cwd = filesystem::current_path();  // save for later
    filesystem::path file_path = filesystem::path(filename).parent_path();
    filesystem::current_path(file_path);

	parseFile();
	fclose(file); 
	file = 0;

	// .. and change back.
    filesystem::current_path(cwd);

	// if no lights are specified, set ambient light to white
	// (do solid color ray casting)
	if (num_lights == 0)
	{
		::printf( "WARNING: No lights specified.  Setting ambient light to (1,1,1)\n");
		ambient_light = Vector3f(1,1,1);
	}
}

SceneParser::SceneParser()
{
	background_color = Vector3f(0.5, 0.5, 0.5);
	ambient_light = Vector3f(0, 0, 0);
}

SceneParser::~SceneParser()
{
}

// ====================================================================
// ====================================================================

void SceneParser::parseFile() {
	//
	// at the top level, the scene can have a camera, 
	// background color and a group of objects
	// (we add lights and other things in future assignments)
	//
	char token[MAX_PARSER_TOKEN_LENGTH];        
	while (getToken( token )) { 
		if (!strcmp(token, "OrthographicCamera")) {
			parseOrthographicCamera();
		} else if (!strcmp(token, "PerspectiveCamera")) {
			parsePerspectiveCamera();
		} else if (!strcmp(token, "Background")) {
			parseBackground();
		} else if (!strcmp(token, "Lights")) {
			parseLights();
		} else if (!strcmp(token, "Materials")) {
			parseMaterials();
		} else if (!strcmp(token, "Group")) {
			group = parseGroup();
		} else {
			::printf ("Unknown token in parseFile: '%s'\n", token);
			exit(0);
		}
	}
}

void SceneParser::parseOrthographicCamera()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	// read in the camera parameters
	getToken( token ); assert (!strcmp(token, "{"));
	getToken( token ); assert (!strcmp(token, "center"));
	Vector3f center = readVector3f();
	getToken( token ); assert (!strcmp(token, "direction"));
	Vector3f direction = readVector3f();
	getToken( token ); assert (!strcmp(token, "up"));
	Vector3f up = readVector3f();
	getToken( token ); assert (!strcmp(token, "size"));
	float size = readFloat();
	getToken( token ); assert (!strcmp(token, "}"));
	camera = make_shared<OrthographicCamera>(center,direction,up,size);
}


void SceneParser::parsePerspectiveCamera() {
	char token[MAX_PARSER_TOKEN_LENGTH];
	// read in the camera parameters
	getToken( token ); assert (!strcmp(token, "{"));
	getToken( token ); assert (!strcmp(token, "center"));
	Vector3f center = readVector3f();
	getToken( token ); assert (!strcmp(token, "direction"));
	Vector3f direction = readVector3f();
	getToken( token ); assert (!strcmp(token, "up"));
	Vector3f up = readVector3f();
	getToken( token ); assert (!strcmp(token, "angle"));
	float angle_degrees = readFloat();
    float angle_radians = angle_degrees * EIGEN_PI / 180.0f;
	getToken( token ); assert (!strcmp(token, "}"));
	camera = make_shared<PerspectiveCamera>(center,direction,up,angle_radians);
}

void SceneParser::parseBackground() {
	char token[MAX_PARSER_TOKEN_LENGTH];
	// read in the background color
	getToken( token ); assert (!strcmp(token, "{"));    
	while (1) {
		getToken( token ); 
		if (!strcmp(token, "}")) { 
			break;    
		} else if (!strcmp(token, "color")) {
			background_color = readVector3f();
		} else if (!strcmp(token, "ambientLight")) {
			ambient_light = readVector3f();
		} else {
			::printf ("Unknown token in parseBackground: '%s'\n", token);
			assert(0);
		}
	}
}

// ====================================================================
// ====================================================================

void SceneParser::parseLights()
{
    char token[MAX_PARSER_TOKEN_LENGTH];
    getToken(token);
    assert(!strcmp(token, "{"));

	// read in the number of objects
	getToken( token );
    assert(!strcmp(token, "numLights"));
	num_lights = readInt();
    lights.clear();

	// read in the objects
	int count = 0;
	while( count < num_lights )
	{
        getToken(token);
		if( !strcmp( token, "DirectionalLight" ) )
		{
            lights.push_back(parseDirectionalLight());
		}
		else if( !strcmp( token, "PointLight" ) )
		{
			lights.push_back(parsePointLight());
		}
		else
		{
			::printf( "Unknown token in parseLight: '%s'\n", token ); 
            exit(0);
		}     
		count++;
	}
	getToken( token );
	assert( !strcmp( token, "}" ) );
}


shared_ptr<Light> SceneParser::parseDirectionalLight()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken( token ); assert (!strcmp(token, "{"));
	getToken( token ); assert (!strcmp(token, "direction"));
	Vector3f direction = readVector3f();
	getToken( token ); assert (!strcmp(token, "color"));
	Vector3f color = readVector3f();
	getToken( token ); assert (!strcmp(token, "}"));
	return make_shared<DirectionalLight>(direction,color);
}

shared_ptr<Light> SceneParser::parsePointLight()
{
	char token[ MAX_PARSER_TOKEN_LENGTH ];
	getToken( token );
	assert( !strcmp( token, "{" ) );
	getToken( token );
	assert( !strcmp( token, "position" ) );
	Vector3f position = readVector3f();
	getToken( token );
	assert( !strcmp( token, "color" ) );
	Vector3f color = readVector3f();
	Vector3f att( 1, 0, 0 );
	getToken( token ); 
	if( !strcmp( token, "attenuation" ) )
	{
		att[0] = readFloat();
		att[1] = readFloat();
		att[2] = readFloat();
		getToken(token); 
	} 
	assert( !strcmp( token, "}" ) );

	return make_shared<PointLight>( position, color, att[0], att[1], att[2] );
}

void SceneParser::parseMaterials() {
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken( token ); assert (!strcmp(token, "{"));
	// read in the number of objects
	getToken( token ); assert (!strcmp(token, "numMaterials"));
	int num_materials = readInt();
    materials.clear();
	// read in the objects
	int count = 0;
	while (count < num_materials) {
		getToken( token ); 
		if( !strcmp( token, "Material" ) ||
			!strcmp( token, "PhongMaterial" ) )
		{
            materials.push_back(parsePhongMaterial());
		}
		else if( !strcmp( token, "Checkerboard" ) )
		{
            materials.push_back(parseCheckerboard(count));
		}
		else
		{
			::printf ("Unknown token in parseMaterial: '%s'\n", token); 
			exit(0);
		}
		count++;
	}
	getToken( token ); assert (!strcmp(token, "}"));
}    


shared_ptr<Material> SceneParser::parsePhongMaterial()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	Vector3f diffuseColor(1,1,1);
	Vector3f specularColor(0,0,0);
	float exponent = 1;
	Vector3f reflectiveColor(0,0,0);
	Vector3f transparentColor(0,0,0);
	float indexOfRefraction = 1;
	char* texture = nullptr;
	int mipmap = 0;
	int linearInterp = 0;
	getToken(token); assert (!strcmp(token, "{"));
	while (1) {
		getToken(token); 
		if (!strcmp(token, "diffuseColor")) {
			diffuseColor = readVector3f();
		} else if (!strcmp(token, "specularColor")) {
			specularColor = readVector3f();
		} else if  (!strcmp(token, "exponent")) {
			exponent = readFloat();
		} else if (!strcmp(token, "reflectiveColor")) {
			reflectiveColor = readVector3f();
		} else if (!strcmp(token, "transparentColor")) {
			transparentColor = readVector3f();
		} else if (!strcmp(token, "indexOfRefraction")) {
			indexOfRefraction = readFloat();
		} else if (!strcmp(token, "texture")) {
			getToken(token); 
			texture = new char[strlen(token)+2];
			strcpy(texture,token);
		} else if (!strcmp(token, "mipmap")) {
			mipmap = 1;
		} else if (!strcmp(token, "linearInterpolation")) {
			linearInterp = 1;
			::printf ("LINEAR\n");
		} else {
			assert (!strcmp(token, "}"));
			break;
		}
	}
    shared_ptr<Material> answer = make_shared<PhongMaterial>(
        diffuseColor, specularColor, exponent,
        reflectiveColor, transparentColor, indexOfRefraction, texture);

	return answer;
}

shared_ptr<Material> SceneParser::parseCheckerboard(int count)
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert (!strcmp(token, "{"));
	Matrix4f matrix;
	matrix.setIdentity();
	getToken(token); 
	if (!strcmp(token, "Transform")) {
		getToken(token); assert (!strcmp(token, "{"));
		parseMatrixHelper(matrix,token);
		assert (!strcmp(token, "}"));
		getToken(token); 
	}
	assert (!strcmp(token, "materialIndex"));
	int m1 = readInt();
	assert (m1 >= 0 && m1 < count);
	getToken(token); assert (!strcmp(token, "materialIndex"));
	int m2 = readInt();
	assert (m2 >= 0 && m2 < count);
	getToken(token); assert (!strcmp(token, "}"));
    return make_shared<Checkerboard>(matrix, materials[m1], materials[m2]);
}

shared_ptr<ObjectBase> SceneParser::parseObject(char token[MAX_PARSER_TOKEN_LENGTH])
{
	if (!strcmp(token, "Group")) {
		return parseGroup();
	} else if (!strcmp(token, "Sphere")) {
        return parseSphere();
	} else if (!strcmp(token, "Plane")) {
        return parsePlane();
	} else if (!strcmp(token, "Triangle")) {
        return parseTriangle();
	} else if (!strcmp(token, "TriangleMesh")) {
        return parseTriangleMesh();
	} else if (!strcmp(token, "Transform")) {
        return parseTransform();
	} else {
		::printf ("Unknown token in parseObject: '%s'\n", token);
		exit(0);
	}
}

shared_ptr<GroupObject> SceneParser::parseGroup()
{
	//
	// each group starts with an integer that specifies
	// the number of objects in the group
	//
	// the material index sets the material of all objects which follow,
	// until the next material index (scoping for the materials is very
	// simple, and essentially ignores any tree hierarchy)
	//
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken( token ); assert (!strcmp(token, "{"));

	// read in the number of objects
	getToken( token ); assert (!strcmp(token, "numObjects"));
	int num_objects = readInt();

	shared_ptr<GroupObject> answer = make_shared<GroupObject>();

	// read in the objects
	int count = 0;
	while (num_objects > count) {
		getToken( token ); 
		if (!strcmp(token, "MaterialIndex")) {
			// change the current material
			int index = readInt();
			assert (index >= 0 && index <= getNumMaterials());
			current_material = getMaterial(index);
		} else {
			auto object = parseObject(token);
			assert(object);
			answer->insert(object);
			count++;
		}
	}
	getToken( token ); assert (!strcmp(token, "}"));
	
	// return the group
	return answer;
}

// ====================================================================
// ====================================================================

shared_ptr<SphereObject> SceneParser::parseSphere()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken( token ); assert (!strcmp(token, "{"));
	getToken( token ); assert (!strcmp(token, "center"));
	Vector3f center = readVector3f();
	getToken( token ); assert (!strcmp(token, "radius"));
	float radius = readFloat();
	getToken( token ); assert (!strcmp(token, "}"));
	assert (current_material != nullptr);
	return make_shared<SphereObject>(center,radius,current_material);
}


shared_ptr<PlaneObject> SceneParser::parsePlane()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken( token ); assert (!strcmp(token, "{"));
	getToken( token ); assert (!strcmp(token, "normal"));
	Vector3f normal = readVector3f();
	getToken( token ); assert (!strcmp(token, "offset"));
	float offset = readFloat();
	getToken( token ); assert (!strcmp(token, "}"));
	assert (current_material != nullptr);
    return make_shared<PlaneObject>(normal, offset, current_material);
}

#if 0
TriangleObject* SceneParser::parseTriangle() {
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert (!strcmp(token, "{"));
	getToken(token); 
	assert (!strcmp(token, "vertex0"));
	Vector3f v0 = readVector3f();
	getToken(token); 
	assert (!strcmp(token, "vertex1"));
	Vector3f v1 = readVector3f();
	getToken(token); 
	assert (!strcmp(token, "vertex2"));
	Vector3f v2 = readVector3f();
	getToken(token); assert (!strcmp(token, "}"));
	assert (current_material != nullptr);
	return new TriangleObject(v0,v1,v2,current_material);
}
#endif

shared_ptr<TriangleObject> SceneParser::parseTriangle()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	getToken(token); assert (!strcmp(token, "{"));
	Vector2f t0( 0, 0 );
	Vector2f t1( 0, 0 );
	Vector2f t2( 0, 0 );
	getToken(token); 
	if (!strcmp(token, "textCoord0")) { t0 = readVector2f(); getToken(token); }
	assert (!strcmp(token, "vertex0"));
	Vector3f v0 = readVector3f();
	getToken(token); 
	if (!strcmp(token, "textCoord1")) { t1 = readVector2f(); getToken(token); }
	assert (!strcmp(token, "vertex1"));
	Vector3f v1 = readVector3f();
	getToken(token); 
	if (!strcmp(token, "textCoord2")) { t2 = readVector2f(); getToken(token); }
	assert (!strcmp(token, "vertex2"));
	Vector3f v2 = readVector3f();
	getToken(token); assert(!strcmp(token, "}"));
	assert (current_material != nullptr);
    return make_shared<TriangleObject>(v0, v1, v2, current_material);
}

shared_ptr<GroupObject> SceneParser::parseTriangleMesh()
{
	char token[MAX_PARSER_TOKEN_LENGTH];
	char filename[MAX_PARSER_TOKEN_LENGTH];
	// get the filename
	getToken( token ); assert (!strcmp(token, "{"));
	getToken( token ); assert (!strcmp(token, "obj_file"));
	getToken(filename); 
	getToken( token ); assert (!strcmp(token, "}"));
	const char *ext = &filename[strlen(filename)-4];
	assert(!strcmp(ext,".obj"));
	// read it once, get counts
	FILE *mesh_file = fopen(filename,"r");
	assert (mesh_file != nullptr);
	int fcount = 0;
    vector<Vector3f> vertices;
    vector<Vector3i> faces;
	while (1) {
		int c = fgetc(mesh_file);
		if (c == EOF)
        {
            break;
		}
        else if (c == 'v' && fgetc(mesh_file)!='n')
        {
			assert(fcount == 0);
            float v0,v1,v2;
            fscanf(mesh_file, "%f %f %f", &v0, &v1, &v2);
            vertices.push_back(Vector3f(v0, v1, v2));
		}
        else if (c == 'f')
        {
			int f0, f1, f2, t1, t2, t3;
            fscanf(mesh_file, "%d//%d %d//%d %d//%d", &f0, &t1, &f1, &t2, &f2, &t3);
            faces.push_back(Vector3i(f0-1, f1-1, f2-1));
			fcount++; 
		} // otherwise, must be whitespace
	}
	fclose(mesh_file);

	// load the whole model as a single preview model instead of dealing with each triangle separately
    auto trimesh = make_unique<GroupObject>();

    for (size_t i = 0; i < faces.size(); ++i)
    {
        const Vector3f& v0 = vertices[faces[i][0]];
        const Vector3f& v1 = vertices[faces[i][1]];
        const Vector3f& v2 = vertices[faces[i][2]];
        trimesh->insert(make_shared<TriangleObject>(v0, v1, v2, current_material));
    }

    return trimesh;
	
	// read it again, save it
	//mesh_file = fopen(filename,"r");
	//assert (mesh_file != nullptr);
	//int new_vcount = 0; int new_fcount = 0;
	//while (1) {
	//	int c = fgetc(mesh_file);
	//	if (c == EOF) { break;
	//	}else if (c == 'v' && fgetc(mesh_file) != 'n') {
	//		assert(new_fcount == 0); float v0,v1,v2;
	//		fscanf (mesh_file,"%f %f %f",&v0,&v1,&v2);
	//		verts[new_vcount] = Vector3f(v0,v1,v2);
	//		new_vcount++; 
	//	} else if (c == 'f') {
	//		assert (vcount == new_vcount);
	//		int f0,f1,f2, t1, t2, t3;
	//		fscanf (mesh_file,"%d//%d %d//%d %d//%d",&f0,&t1,&f1,&t2,&f2,&t3);
	//		// indexed starting at 1...
	//		assert (f0 > 0 && f0 <= vcount);
	//		assert (f1 > 0 && f1 <= vcount);
	//		assert (f2 > 0 && f2 <= vcount);
	//		assert (current_material != nullptr);
	//		auto t = new TriangleObject(verts[f0-1], verts[f1-1], verts[f2-1], current_material);
	//		answer->insert(t);
	//		new_fcount++; 
	//	} // otherwise, must be whitespace
	//}
	//delete [] verts;
	//assert (fcount == new_fcount);
	//assert (vcount == new_vcount);
	//fclose(mesh_file);
	//return answer;
}

shared_ptr<TransformObject> SceneParser::parseTransform()
{
  char token[MAX_PARSER_TOKEN_LENGTH];
  Matrix4f matrix;
  matrix.setIdentity();
  // opening brace
  getToken(token); assert (!strcmp(token, "{"));
  // the matrix
  parseMatrixHelper(matrix,token);
  // the ObjectBase
  shared_ptr<ObjectBase> object = parseObject(token);
  assert(object != nullptr);
  // closing brace
  getToken(token); assert (!strcmp(token, "}"));
  return make_shared<TransformObject>(matrix, object);
}

void SceneParser::parseMatrixHelper( Matrix4f& matrix, char token[ MAX_PARSER_TOKEN_LENGTH ] )
{
	while( true )
	{
		getToken( token );
		if( !strcmp( token, "Scale" ) )
		{
			Vector3f s = readVector3f();
			//matrix = matrix * Matrix4f::scale( Vector3f( s[0], s[1], s[2] ) );
            matrix = matrix * DiagonalMatrix<float, 4>(Vector4f(s[0], s[1], s[2], 1.0f));
		}
		else if( !strcmp( token, "UniformScale" ) )
		{
			float s = readFloat();
			//matrix = matrix * Matrix4f::scale( Vector3f( s, s, s ) );
            matrix = matrix * DiagonalMatrix<float, 4>(Vector4f(s, s, s, 1.0f));
        }
		else if( !strcmp( token, "Translate" ) )
		{
			//matrix = matrix * Matrix4f::translate( readVector3f() );
            Matrix4f T = Matrix4f::Identity();
            T.block(0, 3, 3, 1) = readVector3f();
            matrix = matrix * T;
		}
		else if( !strcmp( token,"XRotate" ) )
		{
			//matrix = matrix * VecUtils::rotate( Vector3f( 1.0f, 0.0f, 0.0f ), DegreesToRadians( readFloat() ) );
            AngleAxis<float> RotX(readFloat() * EIGEN_PI / 180.0f, Vector3f(1.0f, 0.0f, 0.0f));
            Matrix4f R = Matrix4f::Identity();
            R.block(0, 0, 3, 3) = Matrix3f(RotX);
            matrix = matrix * R;
		}
		else if( !strcmp( token,"YRotate" ) )
		{
			//matrix = matrix * VecUtils::rotate( Vector3f( 0.0f, 1.0f, 0.0f ), DegreesToRadians( readFloat() ) );
            AngleAxis<float> RotY(readFloat() * EIGEN_PI / 180.0f, Vector3f(0.0f, 1.0f, 0.0f));
            Matrix4f R = Matrix4f::Identity();
            R.block(0, 0, 3, 3) = Matrix3f(RotY);
            matrix = matrix * R;
		}
		else if( !strcmp( token,"ZRotate") )
		{
			//matrix = matrix * VecUtils::rotate( Vector3f( 0.0f, 0.0f, 1.0f ), DegreesToRadians( readFloat() ) );
            AngleAxis<float> RotZ(readFloat() * EIGEN_PI / 180.0f, Vector3f(0.0f, 0.0f, 1.0f));
            Matrix4f R = Matrix4f::Identity();
            R.block(0, 0, 3, 3) = Matrix3f(RotZ);
            matrix = matrix * R;
		}
		else if( !strcmp( token,"Rotate" ) )
		{
			getToken( token );
			assert( !strcmp( token, "{" ) );
			Vector3f axis = readVector3f();
			float degrees = readFloat();
            float radians = degrees * EIGEN_PI / 180.0f; // DegreesToRadians(degrees);
            AngleAxis<float> Rot(radians, axis);
            Matrix4f R = Matrix4f::Identity();
            R.block(0, 0, 3, 3) = Matrix3f(Rot);
            matrix = matrix * R;
            //matrix = matrix * VecUtils::rotate(axis,radians);
			getToken( token );
			assert( !strcmp( token, "}" ) );
		}
		else if( !strcmp( token, "Matrix" ) )
		{
            Matrix4f matrix2 = Matrix4f::Identity();
			getToken( token );
			assert( !strcmp( token, "{" ) );

			for (int j = 0; j < 4; j++)
			{
				for (int i = 0; i < 4; i++)
				{
					float v = readFloat();
					matrix2( i, j ) = v; 
				} 
			}

			getToken( token );
			assert (!strcmp(token, "}"));

			matrix = matrix2 * matrix;
		}
		else
		{
			// otherwise this must be the thing to transform
			break;
		}
	}
}

int SceneParser::getToken(char token[MAX_PARSER_TOKEN_LENGTH]) {
	// for simplicity, tokens must be separated by whitespace
	assert (file != nullptr);
	int success = fscanf(file,"%s ",token);
	if (success == EOF) {
		token[0] = '\0';
		return 0;
	}
	return 1;
}


Vector3f SceneParser::readVector3f() {
	float x,y,z;
	int count = fscanf(file,"%f %f %f",&x,&y,&z);
	if (count != 3) {
		::printf ("Error trying to read 3 floats to make a Vector3f\n");
		assert (0);
	}
	return Vector3f(x,y,z);
}


Vector2f SceneParser::readVector2f() {
	float u,v;
	int count = fscanf(file,"%f %f",&u,&v);
	if (count != 2) {
		::printf ("Error trying to read 2 floats to make a Vector2f\n");
		assert (0);
	}
	return Vector2f(u,v);
}


float SceneParser::readFloat() {
	float answer;
	int count = fscanf(file,"%f",&answer);
	if (count != 1) {
		::printf ("Error trying to read 1 float\n");
		assert (0);
	}
	return answer;
}


int SceneParser::readInt() {
	int answer;
	int count = fscanf(file,"%d",&answer);
	if (count != 1) {
		::printf ("Error trying to read 1 int\n");
		assert (0);
	}
	return answer;
}
