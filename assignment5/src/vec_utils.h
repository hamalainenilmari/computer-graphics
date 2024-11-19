#pragma once



template<typename DerivedInput, typename DerivedLimit>
inline typename DerivedInput::PlainObject clip(const MatrixBase<DerivedInput>& a, const MatrixBase<DerivedLimit>& low, const MatrixBase<DerivedLimit>& high)
{
    return typename DerivedInput::PlainObject(a.cwiseMin(high).cwiseMax(low));
}

template<typename Derived>
inline typename Derived::PlainObject clip(const MatrixBase<Derived>& a, typename Derived::Scalar low, typename Derived::Scalar high)
{
    return typename Derived::PlainObject(a.cwiseMin(high).cwiseMax(low));
}

template<typename T>
inline T clip(const T& a, const T& low, const T& high)
{
    return a < low ? low : (a > high ? high : a);
}

template<typename Scalar, int N>
inline Vector<Scalar, N + 1> promote(const Vector<Scalar, N>& v)
{
    Vector<Scalar, N + 1> result;
    result.block(0, 0, N, 1) = v;
    result[N] = 1.0f;
    return result;
}

class VecUtils
{
public:
	//static Matrix4f rotate(const Vector3f& axis, float angle)
	//{
	//	Matrix3f R = Matrix3f::rotation(axis, angle);
	//	Matrix4f R4;
	//	R4.setIdentity();
	//	R4.setCol(0, Vector4f(Vector3f(R.getCol(0)), 0.0f));
	//	R4.setCol(1, Vector4f(Vector3f(R.getCol(1)), 0.0f));
	//	R4.setCol(2, Vector4f(Vector3f(R.getCol(2)), 0.0f));
	//	return R4;
	//}

	// transforms a 3D point using a matrix, returning a 3D point
	static Vector3f transformPoint(const Matrix4f& mat, const Vector3f& point)
	{
		return (mat * Vector4f{ point(0), point(1), point(2), 1.0f }).head(3);
	}

	// transform a 3D direction using a matrix, returning a direction
	static Vector3f transformDirection(const Matrix4f& mat, const Vector3f& dir)
	{
		return mat.block(0, 0, 3, 3) * dir;
	}
};
