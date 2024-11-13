#pragma once

// Structure that represents a spring between points i1 and i2
// k is spring constant
// rlen is rest length
struct Spring
{
    Spring() {}
    Spring(unsigned index1, unsigned index2, float spring_k, float rest_length) :
        i1(index1), i2(index2), k(spring_k), rlen(rest_length) {}
    unsigned i1, i2;
    float k, rlen;
};

// Base class for all particle systems
class ParticleSystem
{
public:
    virtual					~ParticleSystem() {};
    virtual VectorXf		evalF(const VectorXf& X) const = 0;

    // Construct the system anew in an initial state. Always overloaded by the subclasses.
    virtual void			reset() = 0;

    // Get/set current state
    const VectorXf&			state() const { return current_state_; }
    const vector<Spring>&   springs() const { return springs_; }
    void					set_state(const VectorXf& s) { current_state_ = s; }

    // Render system as points and lines using Im3d.
    virtual void			render(const VectorXf& X) const = 0;

    // Use ImGui to enable user to change relevant parameters of the system.
    // Does not have to be overloaded; default implementation here does nothing.
    virtual void			imgui_interface() {}

    // This is a helper for debugging and creating visualizations of the phase space.
    // Takes an index into the state vector and returns a human-readable name for it.
    // Has been implemented already for you. See the subclasses for details.
    virtual string			dimension_name(unsigned) const = 0;

protected:
    VectorXf				current_state_;
    vector<Spring>			springs_;
};


// ----------------------------------------------------------------------------
// The first simple 2D system that moves a point along a circle
class SimpleSystem : public ParticleSystem
{
public:
    SimpleSystem()          { reset(); }

    VectorXf				evalF(const VectorXf& X) const override;

    void					reset() override;
    void					render(const VectorXf& X) const override;
    string					dimension_name(unsigned d) const override;

private:
    float					radius_ = 0.5f;
};

// ----------------------------------------------------------------------------
class SpringSystem : public ParticleSystem
{
public:
    SpringSystem()          { reset(); }

    VectorXf				evalF(const VectorXf&) const override;

    // Helper functions to read and write the 2D positions and velocities in state vectors.
    // The Map that is returned acts pretty much like a Vector2f, but its contents are stored in a particular
    // location in the longer state vector. See Eigen's documentation for details.
    static auto				position(VectorXf& X, int idx)				{ return Map<Vector2f>(&X(idx * 4)); }          // write pos
    static auto				position(const VectorXf& X, int idx)		{ return Map<const Vector2f>(&X(idx * 4)); }    // read pos
    static auto				velocity(VectorXf& X, int idx)				{ return Map<Vector2f>(&X(idx * 4 + 2)); }      // write
    static auto				velocity(const VectorXf& X, int idx)		{ return Map<const Vector2f>(&X(idx * 4 + 2)); }// read

    void					reset() override;
    void					render(const VectorXf& X) const override;
    string					dimension_name(unsigned d) const override;

    void					imgui_interface() override;

private:
    Spring					spring_;
    float					k_ = 30.0f;
};

// ----------------------------------------------------------------------------
class MultiPendulumSystem : public ParticleSystem
{
public:
    MultiPendulumSystem(unsigned n)                                 { n_ = n;  reset(); }

    VectorXf				evalF(const VectorXf&) const override;

    // Helper functions to access the 2D positions and velocities in state vectors.
    static auto				position(VectorXf& X, int idx)          { return Map<Vector2f>(&X(idx * 4)); }
    static auto				position(const VectorXf& X, int idx)    { return Map<const Vector2f>(&X(idx * 4)); }
    static auto				velocity(VectorXf& X, int idx)          { return Map<Vector2f>(&X(idx * 4 + 2)); }
    static auto				velocity(const VectorXf& X, int idx)    { return Map<const Vector2f>(&X(idx * 4 + 2)); }

    void					reset() override;
    void					render(const VectorXf& X) const override;
    string					dimension_name(unsigned d) const override;
    void					imgui_interface() override;

private:
    unsigned				n_;
    vector<Spring>			springs_;
    float					k_ = 1000.0f;
    float					drag_k_ = 0.5f;
};

// ----------------------------------------------------------------------------
class ClothSystem : public ParticleSystem
{
public:
    ClothSystem(unsigned x, unsigned y)                             { x_ = x; y_ = y; reset(); }

    VectorXf                evalF(const VectorXf&) const override;

    // Helper functions to access the 3D positions and velocities in state vectors.
    static auto             position(VectorXf& X, int idx)          { return Map<Vector3f>(&X(idx * 6)); }
    static auto             position(const VectorXf& X, int idx)    { return Map<const Vector3f>(&X(idx * 6)); }
    static auto             velocity(VectorXf& X, int idx)          { return Map<Vector3f>(&X(idx * 6 + 3)); }
    static auto             velocity(const VectorXf& X, int idx)    { return Map<const Vector3f>(&X(idx * 6 + 3)); }
    //const  vector<Spring>& getSprings() const { return springs_; }

    void					reset() override;
    void					render(const VectorXf& X) const override;
    string					dimension_name(unsigned d) const override;
    void					imgui_interface() override;

    Vector2i				getSize() { return Vector2i(x_, y_); }

private:
    unsigned				x_, y_;
    vector<Spring>			springs_;
    float					k_ = 300.0f;		// spring constant
    float					drag_k_ = 0.08f;	// dragf coefficient
};



class Particle {
public:
    Particle() { age = 0; };
    void setAge(float a) { age = a; }
    float getAge() const { return age; }

    void setPosition(Vector3f pos) { position = pos; }
    Vector3f getPosition() const { return position; }

    void setVelocity(Vector3f v) { velocity = v; }
    Vector3f getVelocity() const { return velocity; }

    Vector3f getColor() const { return color_; }
    void setColor(const Vector3f& c) { color_ = c; }

private:
    float age;
    Vector3f color_;
    Vector3f position;
    Vector3f velocity;
};

class SprinklerSystem : public ParticleSystem
{
public:
    SprinklerSystem(unsigned n) { n_ = n;  reset(); }

    VectorXf				evalF(const VectorXf& X) const override;
    
    static auto             position(VectorXf& X, int idx) { return Map<Vector3f>(&X(idx * 6)); }
    static auto             position(const VectorXf& X, int idx) { return Map<const Vector3f>(&X(idx * 6)); }
    static auto             velocity(VectorXf& X, int idx) { return Map<Vector3f>(&X(idx * 6 + 3)); }
    static auto             velocity(const VectorXf& X, int idx) { return Map<const Vector3f>(&X(idx * 6 + 3)); }

    void					reset() override;
    void					render(const VectorXf& X) const override;
    string					dimension_name(unsigned d) const override;
    void                    emit();
    void                    update(float dt);

private:
    unsigned				n_;
    float					radius_ = 0.5f;
    float                   spread_ = 0.1f;
    float                   colorSpread_ = 0.1f;
    float					drag_k_ = 0.08f;	// dragf coefficient

    vector<Particle>				alive_particles_;
};
