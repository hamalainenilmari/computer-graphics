#pragma once

#include "hit.h"
#include "ray.h"

//#include "gui/Image.h"
//#include "io/File.h"
//#include "io/ImageTargaIO.h"

#include <cassert>

class Light;

class Material
{
public:
	Material() {}
	Material(const Vector3f& diffuse_color,
			const Vector3f& reflective_color,
			const Vector3f& transparent_color,
			float refraction_index,
			const char* texture_filename ) :
		diffuse_color_(diffuse_color),
		reflective_color_(reflective_color),
		transparent_color_(transparent_color),
		refraction_index_(refraction_index),
		texture_()
	{
		//if (texture_filename)
		//	texture_.reset(FW::importImage(texture_filename));
	}
	virtual ~Material() {}

	virtual Vector3f diffuse_color(const Vector3f& point) const = 0;
	virtual Vector3f reflective_color(const Vector3f& point) const = 0;
	virtual Vector3f transparent_color(const Vector3f& point) const = 0;
	virtual float refraction_index(const Vector3f& point) const = 0;

	// This function evaluates the light reflected at the point determined
	// by hit.t along the ray, in the direction of Ray, when lit from
	// light incident from dirToLight at the specified intensity.
	virtual Vector3f shade(const Ray& ray, const Hit& hit, const Vector3f& dir_to_light, const Vector3f& incident_intensity, bool shade_back ) const = 0;

protected:
	Vector3f diffuse_color_;
	Vector3f reflective_color_;
	Vector3f transparent_color_;
	float refraction_index_;

	//std::unique_ptr<FW::Image> texture_;
	void* texture_;
};

// This class implements the Phong shading model.
class PhongMaterial : public Material
{
public:
	PhongMaterial(const Vector3f& diffuse_color, const Vector3f& specular_color, float exponent, const Vector3f& reflective_color,
			const Vector3f& transparent_color, float refraction_index, const char* texture_TGA_filename) :
		Material(diffuse_color, reflective_color, transparent_color, refraction_index, texture_TGA_filename),
		specular_color_(specular_color), exponent_(exponent)
	{}

	Vector3f	diffuse_color(const Vector3f&) const override { return diffuse_color_; }
	Vector3f	reflective_color(const Vector3f&) const override { return reflective_color_; }
	Vector3f	transparent_color(const Vector3f&) const { return transparent_color_; }
	float		refraction_index(const Vector3f&) const override { return refraction_index_; }
	Vector3f	specular_color() const { return specular_color_; }
	float		exponent() const { return exponent_; }

	// You need to fill in this implementation of this function.
	Vector3f shade(const Ray& ray, const Hit& hit, const Vector3f& dir_to_light, const Vector3f& incident_intensity, bool shade_back) const override;

private:
	Vector3f specular_color_;
	float exponent_;
};


class ProceduralMaterial : public Material
{
public:
	ProceduralMaterial(const Matrix4f& matrix, shared_ptr<Material> m1, shared_ptr<Material> m2) :
		matrix_(matrix), m1_(m1), m2_(m2)
	{ assert(m1 != nullptr && m2 != nullptr); }

	Vector3f	diffuse_color(const Vector3f& point) const override;
	Vector3f	reflective_color(const Vector3f& point) const override;
	Vector3f	transparent_color(const Vector3f& point) const override;
	float		refraction_index(const Vector3f& point) const override;

	Vector3f shade(const Ray& ray, const Hit& hit, const Vector3f& dir_to_light, const Vector3f& lightColor, bool shade_back) const override;
	virtual float interpolation(const Vector3f& point) const = 0;

protected:
	Matrix4f matrix_;
	shared_ptr<Material> m1_;
    shared_ptr<Material> m2_;
};

class Checkerboard : public ProceduralMaterial
{
public:
	Checkerboard(const Matrix4f& matrix, shared_ptr<Material> m1, shared_ptr<Material> m2) :
		ProceduralMaterial(matrix, m1, m2)
	{}
	float interpolation(const Vector3f& point) const override;
};