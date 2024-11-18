#pragma once

#include <memory>
#include <vector>

#include "material.h"
//#include "base/Math.h"
//#include "3d/Mesh.h"

struct Ray;
struct Hit;
class Material;

// This is the base class for the all the kinds of objects in the scene.
// Its subclasses are Groups, Transforms, Triangles, Planes, Spheres, etc.
// The base class defines functions for intersecting rays with the contents.
class ObjectBase
{
public:
	ObjectBase() : material_(nullptr) {}
	ObjectBase(shared_ptr<Material> m) : material_(m) {}
	//Object3D(Material* m, FW::Mesh<FW::VertexPNT>* mesh) : material_(m), preview_mesh(mesh) { set_preview_materials(); }
	virtual ~ObjectBase() { }

	// Intersect ray with the object.
	// You need to fill in the implementations for several types of objects.
	// Hit is the structure that maintains information about the closest hit found
	// so far. Tmin defines the starting point for the ray, which you can use
	// for dealing with those pesky epsilon issues.
	virtual bool intersect(const Ray& r, Hit& h, float tmin) const = 0;

    virtual void preview_render(const Matrix4f& objectToWorld) const = 0;

	shared_ptr<Material> material() const { return material_; }
	void set_material(shared_ptr<Material> m) { material_ = m; }

	//void set_preview_materials() {
	//	if (preview_mesh == nullptr) return;
	//	for (int i = 0; i < preview_mesh->numSubmeshes(); i++) {
	//		auto& mat = preview_mesh->material(i);
	//		mat.diffuse = Vector4f(material_->diffuse_color(Vector3f(.0f)), 1.0f);
	//		mat.specular = material_->reflective_color(Vector3f(.0f));
	//		mat.glossiness = 200.0f;
	//	}
	//}

protected:
	shared_ptr<Material> material_;
};

class BoxObject : public ObjectBase
{
public:
	BoxObject(const Vector3f& min, const Vector3f& max, shared_ptr<Material> m) :
		ObjectBase(m), min_(min), max_(max) {

		//preview_mesh.reset((FW::Mesh<FW::VertexPNT>*)FW::importMesh("preview_assets/cube.obj"));
		//set_preview_materials();
	}
	bool intersect(const Ray& r, Hit& h, float tmin) const override;
	void preview_render(const Matrix4f& objectToWorld) const override;

private:
	Vector3f	min_;
	Vector3f	max_;
};

class GroupObject : public ObjectBase
{
public:
	GroupObject() {}
	GroupObject(shared_ptr<Material> m) : ObjectBase(m) {}

	bool intersect(const Ray& r, Hit& h, float tmin) const override;
	void preview_render(const Matrix4f& objectToWorld) const override;

	size_t size() const { return objects_.size(); }
	shared_ptr<ObjectBase> operator[](int i) const;
	void insert(shared_ptr<ObjectBase> o);
private:
	vector<shared_ptr<ObjectBase>> objects_;
};

class PlaneObject : public ObjectBase
{
public:
	PlaneObject(const Vector3f& normal, float offset, shared_ptr<Material> m) :
		ObjectBase(m), normal_(normal.normalized()), offset_(offset) {
		//preview_mesh.reset((FW::Mesh<FW::VertexPNT>*)FW::importMesh("preview_assets/plane.obj"));
		//set_preview_materials();
	}

	bool intersect(const Ray& r, Hit& h, float tmin) const override;
	void preview_render(const Matrix4f& objectToWorld) const override;

	const Vector3f& normal() const { return normal_; }
	float offset() const { return offset_; }

private:
	Vector3f normal_;
	float offset_;
};

class SphereObject : public ObjectBase
{
public:
	SphereObject(const Vector3f& center, float radius, shared_ptr<Material> m) :
		ObjectBase(m), center_(center), radius_(radius) {
		//preview_mesh.reset((FW::Mesh<FW::VertexPNT>*)FW::importMesh("preview_assets/sphere.obj"));
		//set_preview_materials();
	}

	bool intersect(const Ray& r, Hit& h, float tmin) const override;
	void preview_render(const Matrix4f& objectToWorld) const override;

private:
	Vector3f center_;
	float radius_;
};

// A Transform wraps an object. When intersecting it with
// a ray, you need to transform the ray into the coordinate
// system "inside" the transform as described in the lecture.
class TransformObject : public ObjectBase
{
public:
	TransformObject(const Matrix4f& m, shared_ptr<ObjectBase> o);

	bool intersect(const Ray &r, Hit &h, float tmin) const override;
	void preview_render(const Matrix4f& objectToWorld) const override;

private:
	Matrix4f                matrix_;
	Matrix4f                inverse_;
	Matrix4f                inverse_transpose_;
	shared_ptr<ObjectBase>  object_;
};

class TriangleObject : public ObjectBase
{
public:
	// a triangle contains, in addition to the vertices, 2D texture coordinates,
	// often called "uv coordinates".
	TriangleObject(const Vector3f& a, const Vector3f& b, const Vector3f &c, shared_ptr<Material> m);

	bool intersect(const Ray &r, Hit &h, float tmin) const override;
	void preview_render(const Matrix4f& objectToWorld) const override;

	const Vector3f& vertex(int i) const;

private:
	Vector3f vertices_[3];
};
