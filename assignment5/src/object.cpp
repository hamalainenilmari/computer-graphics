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

	float t_far = FLT_MAX;
	float t_near = -FLT_MAX;
	for (int i = 0; i < 3; ++i) { //x,y,z
		float t1 = (min_[i] - r.origin[i]) / r.direction[i];
		float t2 = (max_[i] - r.origin[i]) / r.direction[i];

		if (t1 > t2) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}

		if (t_near < t1) t_near = t1;
		if (t_far > t2) t_far = t2;

		if (t_near > t_far) return false; 
	}
	if (t_near < tmin) return false; 

	h.set(t_near, this->material(), h.normal); 
	return true;

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
	//auto D = normal_ * offset_;
	const float t = (offset_ - r.origin.dot(normal_)) / (r.direction.dot(normal_));
	if (h.t > t && t > tmin) {
		h.set(t, this->material(), normal_);
		return true;
	}

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

	Vector4f origin_os = inverse_ * Vector4f(r.origin(0), r.origin(1), r.origin(2), 1.0f);
	Vector4f dir_os = inverse_ * Vector4f(r.direction(0), r.direction(1), r.direction(2), 0.0f);


	Ray ray2(origin_os.head<3>(), dir_os.head<3>());
	
	bool intersection = object_->intersect(ray2, h, tmin);

	Vector4f normal_h_back = inverse_transpose_ * Vector4f(h.normal(0), h.normal(1), h.normal(2), 0.0f);
	if (intersection) h.set(h.t, h.material, normal_h_back.head<3>().normalized());

	return intersection;
	//return false; 
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
	const Vector3f a = vertices_[0];
	const Vector3f b = vertices_[1];
	const Vector3f c = vertices_[2];

	Matrix3f A; 
	A <<
		a.x() - b.x(), a.x() - c.x(), r.direction.x(),
		a.y() - b.y(), a.y() - c.y(), r.direction.y(),
		a.z() - b.z(), a.z() - c.z(), r.direction.z();

	Matrix3f At;
	At <<
		(a.x() - b.x()), (a.x() - c.x()), (a.x() - r.origin.x()),
		(a.y() - b.y()), (a.y() - c.y()), (a.y() - r.origin.y()),
		(a.z() - b.z()), (a.z() - c.z()), (a.z() - r.origin.z());

	Matrix3f Ab;
	Ab <<
		(a.x() - r.origin.x()), (a.x() - c.x()), (r.direction.x()),
		(a.y() - r.origin.y()), (a.y() - c.y()), (r.direction.y()),
		(a.z() - r.origin.z()), (a.z() - c.z()), (r.direction.z());

	Matrix3f Ay;
	Ay <<
		(a.x() - b.x()), (a.x() - r.origin.x()), (r.direction.x()),
		(a.y() - b.y()), (a.y() - r.origin.y()), (r.direction.y()),
		(a.z() - b.z()), (a.z() - r.origin.z()), (r.direction.z());

	const float t = At.determinant() / A.determinant();
	const float baryB = Ab.determinant() / A.determinant();
	const float baryY = Ay.determinant() / A.determinant();

	if (baryB + baryY >= 1 || baryB <= 0 || baryY <= 0) { return false; };

	if (h.t > t && t > tmin) {
		//Vector3f normal = r.pointAtParameter(t);
		Vector3f normal((b - a).cross(c - a));

		normal.normalize();
		h.set(t, this->material(), normal);
		return true;
	}
	return false;
}

const Vector3f& TriangleObject::vertex(int i) const {
	assert(i >= 0 && i < 3);
	return vertices_[i];
}


