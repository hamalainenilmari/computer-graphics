
#include <Eigen/Dense>              // Linear algebra
using namespace Eigen;
using namespace std;

#include "particle_system.h"
#include "integrators.h"

// This function uses the specified integrator to advance the system
// from its current state to the next.
void stepSystem(ParticleSystem& ps, integrator_t integrator, float dt)
{
	VectorXf new_state = integrator(ps, dt);
	ps.set_state(new_state);
}


VectorXf eulerStep(const ParticleSystem& ps, float dt)
{
	// YOUR CODE HERE (R1)
	// Implement an Euler integrator.
	// Use ps.state() to access the current state and
	// ps.evalF(...) to compute the derivative dX/dt.
	// Return the new state after the Euler step.
	return ps.state() + (dt * ps.evalF(ps.state()));
	//return ps.state();	// No-op, replace this
};

VectorXf trapezoidStep(const ParticleSystem& ps, float dt)
{
	// YOUR CODE HERE (R3)
	// Implement a trapezoid integrator.
	//return ps.state();	// No-op, replace this
	const auto f0 = ps.evalF(ps.state());
	const auto f1 = ps.evalF(ps.state() + dt * f0);
	return ps.state() + (dt / 2) * (f0 + f1);

}

VectorXf midpointStep(const ParticleSystem& ps, float dt)
{
	const auto& x0 = ps.state();
	auto f0 = ps.evalF(x0);
	VectorXf xm = x0 + 0.5f * dt * f0;
	auto fm = ps.evalF(xm);
	VectorXf x1 = x0 + dt * fm;
	return x1;
}

VectorXf rk4Step(const ParticleSystem& ps, float dt)
{
	// EXTRA: Implement the RK4 Runge-Kutta integrator.
	//return ps.state();	// No-op, replace this
	const auto& x0 = ps.state();
	auto k1 = ps.evalF(x0); // f0
	VectorXf x2 = x0 + dt * 0.5f * k1;
	auto k2 = ps.evalF(x2);
	VectorXf x3 = x0 + dt * 0.5f * k2;
	auto k3 = ps.evalF(x3);
	VectorXf x4 = x0 + dt * k3;
	auto k4 = ps.evalF(x4);

	return (x0 + (dt / 6.0) * (k1 + 2 * k2 + 2 * k3 + k4));
}
