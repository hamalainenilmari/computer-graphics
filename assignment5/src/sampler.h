#pragma once

#include "args.h"

#include <cassert>
#include <cmath>
#include <random>

// for supersampling antialiasing

// Base class for sampler. Only supports one sample through the center of the pixel.
class Sampler
{
public:
    Sampler(int num_samples, int random_seed);
	virtual ~Sampler() {};
    virtual Vector2f getSamplePosition(int n) = 0;

    inline Vector2f random_Vector2f()
    {
        float x = distribution_(generator_);
        float y = distribution_(generator_);
        return Vector2f(x, y);
    }

	// call this to get an instance of the proper subclass
    static Sampler* constructSampler(Args::SamplePatternType t, int num_samples, int random_seed);

protected:
    // Random generator not used by deterministic samplers
    mt19937_64                          generator_;     // See https://en.cppreference.com/w/cpp/numeric/random
    uniform_real_distribution<float>    distribution_;
    // num_samples_ can be set to -1 for random samplers that can
    // take an arbitrary number of samples in a pixel
	int			                        num_samples_;
};

// Regular subpixel grid sampler.
// Divide each pixel into sqrt(n)*sqrt(n) subpixels and return samples at their centers.
class RegularSampler : public Sampler
{
public:
	RegularSampler(int nSamples);
	Vector2f getSamplePosition(int n) override;

protected:
	int sqrt_n_;
};

// Regular subpixel grid sampler.
// Divide each pixel into sqrt(n)*sqrt(n) subpixels and return samples at random position inside the strata.
class JitteredSampler : public RegularSampler
{
public:
	JitteredSampler(int num_samples, int random_seed);
	Vector2f getSamplePosition(int n) override;
};

// Uniform random sampling within the pixel. Each sample is chosen at random within the pixel.
class UniformSampler : public Sampler
{
public:
	UniformSampler(int num_samples, int random_seed);
	Vector2f getSamplePosition(int n) override;
};