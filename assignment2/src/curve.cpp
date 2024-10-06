// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"
#include "im3d.h"

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>
#include <fmt/core.h>
#include <iostream>

using namespace std;        // enables writing "string" instead of std::string, etc.
using namespace Eigen;      // enables writing "Vector3f" instead of "Eigen::Vector3f", etc.

#include "Utils.h"
#include "curve.h"

// Cubic Bezier basis matrix
static Matrix4f B_Bezier {
	{ 1.0f, -3.0f,  3.0f, -1.0f },
	{ 0.0f,  3.0f, -6.0f,  3.0f },
	{ 0.0f,  0.0f,  3.0f, -3.0f },
	{ 0.0f,  0.0f,  0.0f,  1.0f }
};

// Cubic B-spline basis matrix
static Matrix4f B_BSpline = 1.0f/6.0f *
	Matrix4f{
		{ 1.0f, -3.0f,  3.0f, -1.0f },
		{ 4.0f,  0.0f, -6.0f,  3.0f },
		{ 1.0f,  3.0f,  3.0f, -3.0f },
		{ 0.0f,  0.0f,  0.0f,  1.0f }
	};


// This is the core routine of the curve evaluation code. Unlike
// tessellateBezier/tessellateBspline, this is only designed to
// work on 4 control points.
// - The spline basis used is determined by the matrix B (see slides).
// - The tessellated points are to be APPENDED to the vector "dest".
// - num_intervals describes the parameter-space distance between
//   successive points: it should be 1/num_intervals.
// - when include_last_point is true, the last point appended to dest
//   should be evaluated at t=1. Otherwise the last point should be
//   from t=1-1/num_intervals. This is to skip generating double points
//   at the places where one cubic segment ends and another begins.
// - For example, when num_intervals==2 and include_last_point==true,
//   dest should be appended with points evaluated at t=0, t=0.5, t=1.
//   OTOH if include_last_point==false, only t=0 and t=0.5 would be appended.
void tessellateCubicSplineSegment(	const Vector3f& p0,
							const Vector3f& p1,
							const Vector3f& p2,
							const Vector3f& p3,
							unsigned num_intervals,
							bool include_last_point,
							const Matrix4f& B,
							vector<CurvePoint>& dest)
{
	// YOUR CODE HERE (R1): build the geometry matrix G,
	// loop the correct number of points,
	// compute points on the spline, append them to dest.
	// Note that the output vector contains CurvePoints
	// (defined in curve.h), not Vector3fs. This is because
	// many recommended extras require computing tangents
	// and possibly other frame vectors along the spline.
	// If you implement those, it's simplest to just add
	// those members to CurvePoint.
	Matrix4f G{
		{ p0[0], p1[0], p2[0], p3[0] },
		{ p0[1], p1[1], p2[1], p3[1] },
		{ p0[2], p1[2], p2[2], p3[2] },
		{ 1.0f, 1.0f, 1.0f, 1.0f }
	};
	// Fill in as shown on lecture slides.

	unsigned pts_to_add = num_intervals + (include_last_point ? 1 : 0);

	for (unsigned i = 0; i < pts_to_add; ++i)
	{	
		float t = float(i) / num_intervals;

		CurvePoint p;
		Vector4f point = G * (B * Vector4f{ 1.0f, t, t * t, t * t * t });
		p.position = Vector3f(point[0], point[1], point[2]);
		// cout << "position: " << p.position;
		dest.push_back(p);
	}

}    

   
// The P argument holds the control points. Steps determines the step
// size per individual curve segment and should be fed directly to
// tessellateCubicSplineSegment(...).
void tessellateBezier(const vector<Vector3f>& P, vector<CurvePoint>& dest, unsigned num_intervals) {
    // Check
    if (P.size() < 4 || P.size() % 3 != 1) {
		fail("evalBezier must be called with 3n+1 control points.");
	}

	// clear the output array.
	dest.clear();

    // YOUR CODE HERE (R1):
	// Piece together cubic Bézier segments from the control point
	// sequence, making use of tessellateCubicSpline(...).
    // The input "num_intervals" controls the number of points to
	// generate on each piece of the spline, as per the instructions
	// for tessellateCubicSplineSegment(...).

	for (unsigned i = 0; i < P.size() - 3 ; i += 3) {
		bool include_last = (i == P.size() - 4);

		tessellateCubicSplineSegment(P[i], P[i+1], P[i+2], P[i+3], num_intervals, include_last, B_Bezier, dest);
	}

	// EXTRA CREDIT NOTE:
    // Also compute the other Vector3fs for each CurvePoint: T, N, B.
    // A matrix [N, B, T] should be unit and orthogonal.
    // Also note that you may assume that all Bezier curves that you
    // receive have G1 continuity. The T, N and B vectors will not
	// have to be defined at points where this does not hold.

    cerr << ">>> tessellateBezier has been called with the following input:" << endl;

	cerr << fmt::format(">>> {} Control points:\n", P.size());
	for (unsigned i = 0; i < P.size(); ++i)
		cerr << fmt::format("    {:.3f}, {:.3f}, {:.3f}\n", P[i](0), P[i](1), P[i](2));

    cerr << ">>> num_intervals: " << num_intervals << endl;
    cerr << ">>> Returning empty curve." << endl;

    // Right now this will just return an empty curve,
	// as dest was cleared in the beginning.
}

// Like above, the P argument holds the control points and num_intervals determines
// the step size for the individual curve segments.
void tessellateBspline(const vector<Vector3f>& P, vector<CurvePoint>& dest, unsigned num_intervals)
{
    // Check
    if (P.size() < 4) {
		fail("tessellateBspline must be called with 4 or more control points.");
    }

	dest.clear();

    // YOUR CODE HERE (R2):

	for (unsigned i = 0; i < P.size() - 3; ++i) {
		tessellateCubicSplineSegment(P[i], P[i + 1], P[i + 2], P[i + 3], num_intervals, true, B_BSpline, dest);
	}

	cerr << ">>> tessellateBspline has been called with the following input:" << endl;

	cerr << fmt::format(">>> {} Control points:\n", P.size());
	for (unsigned i = 0; i < P.size(); ++i)
		cerr << fmt::format("    {:.3f}, {:.3f}, {:.3f}\n", P[i](0), P[i](1), P[i](2));

	cerr << ">>> num_intervals: " << num_intervals << endl;
	cerr << ">>> Returning empty curve." << endl;

	// Right now this will just return an empty curve,
	// as dest was cleared in the beginning.
}

void drawCurve(const vector<CurvePoint>& curve)
{
	Im3d::BeginLineStrip();
	Im3d::SetColor(1.0f, 1.0f, 1.0f);
	for (unsigned i = 0; i < curve.size(); ++i)
		Im3d::Vertex(curve[i].position(0), curve[i].position(1), curve[i].position(2));
	Im3d::End();
}

