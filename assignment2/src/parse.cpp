// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"
// This is a precaution we take against accidental misuses of the Eigen library,
// as documented here: https://eigen.tuxfamily.org/dox-devel/group__TopicPassingByValue.html 
// Disabling vectorization this way *should* make careless code work,
// but it's better really not to ever pass Eigen objects by value.
#define EIGEN_MAX_STATIC_ALIGN_BYTES 0
#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

#include <map>
#include <fstream>

using namespace std;        // enables writing "string" instead of std::string, etc.
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.
using namespace std;

#include "parse.h"

namespace {

    // read in dim-dimensional control points into a vector
    vector<Vector3f> readCps(istream &in, unsigned dim)
    {    
        // number of control points    
        unsigned n;
        in >> n;

        cerr << "  " << n << " cps" << endl;
    
        // vector of control points
        vector<Vector3f> cps(n);

        char delim;
        float x;
        float y;
        float z;

        for( unsigned i = 0; i < n; ++i )
        {
            switch (dim)
            {
            case 2:
                in >> delim;
                in >> x;
                in >> y;
                cps[i] = Vector3f( x, y, 0 );
                in >> delim;
                break;
            case 3:
                in >> delim;
                in >> x;
                in >> y;
                in >> z;
                cps[i] = Vector3f( x, y, z );
                in >> delim;
                break;            
            default:
                abort();
            }
        }

        return cps;
    }

	Vector4f slerp(float t, Vector4f a, Vector4f b)
	{
		float omega = acosf(clamp(abs(a.dot(b)), -1.0f, 1.0f));
		if (a.dot(b) < 0)
			b = -b;
		if (omega == 0)
			return a;
		return
			sinf((1 - t) * omega) / sinf(omega) * a +
			sinf(t * omega) / sinf(omega) * b;
	}

	Vector4f quatInverse(Vector4f q) {
		q(0) = -q(0);
		q(1) = -q(1);
		q(2) = -q(2);
		return q;
	}

	Vector4f quatMult(Vector4f a, Vector4f b) {
        return Vector4f{ a(3) * b(0) + a(0) * b(3) + a(1) * b(2) - a(2) * b(1), a(3) * b(1) - a(0) * b(2) + a(1) * b(3) + a(2) * b(0), a(3) * b(2) + a(0) * b(1) - a(1) * b(0) + a(2) * b(3), a(3) * b(3) - a(0) * b(0) - a(1) * b(1) - a(2) * b(2) };
	}

	// read in dim-dimensional control points into a vector
	vector<std::array<Vector4f,4>> readQuaternions(istream &in)
	{
		// number of control points    
		unsigned n;
		in >> n;

		cerr << "  " << n << " cps" << endl;

		// vector of control points
		vector<Vector4f> cps(n);

		char delim;

		for (unsigned i = 0; i < n; ++i)
		{
			in >> delim;
			in >> cps[i](0) >> cps[i](1) >> cps[i](2) >> cps[i](3);
			cps[i].normalize();
			in >> delim;
		}
		vector<std::array<Vector4f, 4>> result(n);

		auto clampIdx = [&](unsigned index)->unsigned { while (index < 0)index += n; while (index >= n)index -= n; return index; };

		for (unsigned start = 0; start < n; ++start) {
			result[start][0] = cps[start];
			result[start][1] = quatMult(slerp(.2, Vector4f(.0, .0, .0, 1.), quatMult(cps[clampIdx(start + 1)], quatInverse(cps[clampIdx(start - 1)]))), cps[start]);
			result[start][2] = quatMult(slerp(.2, Vector4f(.0, .0, .0, 1.), quatMult(cps[clampIdx(start)], quatInverse(cps[clampIdx(start + 2)]))), cps[clampIdx(start + 1)]);
			result[start][3] = cps[clampIdx(start + 1)];
		}
		return result;
	}
}



bool parseSWP(const string& filename, vector<SplineCurve>& splines)
{
    ifstream in(filename);

    splines.clear();
    
    string objType;
	vector<array<Vector4f,4>> quaternions;

    // For looking up curve indices by name
    map<string,unsigned> curveIndex;

    // For looking up surface indices by name
    map<string,unsigned> surfaceIndex;
        
    unsigned counter = 0;
    
    while (in >> objType) 
    {
        cerr << ">object " << counter++ << endl;
        string objName;
        in >> objName;

        bool named = (objName != ".");        
       
        if (curveIndex.find(objName) != curveIndex.end() ||
            surfaceIndex.find(objName) != surfaceIndex.end())
        {
            cerr << "error, [" << objName << "] already exists" << endl;
            return false;
        }

        unsigned steps;

        if (objType == "bez2")
        {
            in >> steps;
            cerr << " reading bez2 " << "[" << objName << "]" << endl;
            splines.push_back(SplineCurve{ CurveType_Bezier, readCps(in, 2) });
            if (named) curveIndex[objName] = splines.size()-1;
        }
        else if (objType == "bsp2")
        {
            cerr << " reading bsp2 " << "[" << objName << "]" << endl;
            in >> steps;
            splines.push_back(SplineCurve{ CurveType_BSpline, readCps(in, 2) });
            if (named) curveIndex[objName] = splines.size()-1;
        }
        else if (objType == "bez3")
        {
            cerr << " reading bez3 " << "[" << objName << "]" << endl;
            in >> steps;
            splines.push_back(SplineCurve{ CurveType_Bezier, readCps(in, 3) });
            if (named) curveIndex[objName] = splines.size()-1;
        }
        else if (objType == "bsp3")
        {
            cerr << " reading bsp3 " << "[" << objName << "]" << endl;
            in >> steps;
            splines.push_back(SplineCurve{ CurveType_BSpline, readCps(in, 3) });
            if (named) curveIndex[objName] = splines.size()-1;
        }
		else if (objType == "orientation")
		{
			cerr << " reading camera path orientations" << endl;
			quaternions = readQuaternions(in);
		}
		else if (objType == "camPath")
		{
			//string name;
			//in >> name;
			//cerr << " reading camera path for obj " << "[" << name << "]" << endl;
			//string filename = filepath + name;
			////auto mesh = importMesh(filename);
			////camPath.mesh.reset((Mesh<VertexPNTC>*)mesh);
			//camPath.orientationPoints = quaternions;
			//camPath.positionPath = curves[curveIndex["pos"]];
			//curves[curveIndex["pos"]] = camPath.positionPath;
			//camPath.loaded = true;
		}
        else if (objType == "srev")
        {
            cerr << " reading srev " << "[" << objName << "]" << endl;
            in >> steps;

            // Name of the profile curve
            string profName;
            in >> profName;

            cerr << "  profile [" << profName << "]" << endl;
            
            auto it = curveIndex.find(profName);

            // Failure checks
            if (it == curveIndex.end()) {                
                cerr << "failed: [" << profName << "] doesn't exist!" << endl; return false;
            }
            //if (dims[it->second] != 2) {
            //    cerr << "failed: [" << profName << "] isn't 2d!" << endl; return false;
            //}

            // Make the surface
            //surfaces.push_back( makeSurfRev( curves[it->second], steps ) );
            //surfaceNames.push_back(objName);
            //if (named) surfaceIndex[objName] = surfaceNames.size()-1;
        }
        else if (objType == "gcyl")
        {
            cerr << " reading gcyl " << "[" << objName << "]" << endl;
            
            // Name of the profile curve and sweep curve
            string profName, sweepName;
            in >> profName >> sweepName;

            cerr << "  profile [" << profName << "], sweep [" << sweepName << "]" << endl;

            map<string,unsigned>::const_iterator itP, itS;

            // Failure checks for profile
            itP = curveIndex.find(profName);
            
            if (itP == curveIndex.end()) {                
                cerr << "failed: [" << profName << "] doesn't exist!" << endl; return false;
            }
            //if (dims[itP->second] != 2) {
            //    cerr << "failed: [" << profName << "] isn't 2d!" << endl; return false;
            //}

            // Failure checks for sweep
            itS = curveIndex.find(sweepName);
            if (itS == curveIndex.end()) {                
                cerr << "failed: [" << sweepName << "] doesn't exist!" << endl; return false;
            }

            // Make the surface
            //surfaces.push_back( makeGenCyl( curves[itP->second], curves[itS->second] ) );
            //surfaceNames.push_back(objName);
            //if (named) surfaceIndex[objName] = surfaceNames.size()-1;
        }
        else if (objType == "circ")
        {
            cerr << " reading circ " << "[" << objName << "]" << endl;

            float rad;
            in >> steps >> rad;
            cerr << "  radius [" << rad << "]" << endl;

            //curves.push_back( evalCircle(rad, steps) );
            //curveNames.push_back(objName);
            //dims.push_back(2);
            //if (named) curveIndex[objName] = dims.size()-1;
        }
        else
        {
            cerr << "failed: type " << objType << " unrecognized." << endl;
            return false;
        }
    }

    return true;
}




