/*
#include <Eigen/Dense>
using namespace Eigen;

#include "im3d.h"
#include "imgui.h"
#include "fmt/core.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>

using namespace std;

#include "sprinkler.h"

#define POINT_SIZE 10.0f

// ----------------------------------------------------------------------------
// Helpers.

inline Vector2f fGravity2(float mass)
{
    return Vector2f(0, -9.8f * mass);
}

template <typename Derived>
typename Eigen::MatrixBase<Derived>::PlainObject fSpring(const Eigen::MatrixBase<Derived>& pos1,
    const Eigen::MatrixBase<Derived>& pos2,
    float k,
    float rest_length)
{
    auto spring = pos2 - pos1;
    auto length = spring.norm();
    return (k * (length - rest_length) / length) * spring;
}

// Similar story here.
template<typename Derived>
inline typename Eigen::MatrixBase<Derived>::PlainObject fDrag(const Eigen::MatrixBase<Derived>& v, float k)
{
    return -k * v;
}
// sprinkler
void SprinklerSystem::reset()
{
    //current_state_.setZero(4 * n_); // n particles
    alive_particles_.clear();

    //cout << "reset sprinkler called " << endl;

    const auto start_point = Vector2f(0, -1.0);
    for (auto i = 0u; i < n_; ++i) {
        //cout << "particle number: " << i << endl;
        //position(current_state_, i) = Vector2f(0.0, 0.0);  // position of point #i
        //velocity(current_state_, i) = Vector2f(0.0 + i * spread_, 5.0);  // velocity of point #i

        auto p = Particle();
        p.setPosition(Vector2f(0.0, 0.0));
        p.setVelocity(Vector2f(0.0 + i * spread_, 5.0));
        alive_particles_.push_back(p);
    }

}

void SprinklerSystem::emit()
{
    const int emitPerFrame = 5;
    int iEmissions = 0;

    Vector2f emittorPos(0.0, 0.0);
    while (iEmissions < emitPerFrame) {
        auto p = Particle();
        p.setPosition(Vector2f(0.0, 0.0));
        p.setVelocity(Vector2f(0.0 + iEmissions * spread_, 5.0));
        alive_particles_.push_back(p);

        //int index = alive_particles_.size() - 1;
        //position(current_state_, index) = p.getPosition();
        //velocity(current_state_, index) = p.getVelocity();

        ++iEmissions;
    }
}

void SprinklerSystem::update()
{
    const int emitPerFrame = 5;
    const int n = alive_particles_.size();
    static const auto mass = 0.025f;

    alive_particles_.erase(
        remove_if(alive_particles_.begin(), alive_particles_.end(),
            [](const Particle& p) { return p.getAge() > 1; }),
        alive_particles_.end());

    int i = 0;
    for (auto& particle : alive_particles_) {
        const int age = particle.getAge();
        particle.setAge(age + 0.1);
        particle.setPosition(particle.getVelocity());
        particle.setVelocity(((fGravity2(mass) + fDrag(particle.getVelocity(), drag_k_)) / mass));

        //position(current_state_, i) = particle.getPosition();
        //velocity(current_state_, i) = particle.getVelocity();
        i++;
    }
}
// the derivative at state (x,y) is (-y, x)
VectorXf SprinklerSystem::evalF(const VectorXf& X) const
{
    static const auto mass = 0.025f;
    auto dXdt = VectorXf(0 * X);

    for (auto i = 0u; i < alive_particles_.size(); ++i) {

        //alive_particles_[i].setPosition(alive_particles_[i].getVelocity());
        position(dXdt, i) = velocity(X, i);
        velocity(dXdt, i) = (fGravity2(mass) + fDrag(velocity(X, i), drag_k_)) / mass;
    }
    return dXdt;
// draw the state X, as well as lines that mark the path of the actual solution
void SprinklerSystem::render() const
{
    Im3d::BeginPoints();
    Im3d::SetSize(POINT_SIZE);
    for (auto i = 0u; i < alive_particles_.size(); ++i)
    {
        //cout << "point: " << i << endl;
        Vector2f p = alive_particles_[i].getPosition();
        Im3d::Vertex(p(0), p(1), 0.0f);
    }
    Im3d::End();
}

string SprinklerSystem::dimension_name(unsigned d) const
{
    static array<string, 2> names = { "x", "y" };
    return names[d];
}
*/