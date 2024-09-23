#pragma once

#include <string>
#include <Eigen/Eigen>

using namespace std;
using Eigen::Vector3f, Eigen::Vector4f;

inline Vector4f to_homogeneous(const Vector3f& in) { return Vector4f{ in(0), in(1), in(2), 1.0f }; }
inline Vector3f from_homogeneous(const Vector4f& in) { return in.block(0, 0, 3, 1); }

// Exit with an error message formatted with sprintf
void fail(const string& reason);

// Portable 
string fileOpenDialog(const string& fileTypeName, const string& fileExtensions);
