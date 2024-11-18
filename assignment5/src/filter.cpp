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

#include "filter.h"

Filter::Filter() {}

Filter::~Filter() {}

Filter* Filter::constructFilter(Args::ReconstructionFilterType t, float radius) {
	if(t == Args::Filter_Box) {
		return new BoxFilter(radius);
	} else if (t == Args::Filter_Tent) {
		return new TentFilter(radius);
	} else if (t == Args::Filter_Gaussian) {
		return new GaussianFilter(radius);
	} else {
		::printf( "FATAL: Unknown reconstruction filter type!\n" );
		exit(0);
	}
}

BoxFilter::BoxFilter(float radius) : radius(radius) { }


float BoxFilter::getSupportRadius() const {
	return radius;
}

float BoxFilter::getWeight(const Vector2f& p) const {
	if(fabs(p(0)) <= radius && fabs(p(1)) <= radius)
		return 1;

    return 0;
}

TentFilter::TentFilter(float radius) : radius(radius) { }

float TentFilter::getSupportRadius() const {
	return radius;
}

float TentFilter::getWeight(const Vector2f& p) const {
	// YOUR CODE HERE (EXTRA)
	// Evaluate the origin-centered tent filter here.
	return 0.0f;
}

GaussianFilter::GaussianFilter(float sigma) : sigma(sigma), radius(2.0f*sigma) { }

float GaussianFilter::getSupportRadius() const {
	return radius;
}

float GaussianFilter::getWeight(const Vector2f& p) const {

	// YOUR CODE HERE (EXTRA)
	// Evaluate the origin-centered Gaussian filter here.
	return 0.0f;
}

