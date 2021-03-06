#include "MWQuaternion.h"
#include "MWMath.h"

namespace Myway
{

const Quat Quat::Identity = Quat(0, 0, 0, 1);

/* Quat 
--------------------------------------------------------------------------
    @Remark:
        Quaternion.
--------------------------------------------------------------------------
*/
inline Quat::Quat(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w)
{
}

inline Quat::Quat(const Quat &q) : x(q.x), y(q.y), z(q.z), w(q.w)
{
}

inline Quat & Quat::operator =(const Quat & q)
{
    x = q.x, y = q.y, z = q.z, w = q.w;
    return *this;
}

inline Quat Quat::operator -() const
{
    return Quat(-x, -y, -z, -w);
}

inline Quat Quat::operator +(const Quat & q) const
{
    return Quat(x + q.x, y + q.y, z + q.z, w + q.w);
}

inline Quat Quat::operator -(const Quat & q) const
{
    return Quat(x - q.x, y - q.y, z - q.z, w - q.w);
}

inline Quat Quat::operator *(const Quat & q) const
{
    Quat quat;
    Math::QuatMultiply(quat, *this, q);
    return quat;
}

inline Quat Quat::operator *(float v) const
{
    return Quat(x * v, y * v, z * v, w * v);
}

inline Quat Quat::operator /(float v) const
{
    v = 1.0f / v;
    return Quat(x * v, y * v, z * v, w * v);
}

inline Quat & Quat::operator +=(const Quat & q)
{
    x += q.x, y += q.y, z += q.z, w += q.w;
    return *this;
}

inline Quat & Quat::operator -=(const Quat & q)
{
    x -= q.x, y -= q.y, z -= q.z, w -= q.w;
    return *this;
}

inline Quat & Quat::operator *=(const Quat & q)
{
    Math::QuatMultiply(*this, *this, q);
    return *this;
}

inline Quat & Quat::operator *=(float v)
{
    x *= v, y *=v, z *= v, w *= v;
    return *this;
}

inline Quat & Quat::operator /=(float v)
{
    v = 1.0f / v;
    x *= v, y *=v, z *= v, w *= v;
    return *this;
}

inline bool Quat::operator ==(const Quat & q) const
{
    return x == q.x && y == q.y && z == q.z && w == q.w;
}

inline bool Quat::operator !=(const Quat & q) const
{
    return x != q.x || y != q.y || z != q.z || w != q.w;
}

inline Quat Myway::operator *(float v, const Quat & q)
{
    return q * v;
}

inline Quat Myway::operator /(float v, const Quat & q)
{
    return q / v;
}

Vec3 Myway::operator *(const Vec3 & v, const Quat & q)
{
    Vec3 vOut;
    Math::QuatRotation(vOut, q, v);
    return vOut;
}


float Quat::Dot(const Quat & rk) const
{
    return Math::QuatDot(*this, rk);
}

float Quat::Length() const
{
    return Math::QuatLength(*this);
}

Quat Quat::Inverse() const
{
    Quat quat;
    Math::QuatInverse(quat, *this);
    return quat;
}

Quat Quat::Normalize() const
{
    Quat quat;
    Math::QuatNormalize(quat, *this);
    return quat;
}

Quat Quat::Conjugate() const
{
    Quat quat;
    Math::QuatConjugate(quat, *this);
    return quat;
}

void Quat::FromAxis(const Vec3 & vAxis, float rads)
{
    Math::QuatFromAxis(*this, vAxis, rads);
}

void Quat::FromAxis(const Vec3 & xAxis, const Vec3 & yAxis, const Vec3 & zAxis)
{
    Math::QuatFromAxis(*this, xAxis, yAxis, zAxis);
}

void Quat::FromDir(const Vec3 & dir1, const Vec3 & dir2, const Vec3 & fallbackAxis, bool normalize)
{
    Math::QuatFromDir(*this, dir1, dir2, fallbackAxis, normalize);
}

void Quat::FromMatrix(const Mat4 & rot)
{
    Math::QuatFromMatrix(*this, rot);
}

void Quat::FromPitchYawRoll(float pitch, float yaw, float roll)
{
    Math::QuatFromPitchYawRoll(*this, pitch, yaw, roll);
}

Quat Quat::S_FromAxis(const Vec3 & vAxis, float rads)
{
	Quat q;

	q.FromAxis(vAxis, rads);

	return q;
}

Quat Quat::S_FromAxis(const Vec3 & xAxis, const Vec3 & yAxis, const Vec3 & zAxis)
{
	Quat q;

	q.FromAxis(xAxis, yAxis, zAxis);

	return q;
}

Quat Quat::S_FromDir(const Vec3 & dir1, const Vec3 & dir2, const Vec3 & fallbackAxis, bool normalize)
{
	Quat q;

	q.FromDir(dir1, dir2, fallbackAxis, normalize);

	return q;
}

Quat Quat::S_FromMatrix(const Mat4 & rot)
{
	Quat q;

	q.FromMatrix(rot);

	return q;
}

Quat Quat::S_FromPitchYawRoll(float pitch, float yaw, float roll)
{
	Quat q;

	q.FromPitchYawRoll(pitch, yaw, roll);

	return q;
}


Quat Quat::Slerp(const Quat & rk, float t)
{
    Quat quat;
    Math::QuatSlerp(quat, *this, rk, t);
    return quat;
}

void Quat::ToAxis(Vec3 & axis, float & rads) const
{
    Math::QuatToAxis(axis, rads, *this);
}

Mat4 Quat::ToMatrix() const
{
    Mat4 mat;
    Math::QuatToMatrix(mat, *this);
    return mat;
}

Vec3 Quat::Rotate(const Vec3 & v) const
{
    Vec3 vOut;
    Math::QuatRotation(vOut, *this, v);
    return vOut;
}

Vec3 Quat::AxisX() const
{
    Vec3 xAxis;
    Math::QuatAxisX(xAxis, *this);
    return xAxis;
}

Vec3 Quat::AxisY() const
{
    Vec3 yAxis;
    Math::QuatAxisY(yAxis, *this);
    return yAxis;
}

Vec3 Quat::AxisZ() const
{
    Vec3 zAxis;
    Math::QuatAxisZ(zAxis, *this);
    return zAxis;
}

void Quat::AxisXYZ(Vec3 & xAxis, Vec3 & yAxis, Vec3 & zAxis) const
{
    Math::QuatAxisXYZ(xAxis, yAxis, zAxis, *this);
}


float Quat::GetRoll(bool reprojectAxis) const
{
	if (reprojectAxis)
	{
		// roll = atan2(localx.y, localx.x)
		// pick parts of xAxis() implementation that we need
		//			float fTx  = 2.0*x;
		float fTy  = 2.0f*y;
		float fTz  = 2.0f*z;
		float fTwz = fTz*w;
		float fTxy = fTy*x;
		float fTyy = fTy*y;
		float fTzz = fTz*z;

		// Vector3(1.0-(fTyy+fTzz), fTxy+fTwz, fTxz-fTwy);

		return ::atan2f(fTxy+fTwz, 1.0f-(fTyy+fTzz));

	}
	else
	{
		return ::atan2f(2*(x*y + w*z), w*w + x*x - y*y - z*z);
	}
}
//-----------------------------------------------------------------------
float Quat::GetPitch(bool reprojectAxis) const
{
	if (reprojectAxis)
	{
		// pitch = atan2(localy.z, localy.y)
		// pick parts of yAxis() implementation that we need
		float fTx  = 2.0f*x;
		//			float fTy  = 2.0f*y;
		float fTz  = 2.0f*z;
		float fTwx = fTx*w;
		float fTxx = fTx*x;
		float fTyz = fTz*y;
		float fTzz = fTz*z;

		// Vector3(fTxy-fTwz, 1.0-(fTxx+fTzz), fTyz+fTwx);
		return ::atan2f(fTyz+fTwx, 1.0f-(fTxx+fTzz));
	}
	else
	{
		// internal version
		return ::atan2f(2*(y*z + w*x), w*w - x*x - y*y + z*z);
	}
}

//-----------------------------------------------------------------------
float Quat::GetYaw(bool reprojectAxis) const
{
	if (reprojectAxis)
	{
		// yaw = atan2(localz.x, localz.z)
		// pick parts of zAxis() implementation that we need
		float fTx  = 2.0f*x;
		float fTy  = 2.0f*y;
		float fTz  = 2.0f*z;
		float fTwy = fTy*w;
		float fTxx = fTx*x;
		float fTxz = fTz*x;
		float fTyy = fTy*y;

		// Vector3(fTxz+fTwy, fTyz-fTwx, 1.0-(fTxx+fTyy));

		return ::atan2f(fTxz+fTwy, 1.0f-(fTxx+fTyy));

	}
	else
	{
		// internal version
		return Math::ASin(-2*(x*z - w*y));
	}
}




}
