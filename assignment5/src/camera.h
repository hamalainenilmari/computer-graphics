#pragma once

#include "ray.h"
//#include "base/Math.h"

#include <cassert>
#include <cfloat>
#include <cmath>

class Camera
{
public:
	// generate rays for each screen-space coordinate
    virtual Ray generateRay(const Vector2f& point, float fAspect) = 0;

	static inline Vector2f normalizedImageCoordinateFromPixelCoordinate(const Vector2f& pixel, const Vector2i& imageSize) {
		// YOUR CODE HERE (R1)
		// Given floating-point pixel coordinates (px,py), you should return the corresponding normalized screen coordinates in [-1,1]^2
		// Pay attention to which direction is "up" :)

		float x = pixel.x();
		float y = pixel.y();

		float imageX = 2.0f * (x / imageSize.x()) - 1.0f;
		float imageY = 1.0f - 2.0f * (y / imageSize.y());

		return Vector2f(imageX, imageY);
	}
	
    virtual float getTMin() const = 0;

	Matrix3f getOrientation()
    {
		Matrix3f result;
		result.col(0) = horizontal;
		result.col(1) = up;
		result.col(2) = direction;
		return result;
	}

	void setOrientation(Matrix3f orientation)
    {
		horizontal	= orientation.col(0);
		up			= orientation.col(1);
		direction	= orientation.col(2);
	}

    Vector3f    getCenter()                     { return center; }
    void        setCenter(Vector3f position)    { center = position; }

	virtual bool isOrtho() const = 0;

protected:
	Vector3f center; 
	Vector3f direction;
	Vector3f up;
	Vector3f horizontal;
};


class OrthographicCamera: public Camera
{
public:
	OrthographicCamera(const Vector3f& center, const Vector3f& direction, const Vector3f& up, float size) {
		this->center = center;
		this->direction = direction.normalized();	
		this->horizontal = direction.cross(up).normalized();
		// need to make an orthonormal vector to the projection
		this->up = horizontal.cross(direction).normalized();
		this->size = size;
	}

	virtual Ray generateRay(const Vector2f& point, float fAspect ) override
    {
		// YOUR CODE HERE (R1)
		// Generate a ray with the given screen coordinates, which you should assume lie in [-1,1]^2
		return Ray(center + (size * fAspect * horizontal * point.x())/2 + (size * up * point.y())/2, direction);
	}

	bool isOrtho() const override { return true; }
	float getSize() const { return size; }
	void setSize(float new_size) { size = new_size; }

	virtual float getTMin() const {
		return -1.0f*FLT_MAX;
	}

private:
	float size ; 
};


class PerspectiveCamera: public Camera
{
public:
	PerspectiveCamera(const Vector3f& center_, const Vector3f& direction_, const Vector3f& up_ , float fov_y)
	{
		this->center = center_;
		this->direction = direction_.normalized();
		this->horizontal = direction_.cross(up_).normalized();
		this->up = horizontal.cross(direction_).normalized();
		this->fov_y = fov_y;
	}

	virtual Ray generateRay( const Vector2f& point, float fAspect ) override
	{	
		// YOUR CODE HERE (R3)
		// Generate a ray with the given screen coordinates, which you should assume lie in [-1,1]^2
		// How to do this is described in the lecture notes.
		float d = 1.0f / tan(fov_y / 2.0f);
		Vector3f newDirection =
			(point.x() * horizontal * fAspect) +
			(point.y() * up) +
			direction * d;

		newDirection.normalize();
		return Ray(center, newDirection);
	}

	bool isOrtho() const override { return false; }
	float getFov() const { return fov_y; }
	void setFov(float new_fov) { fov_y = new_fov; }

	virtual float getTMin() const { 
		return 0.0f;
	}

private:
	float fov_y;
};