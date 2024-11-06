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

#include "particle_system.h"

#define POINT_SIZE 10.0f
#define LINE_WIDTH 2.0f

// ----------------------------------------------------------------------------
// Helpers.

inline Vector2f fGravity2(float mass)
{
    return Vector2f(0, -9.8f * mass);
}

inline Vector3f fGravity3(float mass)
{
    return Vector3f(0, -9.8f * mass, 0);
}

// Force acting on particle at pos1 due to spring attached to pos2 at the other end.
// This complicated template thing is just to be able to use the same code for both Vector2f and Vector3f inputs,
// as well as the Map<...> objects returned by the convenience accessors position() and velocity()
// in the appropriate subclasses of ParticleSystem.
// Use it like Vector2f force = fSpring(position(X, s.i1), position(X, s.i2), k, rl) for a 2D system;
// if the system is 3D, then the return type will be Vector3f.
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
    return -k*v;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Simple system

// initialize the system state to (0, radius)
void SimpleSystem::reset()
{
    current_state_.setZero(2);
    current_state_ << 0, radius_;
}

// the derivative at state (x,y) is (-y, x)
VectorXf SimpleSystem::evalF(const VectorXf& X) const
{
    VectorXf f(2);
    f << -X[1], X[0];
    return f;
}

// draw the state X, as well as lines that mark the path of the actual solution
void SimpleSystem::render(const VectorXf& X) const
{
    Im3d::BeginPoints();
    Im3d::SetSize(POINT_SIZE);
    Im3d::Vertex(X[0], X[1], 0.0f);
    Im3d::End();

    Im3d::BeginLineLoop();
    Im3d::SetSize(LINE_WIDTH);
    static const auto n_lines = 50u;
    for (auto i = 0u; i < n_lines; ++i)
    {
        float x = radius_ * sinf(i * 2.0f * EIGEN_PI / n_lines);
        float y = radius_ * cosf(i * 2.0f * EIGEN_PI / n_lines);
        Im3d::Vertex(x, y, 0.0f);
    }
    Im3d::End();
}

string SimpleSystem::dimension_name(unsigned d) const
{
    static array<string, 2> names = { "x", "y" };
    return names[d];
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Spring system

void SpringSystem::reset()
{
    const auto start_pos = Vector2f(0.1f, -0.5f);
    const auto rest_length = 0.5f;
    current_state_.setZero(2 * 4);	// 2 points that have 4 variables each (2 pos + 2 velocity)
    position(current_state_, 0) = Vector2f::Zero();  // position of point #0
    velocity(current_state_, 0) = Vector2f::Zero();  // velocity of point #0
    // YOUR CODE HERE (R2)
    // Set the initial state for a particle system with one particle fixed
    // at origin and another particle hanging off the first one with a spring.
    // Place the second particle initially at start_pos.
    position(current_state_, 1) = start_pos;
    velocity(current_state_, 1) = Vector2f::Zero();
}

void SpringSystem::imgui_interface()
{
    if (ImGui::TreeNodeEx("System parameters", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::SliderFloat("Spring constant", &k_, 0.0f, 1000.0f))
            spring_.k = k_;
        ImGui::TreePop();
    }
}

VectorXf SpringSystem::evalF(const VectorXf& X) const
{
    const auto drag_k = 0.5f;
    const auto mass = 1.0f;
    VectorXf f(0*X);	// initialize f into 0*X
    position(f, 0) = Vector2f::Zero();
    velocity(f, 0) = Vector2f::Zero();
    // YOUR CODE HERE (R2)
    // Return a derivative for the system as if it was in state "state".
    // You can use the fGravity, fDrag and fSpring helper functions for the forces.
 
    position(f, 1) = velocity(X, 1);
    velocity(f, 1) = ( fGravity2(mass) + fDrag(velocity(X, 1), drag_k) + fSpring(position(X,1), position(X, 0), k_, 0.5f) ) / mass;

    return f;
}

string SpringSystem::dimension_name(unsigned d) const
{
    unsigned idx = d >> 2;
    string r = d & 0x02 ? fmt::format("velocity{}", idx) : fmt::format("position{}", idx);
    r += d & 0x01 ? ".y" : ".x";
    return r;
}

void SpringSystem::render(const VectorXf& state) const
{
    Im3d::BeginPoints();
    Im3d::SetSize(POINT_SIZE);
    Vector2f p0 = position(state, 0);
    Vector2f p1 = position(state, 1);
    Im3d::Vertex(p0(0), p0(1), 0.0f);
    Im3d::Vertex(p1(0), p1(1), 0.0f);
    Im3d::End();

    Im3d::BeginLines();
    Im3d::SetSize(LINE_WIDTH);
    Im3d::Vertex(p0(0), p0(1), 0.0f);
    Im3d::Vertex(p1(0), p1(1), 0.0f);
    Im3d::End();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Multiple pendulum system

void MultiPendulumSystem::reset()
{
    const auto start_point = Vector2f(0.0f, 1.0f);
    Vector2f end_point = start_point + Vector2f(1.5f, 0.1f); // (1.5, 1.1)
    current_state_.setZero(4 * n_);
    // YOUR CODE HERE (R4)
    // Set the initial state for a pendulum system with n_ particles
    // connected with springs into a chain from start_point to end_point with uniform intervals.
    // The rest length of each spring is its length in this initial configuration.

    position(current_state_, 0) = start_point;  // position of point #0
    velocity(current_state_, 0) = Vector2f::Zero();  // velocity of point #0
    const auto intervalX = 1.5 / n_;
    const auto intervalY = 0.1 / n_;

    for (auto i = 1u; i < n_; ++i) {
        position(current_state_, i) = Vector2f(i * intervalX, 1.0 + i * intervalY);  // position of point #i
        velocity(current_state_, i) = Vector2f::Zero();  // velocity of point #i
        auto const spr = Spring(i-1, i, k_, (position(current_state_, i) - position(current_state_, i - 1)).norm()); // TODO check this!
        springs_.push_back(spr);
    };
}

void MultiPendulumSystem::imgui_interface()
{
    if (ImGui::TreeNodeEx("System parameters", ImGuiTreeNodeFlags_Leaf))
    {
        int segments = n_ - 1;
        if (ImGui::SliderInt("Segments", &segments, 1, 20))
        {
            n_ = segments + 1;
            reset();
        }

        if (ImGui::SliderFloat("Spring constant", &k_, 0.0f, 50000.0f))
            for (auto& s : springs_)
                s.k = k_;

        ImGui::SliderFloat("Drag coefficient", &drag_k_, 0.0f, 5.0f);

        ImGui::TreePop();
    }
}

VectorXf MultiPendulumSystem::evalF(const VectorXf& X) const
{
    const auto mass = 0.5f;
    VectorXf dXdt(0 * X);	// initialize with 0*X
    // YOUR CODE HERE (R4)
    // As in R2, return a derivative of the system state specified by the input vector X.
    position(dXdt, 0) = Vector2f::Zero();
    velocity(dXdt, 0) = Vector2f::Zero();

    for (auto i = 1u; i < n_; ++i) {
        position(dXdt, i) = velocity(X, i);
        velocity(dXdt, i) = (fGravity2(mass) + fDrag(velocity(X, i), drag_k_)) / mass;
    }
    
    for (const auto& s : springs_) {
        
        const auto forceSum1 = fSpring(position(X, s.i1), position(X, s.i2), k_, s.rlen);
        const auto forceSum2 = fSpring(position(X, s.i2), position(X, s.i1), k_, s.rlen);

        velocity(dXdt, s.i1) += forceSum1 / mass;
        velocity(dXdt, s.i2) += forceSum2 / mass;
    };
    return dXdt;
}

string MultiPendulumSystem::dimension_name(unsigned d) const
{
    unsigned idx = d >> 2;
    string r = d & 0x02 ? fmt::format("velocity{}", idx) : fmt::format("position{}", idx);
    r += d & 0x01 ? ".y" : ".x";
    return r;
}

void MultiPendulumSystem::render(const VectorXf& X) const
{
    Im3d::BeginPoints();
    Im3d::SetSize(POINT_SIZE);
    for (auto i = 0u; i < n_; ++i)
    {
        Vector2f p = position(X, i);
        Im3d::Vertex(p(0), p(1), 0.0f);
    }
    Im3d::End();
    Im3d::BeginLines();
    Im3d::SetSize(LINE_WIDTH);
    for (const auto& s : springs_)
    {
        Vector2f p1 = position(X, s.i1);
        Vector2f p2 = position(X, s.i2);
        Im3d::Vertex(p1(0), p1(1), 0.0f);
        Im3d::Vertex(p2(0), p2(1), 0.0f);
    }
    Im3d::End();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Cloth system

void ClothSystem::reset()
{
    const auto width = 1.5f, height = 1.5f; // width and height of the whole grid
    current_state_.setZero(6 * x_ * y_); // 3+3 floats per point
    // YOUR CODE HERE (R5)
    // Construct a particle system with a x_ * y_ grid of particles,
    // connected with a variety of springs as described in the handout:
    // structural springs, shear springs and flex springs.
}

void ClothSystem::imgui_interface()
{
    if (ImGui::TreeNodeEx("System parameters", ImGuiTreeNodeFlags_Leaf))
    {
        if (ImGui::SliderFloat("Spring constant", &k_, 0.0f, 50000.0f))
            for (auto& s : springs_)
                s.k = k_;

        ImGui::SliderFloat("Drag coefficient", &drag_k_, 0.0f, 5.0f);

        ImGui::TreePop();
    }
}

VectorXf ClothSystem::evalF(const VectorXf& X) const
{
    const auto n = x_ * y_;
    static const auto mass = 0.025f;
    auto dXdt = VectorXf(0*X);
    // YOUR CODE HERE (R5)
    // This will be much like in R2 and R4.
    return dXdt;
}

string ClothSystem::dimension_name(unsigned d) const
{
    unsigned idx = d / 6;
    unsigned local = d % 6;
    string r = d >= 3 ? fmt::format("velocity{}", idx) : fmt::format("position{}", idx);
    switch (d % 3)
    {
    case 0:
        r += ".x";
        break;
    case 1:
        r += ".y";
        break;
    case 2:
        r += ".z";
        break;
    }
    return r;
}

void ClothSystem::render(const VectorXf& X) const
{
    auto n = x_ * y_;
    Im3d::BeginPoints();
    Im3d::SetSize(POINT_SIZE);
    for (auto i = 0u; i < n; ++i)
    {
        auto p = position(X, i);
        Im3d::Vertex(p(0), p(1), p(2));
    }
    Im3d::End();
    Im3d::BeginLines();
    Im3d::SetSize(LINE_WIDTH);
    for (const auto& s : springs_)
    {
        auto p1 = position(X, s.i1);
        auto p2 = position(X, s.i2);
        Im3d::Vertex(p1(0), p1(1), p1(2));
        Im3d::Vertex(p2(0), p2(1), p2(2));
    }
    Im3d::End();
}
