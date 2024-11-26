#pragma once

//#include "base/Math.h"

#include <cassert>
#include <cstdio>
#include <string>

class Camera;
class Light;
class Material;
class ObjectBase;
class GroupObject;
class SphereObject;
class PlaneObject;
class TriangleObject;
class TransformObject;

#define MAX_PARSER_TOKEN_LENGTH 100

class SceneParser
{
public:
    enum ProjectionType
    {
        Camera_Orthographic,
        Camera_Perspective
    };

    SceneParser(const std::string& filename);
	SceneParser();

    ~SceneParser();

    shared_ptr<Camera> getCamera() const {
        return camera;
	}

	void setCamera(shared_ptr<Camera> new_camera) {
		camera = new_camera;
	}

    Vector3f getBackgroundColor() const {
        return background_color;
    }

    Vector3f getAmbientLight() const {
        return ambient_light;
    }
    
    int getNumLights() const {
        return num_lights;
    }
    
    shared_ptr<Light> getLight( int i ) const {
        assert( i >= 0 && i < num_lights );
        return lights[i];
    }

    int getNumMaterials() const {
        return materials.size();
    }

    shared_ptr<Material> getMaterial( int i ) const
    {
        assert(i >= 0 && i < materials.size());
        return materials[i];
    }

    shared_ptr<GroupObject> getGroup() const {
        return group;
    }

private:
    void parseFile();
    void parseOrthographicCamera();
    void parsePerspectiveCamera();
    void parseBackground();

    void                parseLights();
    shared_ptr<Light>   parseDirectionalLight();
    shared_ptr<Light>   parsePointLight();

    void                    parseMaterials();
    shared_ptr<Material>    parsePhongMaterial();
    shared_ptr<Material>    parseCheckerboard(int count);

    shared_ptr<ObjectBase>          parseObject(char token[MAX_PARSER_TOKEN_LENGTH]);
    shared_ptr<GroupObject>         parseGroup();
    shared_ptr<SphereObject>        parseSphere();
    shared_ptr<PlaneObject>         parsePlane();
    shared_ptr<TriangleObject>      parseTriangle();
    shared_ptr<GroupObject>         parseTriangleMesh();
    shared_ptr<TransformObject>     parseTransform();

    void parseMatrixHelper(Matrix4f& matrix, char token[MAX_PARSER_TOKEN_LENGTH]);

    int getToken(char token[MAX_PARSER_TOKEN_LENGTH]);
    Vector3f readVector3f();
    Vector2f readVector2f();
    float readFloat();
    int readInt();

    FILE* file;
    shared_ptr<Camera> camera;
    Vector3f background_color;
    Vector3f ambient_light;
    int num_lights;
    vector<shared_ptr<Light>> lights;
    vector<shared_ptr<Material>> materials;
    shared_ptr<Material> current_material;
    shared_ptr<GroupObject> group;
};