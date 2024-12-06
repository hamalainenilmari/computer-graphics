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

#include "sampler.h"

#include <cassert>

Sampler::Sampler(int num_samples, int random_seed)
{
    num_samples_ = num_samples;
    generator_.seed(random_seed);
}

Sampler* Sampler::constructSampler(Args::SamplePatternType t, int num_samples, int random_seed)
{
	if( t == Args::Pattern_UniformRandom ) {
		return new UniformSampler(num_samples, random_seed);
	} else if ( t == Args::Pattern_Regular ) {
		return new RegularSampler(num_samples);
	} else if ( t == Args::Pattern_JitteredRandom ) {
		return new JitteredSampler(num_samples, random_seed);
	} else {
		assert(false && "Bad sampler type");
		return nullptr;
	}
}

UniformSampler::UniformSampler(int num_samples, int random_seed) :
	Sampler(num_samples, random_seed)
{}

Vector2f UniformSampler::getSamplePosition(int i)
{
    return random_Vector2f();
}

RegularSampler::RegularSampler(int num_samples) :
	Sampler(num_samples, 0)
{
	// test that it's a perfect square
	int dim = (int)sqrtf(float(num_samples_));
	assert(num_samples_ == dim * dim);
    sqrt_n_ = dim;
}

Vector2f RegularSampler::getSamplePosition(int n)
{
	// YOUR CODE HERE (R9)
	// Return a sample through the center of the Nth subpixel.
	// The starter code only supports one sample per pixel.

	int i = n / sqrt_n_;
	int j = n % sqrt_n_;

	float x = (j + 0.5f) / sqrt_n_; 
	float y = (i + 0.5f) / sqrt_n_;

	return Vector2f(x, y);
}

JitteredSampler::JitteredSampler(int num_samples, int random_seed) :
	RegularSampler(num_samples)
{
    generator_.seed(random_seed);   // must do here, as base class doesn't randomize
}

Vector2f JitteredSampler::getSamplePosition(int n)
{
	// YOUR CODE HERE (R9)
	// Return a randomly generated sample through Nth subpixel.
	int i = n / sqrt_n_;
	int j = n % sqrt_n_;

	std::uniform_real_distribution<float> uniform_dist(-0.5f / sqrt_n_, 0.5f / sqrt_n_);
	
	float x = (j + 0.5f) / sqrt_n_ + uniform_dist(generator_);
	float y = (i + 0.5f) / sqrt_n_ + uniform_dist(generator_);

	return Vector2f(x, y);
}

