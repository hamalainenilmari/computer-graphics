#include "app.h"

#include "subdiv.h"

#include <vector>
#include <map>
#include <iostream>
#include <fstream>

// assumes vertices and indices are already filled in.
void MeshWithConnectivity::computeConnectivity()
{
	// assign default values. boundary edges (no neighbor on other side) are denoted by -1.
	neighborTris.assign(indices.size(), Vector3i{ -1, -1, -1 });
	neighborEdges.assign(indices.size(), Vector3i{ -1, -1, -1 });

	// bookkeeping: map edges (vert0, vert1) to (triangle, edge_number) pairs
	typedef std::map<std::pair<int, int>, std::pair<int, int>> edgemap_t;
	edgemap_t M;

	for (int i = 0; i < (int)indices.size(); ++i) {
		// vertex index is also an index for the corresponding edge starting at that vertex
		for (int j = 0; j < 3; ++j) {
			int v0 = indices[i][j];
			int v1 = indices[i][(j+1)%3];
			auto it = M.find(std::make_pair(v1, v0));
			if (it == M.end()) {
				// edge not found, add myself to mapping
				// (opposite direction than when finding because we look for neighbor edges)
				M[std::make_pair(v0, v1)] = std::make_pair(i, j);
			} else {
				if (it->second.first == -1)	{
					cerr << "Non-manifold edge detected\n";
				} else {
					// other site found, let's fill in the data
					int other_t = it->second.first;
					int other_e = it->second.second;

					neighborTris[i][j] = other_t;
					neighborEdges[i][j] = other_e;

					neighborTris[other_t][other_e] = i;
					neighborEdges[other_t][other_e] = j;

					it->second.first = -1;
				}
			}
		}
	}
	
}


void MeshWithConnectivity::LoopSubdivision() {
	// generate new (odd) vertices

	// visited edge -> vertex position information
	// Note that this is different from the one in computeConnectivity()
	typedef map<std::pair<int, int>, int> edgemap_t;
	edgemap_t new_vertices;

	// The new data must be doublebuffered or otherwise some of the calculations below would
	// not read the original positions but the newly changed ones, which is slightly wrong.
	vector<Vector3f> new_positions(positions.size());
	vector<Vector3f> new_normals(normals.size());
	vector<Vector3f> new_colors(colors.size());

	for (size_t i = 0; i < indices.size(); ++i)
		for (int j = 0; j < 3; ++j) {
			int v0 = indices[i][j];
			int v1 = indices[i][(j + 1) % 3];

			// Map the edge endpoint indices to new vertex index.
			// We use min and max because the edge direction does not matter when we finally
			// rebuild the new faces (R3); this is how we always get unique indices for the map.
			auto edge = std::make_pair(min(v0, v1), max(v0, v1));

			// With naive iteration, we would find each edge twice, because each is part of two triangles
			// (if the mesh does not have any holes/empty borders). Thus, we keep track of the already
			// visited edges in the new_vertices map. That requires the small R3 task below in the 'if' block.
			if (new_vertices.find(edge) == new_vertices.end()) {
				// YOUR CODE HERE (R4): compute the position for odd (= new) vertex.
				// You will need to use the neighbor information to find the correct vertices and then combine the four corner vertices with the correct weights.
				// Be sure to see section 3.2 in the handout for an in depth explanation of the neighbor index tables; the scheme is somewhat involved.
				Vector3f pos, col, norm;

				// This default implementation just puts the new vertex at the edge midpoint.
				//pos = 0.5f * (positions[v0] + positions[v1]);
				col = 0.5f * (colors[v0] + colors[v1]);
				norm = 0.5f * (normals[v0] + normals[v1]);
				int neighbor_left = indices[i][(j + 2) % 3];
				/*
				Use the connectivity structure to get the index of the triangle on the other side and the index of the
				edge that corresponds to the current one; then you can just walk around the other triangle two steps like
				you just did in this current triangle.
				*/
				// Vector3i this_triangle = indices[i]; // start vertices of e_0, e_1, e_2 -> is the triangle

				int triangle_right = neighborTris[i][j]; // neighbor triangle of the current edge

				Vector3f weight_right;
				if (triangle_right != -1) {
					// has neighbor -> calculate its weight
					int neighbor_edge = neighborEdges[i][j]; // the same edge of the neighboring triangle
					int neighbor_right = indices[triangle_right][neighbor_edge];
					weight_right = (1.0f / 8.0f) * positions[neighbor_right];
				}
				else {
					// no neighbor, is the boundary -> set weight of that side vertex to 0
					weight_right = { 0.0f,0.0f,0.0f };
				}

				Vector3f weight_left = (1.0f / 8.0f) * positions[neighbor_left];
				
				pos = (3.0f / 8.0f) * (positions[v0] + positions[v1]) +
					weight_left + weight_right;

				new_positions.push_back(pos);
				new_colors.push_back(col);
				new_normals.push_back(norm);

				// YOUR CODE HERE (R3):
				// Map the edge to the correct vertex index.
				// This is just one line! Use new_vertices and the index of the position that was just pushed back to the vector.
				new_vertices[edge] = new_positions.size() - 1;
			}
		}
	// compute positions for even (old) vertices
	vector<bool> vertexComputed(new_positions.size(), false);

	for (int i = 0; i < (int)indices.size(); ++i) { // every triangle
		for (int j = 0; j < 3; ++j) { // every 3 vertices of the triangle
			int v0 = indices[i][j]; // vertice in the triangle -> compute weights of vertices connected to this one

			// don't redo if this one is already done
			if (vertexComputed[v0] )
				continue;

			vertexComputed[v0] = true;

			Vector3f pos(Vector3f::Zero());
			Vector3f col(Vector3f::Zero());
			Vector3f norm(Vector3f::Zero());
			// YOUR CODE HERE (R5): reposition the old vertices

			// positions of neighboring vertices
			vector<Vector3f> neighborPositions;

			int current_triangle = i;
			int current_edge = j;
			// cout << "Current triangle, edge, vertice at beginning: " << current_triangle << " " << current_edge << " " << v0 << endl;

			while (true) {
				// cout << "Current triangle and edge: " << current_triangle << " " << current_edge << endl;

				// find neighboring triangle and edge
				int neighbor_triangle = neighborTris[current_triangle][current_edge];
				int neighbor_edge = neighborEdges[current_triangle][current_edge];

				// cout << "travelling to neighbor triangle and edge: " << neighbor_triangle << " " << neighbor_edge << endl;

				// in case of boundary
				if (neighbor_triangle == -1) {
					// cout << "neighbor triangle is boundary " << current_triangle << endl;
					break;
				}

				// find third vertex of the triangle to add to neighbors
				int third_vertex = indices[neighbor_triangle][(neighbor_edge + 2) % 3]; 
				neighborPositions.push_back(positions[third_vertex]);

				// cout << "third vertice of neighbor, adding to neigbors : " << third_vertex << endl;
				

				// travel to next triangle
				current_triangle = neighbor_triangle;
				current_edge = (neighbor_edge + 1) % 3;
				
				// stop when travelled full cirle
				if (current_triangle == i) {
					// cout << "back to original triangle: " << current_triangle << endl;
					break;
				}
				
			}

			int n = (int)neighborPositions.size();
			Vector3f v0_pos;
			// cout << "size of neighboring vertices : " << n << endl;
			// cout << "--------------------------------" << endl;
			float B;
			if (n > 3) {
				B = 3.0f / (8.0f * n);
				
			} else if (n == 3)  {
				B = 3.0f / (16.0f);
			}
			else {
				B = 0;
			}
			Vector3f pos_sum(0.0f, 0.0f, 0.0f);
			for (Vector3f position : neighborPositions) {
				pos_sum += position;
			}
			v0_pos = (1.0f - n * B) * positions[v0] + B*(pos_sum);

			// This default implementation just passes the data through unchanged.
			// You need to replace these three lines with the loop over the 1-ring
			// around vertex v0, and compute the new position as a weighted average
			// of the other vertices as described in the handout.

			// If you're having a difficult time, you can try debugging your implementation
			// with the debug highlight mode. If you press alt, LoopSubdivision will be called
			// for only the vertex under your mouse cursor, which should help with debugging.
			// You can also push vertex indices into the highLightIndices vector to draw the
			// vertices with a visible color, so you can ensure that the 1-ring generated is correct.
			// The solution exe implements this so you can see an example of what you can do with the
			// highlight mode there.
			pos = positions[v0];
			col = colors[v0];
			norm = normals[v0];

			new_positions[v0] = v0_pos;
			new_colors[v0] = col;
			new_normals[v0] = norm;
		}
	}

	// and then, finally, regenerate topology
	// every triangle turns into four new ones
	std::vector<Vector3i> new_indices;
	new_indices.reserve(indices.size()*4);
	for (size_t i = 0; i < indices.size(); ++i) {
		Vector3i even = indices[i]; // start vertices of e_0, e_1, e_2

		/*
		For each old triangle, you create four new ones.
		The old indices are given in the triplet even.
		Build the similar vector of 3 ints odd using the information from new_vertices. For this, you will need
		to query new_vertices to get the index of each new vertex added to the middle of each edge of the old
		triangle.
		*/

		// YOUR CODE HERE (R3):
		// fill in X and Y (it's the same for both)

		auto edge_a = std::make_pair(min(even[0], even[1]), max(even[0], even[1])); // old edge e0
		auto edge_b = std::make_pair(min(even[1], even[2]), max(even[1], even[2])); // old edge e1
		auto edge_c = std::make_pair(min(even[2], even[0]), max(even[2], even[0])); // old edge e2

		// query new_vertices to get the index of each new vertex added to the middle of each old edge
		int new_vertex_e0 = new_vertices[edge_a];
		int new_vertex_e1 = new_vertices[edge_b];
		int new_vertex_e2 = new_vertices[edge_c];

		// The edges edge_a, edge_b and edge_c now define the vertex indices via new_vertices.
		// (The mapping is done in the loop above.)
		// The indices define the smaller triangle inside the indices defined by "even", in order.
		// Read the vertex indices out of new_vertices to build the small triangle "odd"

		Vector3i odd1 = { even[0], new_vertex_e0, new_vertex_e2 };
		Vector3i odd2 = { new_vertex_e0, even[1], new_vertex_e1 };
		Vector3i odd3 = { new_vertex_e0, new_vertex_e1, new_vertex_e2 };
		Vector3i odd4 = { new_vertex_e2, new_vertex_e1, even[2]};


		// Then, construct the four smaller triangles from the surrounding big triangle  "even"
		// and the inner one, "odd". Push them to "new_indices".


		// NOTE: REMOVE the following line after you're done with the new triangles.
		// This just keeps the mesh intact and serves as an example on how to add new triangles.
		//new_indices.push_back( Vector3i( even[0], even[1], even[2] ) );
		new_indices.push_back(odd1);
		new_indices.push_back(odd2);
		new_indices.push_back(odd3);
		new_indices.push_back(odd4);
	}

	// ADD THESE LINES when R3 is finished. Replace the originals with the repositioned data.
	indices = std::move(new_indices);
	positions = std::move(new_positions);
	normals = std::move(new_normals);
	colors = std::move(new_colors);
}

MeshWithConnectivity* MeshWithConnectivity::loadOBJ(const string& filename)
{
	// Open input file stream for reading.
	ifstream input(filename, ios::in);

	// vertex and index arrays read from OBJ
	vector<Vector3f> positions;
	vector<Vector3i> faces;

	// Read the file line by line.
	string line;
	while (getline(input, line)) {
		for (auto& c : line)
			if (c == '/')
				c = ' ';

		Vector3i  f; // vertex indices
		Vector3f  v;
		string    s;

		// Create a stream from the string to pick out one value at a time.
		istringstream        iss(line);

		// Read the first token from the line into string 's'.
		// It identifies the type of object (vertex or normal or ...)
		iss >> s;

		if (s == "v") { // vertex position
			iss >> v[0] >> v[1] >> v[2];
			positions.push_back(v);
		}
		else if (s == "f") { // face
			iss >> f[0] >> f[1] >> f[2];
			f -= Vector3i{ 1, 1, 1 }; // go to zero-based indices from OBJ's one-based
			faces.push_back(f);
		}
	}

	// deduplicate vertices (lexicographical comparator CompareVector3f defined in app.h)
	// first, insert all positions into a search structure
	typedef map<Vector3f, unsigned, CompareVector3f> postoindex_t;
	postoindex_t vmap;
	for (auto& v : positions)
		if (vmap.find(v) == vmap.end()) // if this position wasn't there..
			vmap[v] = vmap.size();      // put it in and mark down its index (==current size of map)

	// Construct mesh:
	// Insert the unique vertices into their right places in the vertex array,
	// and compute bounding box while we're at it
	MeshWithConnectivity* pMesh = new MeshWithConnectivity();
	pMesh->positions.resize(vmap.size());
	pMesh->colors.resize(vmap.size());
	pMesh->normals.resize(vmap.size());
	Array3f bbmin{ FLT_MAX, FLT_MAX, FLT_MAX };
	Array3f bbmax = -bbmin;
	for (auto& unique_vert : vmap)
	{
		pMesh->positions[unique_vert.second] = unique_vert.first;
		pMesh->colors[unique_vert.second]    = Vector3f{ 0.75f, 0.75f, 0.75f };
		pMesh->normals[unique_vert.second]   = Vector3f::Zero();
		bbmin = bbmin.min(unique_vert.first.array());
		bbmax = bbmax.max(unique_vert.first.array());
	}

	// set up indices: loop over all faces, look up deduplicated vertex indices by using position from original array
	pMesh->indices.resize(faces.size());
	for (size_t i = 0; i < faces.size(); ++i)
	{
		int i0 = vmap[positions[faces[i][0]]];  // the unique index of the 3D position vector referred to by 0th index of faces[i], etc.
		int i1 = vmap[positions[faces[i][1]]];
		int i2 = vmap[positions[faces[i][2]]];
		pMesh->indices[i] = Vector3i{ i0, i1, i2 };
	}

	// center mesh to origin and normalize scale
	// first construct scaling and translation matrix
	float scale = 10.0f / Vector3f(bbmax - bbmin).norm();
	Vector3f ctr = 0.5f * Vector3f(bbmin + bbmax);
	Matrix4f ST{
		{ scale,  0.0f,  0.0f, -ctr(0) * scale },
		{  0.0f, scale,  0.0f, -ctr(1) * scale },
		{  0.0f,  0.0f, scale, -ctr(2) * scale },
		{  0.0f,  0.0f,  0.0f,            1.0f }
	};
	// then apply it to all vertices
	for (auto& v : pMesh->positions)
	{
		Vector4f v4;
		v4 << v, 1.0f;
		v4 = ST * v4;
		v = v4.block(0, 0, 3, 1);
	}
	// put in the vertex normals..
	pMesh->computeVertexNormals();
	pMesh->computeConnectivity();

	return pMesh;
}

int MeshWithConnectivity::pickTriangle(const Vector3f& o, const Vector3f& d) const
{
	float mint = FLT_MAX;
	int retval = -1;
	for (size_t i = 0; i < indices.size(); ++i)
	{
		const Vector3f& p0 = positions[indices[i](0)];
		const Vector3f& p1 = positions[indices[i](1)];
		const Vector3f& p2 = positions[indices[i](2)];
		Matrix3f T;
		T << p0 - p1, p0 - p2, d;
		Vector3f b = p0 - o;
		Vector3f x = T.inverse() * b;

		float b1 = x(0);
		float b2 = x(1);
		float t = x(2);

		if (b1 >= 0.0f &&
			b2 >= 0.0f &&
			b1 + b2 <= 1.0f &&
			t > 0 &&
			t < 1 &&
			t < mint)
		{
			mint = t;
			retval = i;
		}
	}
	return retval;
}
