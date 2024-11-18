#pragma once

#include "ray.h"

//#include "base/Math.h"

class Material;

struct Hit
{
public:
    Hit() {}
    Hit(float t_max) : t(t_max) {} 
    Hit(const Hit& h)
    { 
        t = h.t;
        material = h.material; 
        normal = h.normal;
    }

    void set(float tnew, shared_ptr<Material> m, const Vector3f& n)
    {
        t = tnew;
        material = m;
        normal = n;
    }

    float		            t           = FLT_MAX;// closest hit found so far
    shared_ptr<Material>	material    = nullptr;
    Vector3f	            normal      = Vector3f::Zero();
};

inline std::ostream& operator<<(std::ostream &os, const Hit& h) {
    os << "Hit <" << h.t << ", " << h.normal << ">";
    return os;
}