#pragma once

#include <map>

// This class converts a regular mesh into a form suitable for performing subdivision.
// In particular, it computes neighbor information that determines, for each triangle,
// which other triangles are adjacent to it in the mesh.
struct MeshWithConnectivity
{
	//void fromMesh								(const vector<VertexPNC>& vert, const vector<Vector3i>& idx);
	//void toMesh									(vector<VertexPNC>& destVert, vector<Vector3i>& destIdx);

	static MeshWithConnectivity* loadOBJ		(const string& filename);

	// This is where all of the subdivision requirements happen.
	void LoopSubdivision();

	// Supporting functionality.
	void computeConnectivity();
	void computeVertexNormals();

	// Runs a debug version of the subdivision pass
	//vector<Vector3f> debugHighlight(const Vector2f& mousePos, const Matrix4f& worldToClip);

	// vertex data
	vector<Vector3f>	positions;
	vector<Vector3f>	normals;
	vector<Vector3f>	colors;

	// index data
	// (for each triangle, a triplet of indices into the above vertex data arrays)
	vector<Vector3i>	indices;

	// connectivity information (see instructions PDF)
	vector<Vector3i>	neighborTris;
	vector<Vector3i>	neighborEdges;

	// This is for being able to use Vector3fs as keys in an std::map.
	struct CompareVector3f
	{
		bool operator()(const Vector3f& a, const Vector3f& b) const
		{
			return lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
		}
	};

	int pickTriangle(const Vector3f& o, const Vector3f& d) const;


};
