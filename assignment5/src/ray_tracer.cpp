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

#include "ray_tracer.h"

#include "args.h"
#include "camera.h"
#include "hit.h"
#include "light.h"
#include "material.h"
#include "object.h"
#include "ray.h"
#include "scene_parser.h"

#define EPSILON 0.001f

using namespace std;

namespace {

// Compute the mirror direction for the incoming direction, given the surface normal.
Vector3f mirrorDirection(const Vector3f& normal, const Vector3f& incoming) {
	// YOUR CODE HERE (R8)
	// Pay attention to the direction which things point towards, and that you only
	// pass in normalized vectors.
	return Vector3f::Zero();
}

bool transmittedDirection(const Vector3f& normal, const Vector3f& incoming, 
		float index_i, float index_t, Vector3f& transmitted) {
	// YOUR CODE HERE (EXTRA)
	// Compute the transmitted direction for the incoming direction, given the surface normal
	// and indices of refraction. Pay attention to the direction which things point towards!
	// You should only pass in normalized vectors!
	// The function should return true if the computation was successful, and false
	// if the transmitted direction can't be computed due to total internal reflection.
	return true;
}

} // namespace

Vector3f RayTracer::traceRay(Ray& ray, float tmin, int bounces, float refr_index, Hit& hit, Vector3f debug_color) const
{
	// initialize a hit to infinitely far away
	hit = Hit(FLT_MAX);
	
	// Ask the root node (the single "Group" in the scene) for an intersection.
	bool intersect = false;
	if(scene_.getGroup() != nullptr)
		intersect = scene_.getGroup()->intersect(ray, hit, tmin);

	// Write out the ray segment if visualizing (for debugging purposes)
    if (debug_trace)
        debug_rays.push_back(RaySegment(ray.origin, ray.direction.normalized() * min(100.0f, hit.t), hit.normal, debug_color));

	// if the ray missed, we return the background color.
	if (!intersect)
		return scene_.getBackgroundColor();

	const Material* m = hit.material.get();
	assert(m != nullptr);

	// get the intersection point and normal.
	Vector3f normal = hit.normal;
	Vector3f point = ray.pointAtParameter(hit.t);

	// YOUR CODE HERE (R1)
	// Apply ambient lighting using the ambient light of the scene
	// and the diffuse color of the material.
	
	// kd * ka
	Vector3f answer = Vector3f(
		scene_.getAmbientLight().x() * m->diffuse_color(point).x(),
		scene_.getAmbientLight().y() * m->diffuse_color(point).y(),
		scene_.getAmbientLight().z() * m->diffuse_color(point).z())
		;

	// YOUR CODE HERE (R4 & R7)
	// For R4, loop over all the lights in the scene and add their contributions to the answer.
	// If you wish to include the shadow rays in the debug visualization, insert the segments
	// into the debug_rays list similar to what is done to the primary rays after intersection.
	// For R7, if args_.shadows is on, also shoot a shadow ray from the hit point to the light
	// to confirm it isn't blocked; if it is, ignore the contribution of the light.

	Vector3f difSum = Vector3f::Zero();
	int lights = scene_.getNumLights();
	for (int i = 0; i < lights; ++i) { // for every light in the scene
		// For each light source, ask for the incident illumination with Light::getIncidentIllumination.
		auto light = scene_.getLight(i);
		Vector3f dir;
		Vector3f intensity;
		float dis = 1.0f;
		light -> getIncidentIllumination(point, dir, intensity, dis);
		
		if (args_.shadows) {
			Ray ray2(point, dir);
			Hit hit2;

			float eps = 0.0001;
			bool intersect2 = scene_.getGroup()->intersect(ray2, hit2, eps);

			if (fabs(hit2.t - dis) < eps) { // nothing in between -> add color
				Vector3f d = m->shade(ray, hit, dir, intensity, false);
				difSum += d;
			}
		}
		else {
			Vector3f d = m->shade(ray, hit, dir, intensity, false);
			difSum += d;
		}

	}

	// are there bounces left?
	if (bounces >= 1) {
		// reflection, but only if reflective coefficient > 0!
		if (m->reflective_color(point).norm() > 0.0f) {
			// YOUR CODE HERE (R8)
			// Generate and trace a reflected ray to the ideal mirror direction and add
			// the contribution to the result. Remember to modulate the returned light
			// by the reflective color of the material of the hit point.
		}

		// refraction, but only if surface is transparent!
		if (m->transparent_color(point).norm() > 0.0f) {
			// YOUR CODE HERE (EXTRA)
			// Generate a refracted direction and trace the ray. For this, you need
			// the index of refraction of the object. You should consider a ray going through
			// the object "against the normal" to be entering the material, and a ray going
			// through the other direction as exiting the material to vacuum (refractive index=1).
			// (Assume rays always start in vacuum, and don't worry about multiple intersecting
			// refractive objects!) Remembering this will help you figure out which way you
			// should use the material's refractive index. Remember to modulate the result
			// with the material's refractiveColor().
			// REMEMBER you need to account for the possibility of total internal reflection as well.
		}
	}
	return answer + difSum;
}
