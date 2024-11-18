#pragma once

#include "hit.h"
#include "object.h"
#include "ray.h"

struct Args;
class SceneParser;

struct RaySegment {
	RaySegment(Vector3f origin, Vector3f offset, Vector3f normal_at_offset, Vector3f color) : origin(origin), offset(offset), normal_at_offset(normal_at_offset), color(color) {}
	Vector3f origin, offset, normal_at_offset, color;
};

class RayTracer
{
public:
	RayTracer(const SceneParser& scene, const Args& args, bool debug = false) :
		args_(args),
		scene_(scene),
		debug_trace(debug)
	{}

	// You need to fill in the implementation for this function.
    Vector3f traceRay(Ray& ray, float tmin, int bounces, float refr_index, Hit& hit, Vector3f debug_color) const;
	
	// For the debug visualisation: mutable means that we can modify it inside the traceRay method even though it is const.
	mutable std::vector < RaySegment > debug_rays;
private:
	RayTracer& operator=(const RayTracer&); // squelch compiler warning
	Vector3f computeShadowColor(Ray& ray, float distanceToLight) const;

	bool debug_trace;

	const SceneParser&	scene_;
	const Args&			args_;
};
