#pragma once

//#include "base/Math.h"

#include <iostream>

struct Ray
{
	Ray(const Vector3f& orig, const Vector3f& dir) {
		origin = orig;
		direction = dir;
	}

	Vector3f pointAtParameter(float t) const {
		return origin + direction * t;
	}

	Vector3f origin;
	Vector3f direction;
};

inline std::ostream& operator<<(std::ostream& os, const Vector3f& v) {
	os << "[" << v(0) << ", " << v(1) << ", " << v(2) << "]";
}

inline std::ostream& operator<<(std::ostream& os, const Vector2f& v) {
	os << "[" << v(0) << ", " << v(1) << "]";
}

inline std::ostream& operator<<(std::ostream& os, const Ray& r) {
	os << "Ray <" << r.origin << ", " << r.direction << ">";
	return os;
}