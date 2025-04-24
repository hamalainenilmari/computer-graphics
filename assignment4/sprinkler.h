#pragma once
/*
class Particle {
public:
    Particle() { age = 0; };
    void setAge(int a) { age = a; }
    int getAge() const { return age; }

    void setPosition(Vector2f pos) { position = pos; }
    Vector2f getPosition() const { return position; }

    void setVelocity(Vector2f v) { velocity = v; }
    Vector2f getVelocity() const { return velocity; }

private:
    int age;
    Vector3f color;
    Vector2f position;
    Vector2f velocity;
};

class SprinklerSystem 
{
public:
    SprinklerSystem(unsigned n) { n_ = n;  reset(); }

    //VectorXf				evalF(const VectorXf& X) const override;


    static auto				position(VectorXf& X, int idx) { return Map<Vector2f>(&X(idx * 4)); }          // write pos
    static auto				position(const VectorXf& X, int idx) { return Map<const Vector2f>(&X(idx * 4)); }    // read pos
    static auto				velocity(VectorXf& X, int idx) { return Map<Vector2f>(&X(idx * 4 + 2)); }      // write
    static auto				velocity(const VectorXf& X, int idx) { return Map<const Vector2f>(&X(idx * 4 + 2)); }// read

    void					reset() ;
    void					render() const;
    string					dimension_name(unsigned d) const;
    void                    emit();
    void                    update();

private:
    unsigned				n_;
    float					radius_ = 0.5f;
    float                   spread_ = 0.1f;
    float                   colorSpread_ = 0.1f;
    float					drag_k_ = 0.08f;	// dragf coefficient

    vector<Particle>				alive_particles_;
};
*/