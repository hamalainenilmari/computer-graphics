#pragma once

#include <functional>

typedef std::function<VectorXf(const ParticleSystem&, float)> integrator_t;

void stepSystem(ParticleSystem&, integrator_t integrator, float dt);

// R1
VectorXf eulerStep(const ParticleSystem& ps, float dt);

// R3
VectorXf trapezoidStep(const ParticleSystem& ps, float dt);

// Given as a reference
VectorXf midpointStep(const ParticleSystem& ps, float dt);

// Extra
VectorXf rk4Step(const ParticleSystem& ps, float dt);

