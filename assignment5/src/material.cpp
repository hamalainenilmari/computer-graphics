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

#include "material.h"

#include "light.h"
#include "vec_utils.h"


Vector3f PhongMaterial::shade(const Ray &ray, const Hit &hit, 
		const Vector3f& dir_to_light, 
		const Vector3f& incident_intensity,
		bool shade_back) const
{
	// YOUR CODE HERE (R4)
	// Ambient light was already dealt with in R1; implement the rest
	// of the Phong reflectance model (diffuse and specular) here.
	// Start with diffuse and add specular when diffuse is working.
	// NOTE: if shade_back flag is set,
	// you should treat the material as two-sided, i.e., flip the
	// normal if the ray hits the surface from behind. Otherwise
	// you should return zero for hits coming from behind the surface.
	// Remember, when computing the specular lobe, you shouldn't add
	// anything if the light is below the local horizon!

	// remember shade back !!!
	Vector3f normal = hit.normal;
	if (shade_back && normal.dot(ray.direction) > 0 ) {
		normal = -normal;
	}

	float dot = dir_to_light.dot(normal);
	float d = (dot > 0) ? dot : 0;

	Vector3f point = ray.pointAtParameter(hit.t);
	Vector3f dif = d * (incident_intensity.cwiseProduct(hit.material->diffuse_color(point))); // diffuse

	// specular
	auto lightNorm = dir_to_light.normalized();

	Vector3f ri = lightNorm - 2 * (lightNorm.dot(normal) * normal);

	float clamp2 = ray.direction.normalized().dot(ri);
	float c2 = (clamp2 > 0) ? clamp2 : 0; // clamp to 0
	auto Si = incident_intensity.cwiseProduct(specular_color_) * pow(c2,exponent_);

	return dif + Si;
}

Vector3f ProceduralMaterial::diffuse_color(const Vector3f& point) const
{
	Vector3f a1 = m1_->diffuse_color(point);
	Vector3f a2 = m2_->diffuse_color(point);
	Vector3f pt = VecUtils::transformPoint(matrix_, point);
	float v = interpolation(pt);
	return a1 * v + a2 * (1 - v);
}

Vector3f ProceduralMaterial::reflective_color(const Vector3f& point) const
{
	Vector3f a1 = m1_->reflective_color(point);
	Vector3f a2 = m2_->reflective_color(point);
	Vector3f pt = VecUtils::transformPoint(matrix_, point);
	float v = interpolation(pt);
	return a1 * v + a2 * (1 - v);
}

Vector3f ProceduralMaterial::transparent_color( const Vector3f& point) const
{
	Vector3f a1 = m1_->transparent_color(point);
	Vector3f a2 = m2_->transparent_color(point);
	Vector3f pt = VecUtils::transformPoint(matrix_, point);
	float v = interpolation(pt);
	return a1 * v + a2 * (1 - v);
}

float ProceduralMaterial::refraction_index(const Vector3f& point) const
{
	float a1 = m1_->refraction_index(point);
	float a2 = m2_->refraction_index(point);
	Vector3f pt = VecUtils::transformPoint(matrix_, point);
	float v = interpolation(pt);
	return a1 * v + a2 * (1 - v);
}

Vector3f ProceduralMaterial::shade(const Ray &ray, const Hit &hit, 
		const Vector3f &dirToLight, 
		const Vector3f &lightColor,
		bool shade_back) const
{
	Vector3f pt = ray.pointAtParameter(hit.t);
	Vector3f a1 = m1_->shade(ray,hit,dirToLight,lightColor, shade_back);
	Vector3f a2 = m2_->shade(ray,hit,dirToLight,lightColor, shade_back);
	Vector3f pt2 = VecUtils::transformPoint(matrix_, pt);
	float v = interpolation(pt2);
	Vector3f answer = a1 * v + a2 * (1 - v);
	return answer;
}

float Checkerboard::interpolation(const Vector3f& point) const
{
	int count = 1;
	count += int(point[0]) % 2;
	count += int(point[1]) % 2;
	count += int(point[2]) % 2;
	if(point[0] < 0) count ++;
	if(point[1] < 0) count ++;
	if(point[2] < 0) count ++;
	return float(count & 0x01);
}

