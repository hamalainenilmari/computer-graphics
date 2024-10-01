#pragma once

#include <vector>

enum CurveType
{
    CurveType_Bezier,
    CurveType_BSpline
};

template<class P>
struct SplineCurveT
{
    CurveType   curve_type;
    vector<P>   control_points;
};

typedef SplineCurveT<Vector3f> SplineCurve;

// The CurvePoint object stores information about a point on a curve
// after it has been tesselated: the vertex (V), the tangent (T), the
// normal (N), and the binormal (B).  It is the responsiblility of
// functions that create these objects to fill in all the data.
struct CurvePoint
{
    Vector3f    position = Vector3f::Zero();   // Position
    //Vector3f    tangent = Vector3f::Zero(); // Tangent  (unit)
    //Vector3f    normal = Vector3f::Zero(); // Normal   (unit)
    //Vector3f    binormal = Vector3f::Zero(); // Binormal (unit)
    //float       t = 0.0f;
};

// This is just a handy shortcut.
//typedef vector<CurvePoint> Curve;

////////////////////////////////////////////////////////////////////////////
// The following two functions take an array of control points (stored
// in P) and generate an STL Vector of CurvePoints.  They should
// return an empty array if the number of control points in C is
// inconsistent with the type of curve.  In both these functions,
// "step" indicates the number of samples PER PIECE.  E.g., a
// 7-control-point Bezier curve will have two pieces (and the 4th
// control point is shared).
////////////////////////////////////////////////////////////////////////////

// Assume number of control points properly specifies a piecewise
// Bezier curve.  I.e., C.size() == 4 + 3*n, n=0,1,...
void tessellateBezier(const vector<Vector3f>& P, vector<CurvePoint>& dest, unsigned num_intervals);

// Bsplines only require that there are at least 4 control points.
void tessellateBspline(const vector<Vector3f>& P, vector<CurvePoint>& dest, unsigned num_intervals);

// Draw the curve. For the extras that require computing coordinate
// frames, we suggest you append a mechanism for optionally drawing
// the frames as well.
void drawCurve(const vector<CurvePoint>& curve);