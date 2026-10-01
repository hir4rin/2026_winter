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
	//Θをacos(内積)で求める//ベクトルを正規化する
	Vector3 startNorm = start.Normalize();
	Vector3 endNorm = end.Normalize();

	//ゼロベクトルは補間できないのでそのまま返す
	if (startNorm.sqMagnitude() <= 0.0f || endNorm.sqMagnitude() <= 0.0f)return startNorm;

	float dot = startNorm.Dot(endNorm);
	//clampする
	dot = std::clamp(dot, -1.0f, 1.0f);
	float rad = acosf(dot);

	//ほぼ同じ向きなら補間不要
	if (rad <= kSlerpEpsilon)return startNorm;

	float sinRad = sinf(rad);

	//ほぼ真逆(180度)だと回転軸が一意に決まらないので、startに垂直な任意の軸を選んで回す
	if (fabsf(sinRad) < kSlerpEpsilon)
	{
		//startと平行になりにくい軸を選ぶ
		Vector3 axis = (fabsf(startNorm.y) < 0.9f) ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
		//startに垂直な単位ベクトル
		Vector3 perp = axis.Cross(startNorm).Normalize();
		//start→perp方向へ、π*tだけ回す
		float angle = DX_PI_F * t;
		return startNorm * cosf(angle) + perp * sinf(angle);
	}

	return startNorm * (sinf((1.0f - t) * rad) / sinRad) + endNorm * (sinf(t * rad) / sinRad);
}

Vector3 Vector3::OrbitLerp(const Vector3& start, const Vector3& end, float t, OrbitDirection dir)
{
	Vector3 startNorm = start.Normalize();
	Vector3 endNorm = end.Normalize();

	//ほぼ真上/真下だと水平角が定まらないので、通常のSlerpに任せる
	float horizS = sqrtf(startNorm.x * startNorm.x + startNorm.z * startNorm.z);
	float horizE = sqrtf(endNorm.x * endNorm.x + endNorm.z * endNorm.z);
	if (horizS < kSlerpEpsilon || horizE < kSlerpEpsilon)
	{
		return Slerp(startNorm, endNorm, t);
	}

	float yawS = atan2f(startNorm.x, startNorm.z);
	float yawE = atan2f(endNorm.x, endNorm.z);

	//水平角の差を-π~πに正規化(最短側)
	float dYaw = yawE - yawS;
	dYaw = fmodf(dYaw + DX_PI_F, DX_TWO_PI_F);
	if (dYaw < 0.0f)dYaw += DX_TWO_PI_F;
	dYaw -= DX_PI_F;

	//向きが指定されていて最短と逆なら、反対側(もう片方の弧)に回す
	//yawが増える向き=真上から見て時計回り
	if (dir == OrbitDirection::Clockwise && dYaw < 0.0f)dYaw += DX_TWO_PI_F;
	else if (dir == OrbitDirection::CounterClockwise && dYaw > 0.0f)dYaw -= DX_TWO_PI_F;

	float pitchS = asinf(std::clamp(startNorm.y, -1.0f, 1.0f));
	float pitchE = asinf(std::clamp(endNorm.y, -1.0f, 1.0f));

	//tが1.0を超えても(EaseOutBack)、そのまま外挿される
	float yaw = yawS + dYaw * t;
	float pitch = pitchS + (pitchE - pitchS) * t;

	float cosPitch = cosf(pitch);
	return Vector3(cosPitch * sinf(yaw), sinf(pitch), cosPitch * cosf(yaw));
}

Vector3 Vector3::EaseOrbitLerp(const Vector3& start, const Vector3& end, float t, EasingMode mode, float power, OrbitDirection dir)
{
	return OrbitLerp(start, end, Easing::Apply(mode, t, power), dir);
}

Vector3 Vector3::EaseLerp(const Vector3& start, const Vector3& end, float t, EasingMode mode, float power)
{
	return Lerp(start, end, Easing::Apply(mode, t, power));
}

Vector3 Vector3::EaseSlerp(const Vector3& start, const Vector3& end, float t, EasingMode mode, float power)
{
	return Slerp(start, end, Easing::Apply(mode, t, power));
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
