#include "Vector3.h"
#include <cmath>
#include <algorithm>

namespace
{
	constexpr float kSlerpEpsilon = 0.0001f;//Slerpでのゼロ除算を防ぐための許容誤差
}

Vector3::Vector3() :
	x(0.0f),
	y(0.0f),
	z(0.0f)
{
}

Vector3::Vector3(float x, float y, float z) :
	x(x),
	y(y),
	z(z)
{
}

Vector3::Vector3(VECTOR vec) :
	x(vec.x),
	y(vec.y),
	z(vec.z)
{
}

Vector3::~Vector3()
{
}

float Vector3::Magnitude()const
{
	return sqrtf(x * x + y * y + z * z);
}

float Vector3::sqMagnitude() const
{
	return x * x + y * y + z * z;
}

Vector3 Vector3::Normalize() const
{
	if (Magnitude() <= 0.0f)
	{
		return *this;
	}

	Vector3 ans;
	ans = (*this) / Magnitude();
	return ans;
}

float Vector3::Dot(const Vector3& right) const
{
	//a1b1 + a2b2 + a3b3
	float ans;
	ans = x * right.x + y * right.y + z * right.z;
	return ans;
}

Vector3 Vector3::Cross(const Vector3& right) const
{
	//a2b3 - a3b2, a3b1 - a1b3, a1b2 - a2b1

	Vector3 ans;
	ans.x = y * right.z - z * right.y;
	ans.y = z * right.x - x * right.z;
	ans.z = x * right.y - y * right.x;
	return ans;
}

float Vector3::Cross2DXZ(const Vector3& right) const
{
	float ans;
	ans = x * right.z - z * right.x;
	return ans;
}

VECTOR Vector3::ToDxLibVector()const
{
	return VGet(x, y, z);
}

Vector3 Vector3::FromDxLibVector(const VECTOR& vec)
{
	return Vector3(vec.x,vec.y,vec.z);
}

Vector3 Vector3::Lerp(const Vector3& start, const Vector3& end, float t)
{
	return Vector3(start + (end - start) * t);
}

Vector3 Vector3::Slerp(const Vector3& start, const Vector3& end, float t)
{
	


	Vector3 ans;
	//Θをacos(内積)で求める//ベクトルを正規化する
	Vector3 startNorm = start.Normalize();
	Vector3 endNorm = end.Normalize();
	float dot = startNorm.Dot(endNorm);
	//clampする
	dot = std::clamp(dot, -1.0f, 1.0f);
	float rad = acos(dot);
	
	//ゼロ除算を除く
	if (rad <= kSlerpEpsilon)return start;
	float sinRad = sinf(rad);

	if (fabsf(sinRad) < kSlerpEpsilon)
	{
		return start;
	}

	ans = startNorm * (sin((1 - t) * rad) / sin(rad)) + endNorm * (sin(t * rad) / sin(rad));

	return ans;
}

Vector3 Vector3::operator+(const Vector3& right) const
{
	Vector3 ans;
	ans.x = x + right.x;
	ans.y = y + right.y;
	ans.z = z + right.z;
	return ans;
}
Vector3 Vector3::operator-(const Vector3& right) const
{
	Vector3 ans;
	ans.x = x - right.x;
	ans.y = y - right.y;
	ans.z = z - right.z;
	return ans;
}
Vector3 Vector3::operator*(const float& right) const
{
	Vector3 ans;
	ans.x = x * right;
	ans.y = y * right;
	ans.z = z * right;
	return ans;
}
Vector3 Vector3::operator/(const float& right) const
{
	Vector3 ans;
	ans.x = x / right;
	ans.y = y / right;
	ans.z = z / right;
	return ans;
}
Vector3 Vector3::operator=(const float& right) const
{
	Vector3 ans;
	ans.x = right;
	ans.y = right;
	ans.z = right;
	return ans;
}
Vector3 Vector3::operator+=(const Vector3& right)
{
	x += right.x;
	y += right.y;
	z += right.z;
	return *this;
}
Vector3 Vector3::operator-=(const Vector3& right)
{
	x -= right.x;
	y -= right.y;
	z -= right.z;
	return *this;
}
Vector3 Vector3::operator*=(const float& right)
{
	x = x * right;
	y = y * right;
	z = z * right;
	return *this;
}
Vector3 Vector3::operator/=(const float& right)
{
	x = x / right;
	y = y / right;
	z = z / right;
	return *this;
}
