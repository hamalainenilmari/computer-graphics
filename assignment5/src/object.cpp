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

#include "object.h"
#include "hit.h"
#include "vec_utils.h"

#include <cassert>

shared_ptr<ObjectBase> GroupObject::operator[](int i) const
{
	assert(i >= 0 && size_t(i) < size());
	return objects_[i];
}

void GroupObject::insert(shared_ptr<ObjectBase> o)
{
	assert(o);
	objects_.emplace_back(o);
}

bool GroupObject::intersect(const Ray& r, Hit& h, float tmin) const {
	// We intersect the ray with each object contained in the group.
	bool intersected = false;
	for (int i = 0; i < int(size()); ++i) {
		ObjectBase* o = objects_[i].get();
		assert(o != nullptr);
		assert(h.t >= tmin);
		bool tmp = o->intersect(r, h, tmin);
		assert(h.t >= tmin);
		if (tmp)
			intersected = true;
	}
	assert(h.t >= tmin);
	return intersected;
}

bool BoxObject::intersect(const Ray& r, Hit& h, float tmin) const {
// YOUR CODE HERE (EXTRA)
// Intersect the box with the ray!

  return false;
}

bool PlaneObject::intersect( const Ray& r, Hit& h, float tmin ) const {
	// YOUR CODE HERE (R5)
	// Intersect the ray with the plane.
	// Pay attention to respecting tmin and h.t!
	// Equation for a plane:
	// ax + by + cz = d;
	// normal . p - d = 0
	// (plug in ray)
	// origin + direction * t = p(t)
	// origin . normal + t * direction . normal = d;
	// t = (d - origin . normal) / (direction . normal);
	return false;
}

TransformObject::TransformObject(const Matrix4f& m, shared_ptr<ObjectBase> o) :
	matrix_(m),
	object_(o)
{
	assert(o != nullptr);
	inverse_ = matrix_.inverse();
	inverse_transpose_ = inverse_.transpose();
}

bool TransformObject::intersect(const Ray& r, Hit& h, float tmin) const {
	// YOUR CODE HERE (EXTRA)
	// Transform the ray to the coordinate system of the object inside,
	// intersect, then transform the normal back. If you don't renormalize
	// the ray direction, you can just keep the t value and do not need to
	// recompute it!
	// Remember how points, directions, and normals are transformed differently!

	return false; 
}

bool SphereObject::intersect( const Ray& r, Hit& h, float tmin ) const {
	// Note that the sphere is not necessarily centered at the origin.
	
	Vector3f tmp = center_ - r.origin;
	Vector3f dir = r.direction;

	float A = dir.dot(dir);
	float B = - 2 * dir.dot(tmp);
    float C = tmp.dot(tmp) - (radius_* radius_);
	float radical = B*B - 4*A*C;
	if (radical < 0)
		return false;

	radical = sqrtf(radical);
	float t_m = ( -B - radical ) / ( 2 * A );
	float t_p = ( -B + radical ) / ( 2 * A );
	Vector3f pt_m = r.pointAtParameter( t_m );
	Vector3f pt_p = r.pointAtParameter( t_p );

	assert(r.direction.norm() > 0.9f);

	bool flag = t_m <= t_p;
	if (!flag) {
		::printf( "sphere ts: %f %f %f\n", tmin, t_m, t_p );
		return false;
	}
	assert( t_m <= t_p );

	// choose the closest hit in front of tmin
	float t = (t_m < tmin) ? t_p : t_m;

	if (h.t > t  && t > tmin) {
		Vector3f normal = r.pointAtParameter(t);
		normal -= center_;
		normal.normalize();
		h.set(t, this->material(), normal);
		return true;
	}
	return false;
} 

TriangleObject::TriangleObject(const Vector3f& a, const Vector3f& b, const Vector3f& c, shared_ptr<Material> m) :
	ObjectBase(m)
{
	vertices_[0] = a;
	vertices_[1] = b;
	vertices_[2] = c;
}

bool TriangleObject::intersect( const Ray& r, Hit& h, float tmin ) const
{
	// YOUR CODE HERE (R6)
	// Intersect the triangle with the ray!
	// Again, pay attention to respecting tmin and h.t!
	return false;
}

const Vector3f& TriangleObject::vertex(int i) const {
	assert(i >= 0 && i < 3);
	return vertices_[i];
}


