#pragma once
#include <DxLib.h>
class Vector2;
class Vector3
{
public:
	Vector3();
	Vector3(float x, float y, float z);
	Vector3(VECTOR vec);
	virtual ~Vector3();
	/// <summary>
	/// ベクトルの大きさを返す
	/// </summary>
	/// <returns></returns>
	float Magnitude()const;

	/// <summary>
	/// ベクトルの大きさの2乗を返す
	/// </summary>
	/// <returns></returns>
	float sqMagnitude()const;

	/// <summary>
	/// 正規化(大きさを1にする)
	/// </summary>
	/// <returns></returns>
	Vector3 Normalize() const;

	/// <summary>
	/// 内積
	/// </summary>
	/// <param name="right"></param>
	/// <returns></returns>
	float Dot(const Vector3& right)const;

	/// <summary>
	/// 外積
	/// </summary>
	/// <param name="right"></param>
	/// <returns></returns>
	Vector3 Cross(const Vector3& right)const;
	/// <summary>
	/// 2次元空間のベクトルは|a||b|sinθを返す
	/// XとZを使う
	/// </summary>
	/// <returns></returns>
	float Cross2DXZ(const Vector3& right)const;

	/// <summary>
	/// DxLibのベクトルに変換します
	/// </summary>
	/// <returns></returns>
	VECTOR ToDxLibVector()const;

	/// <summary>
	/// DxLibのベクトルからVecotr3に変換します
	/// </summary>
	/// <returns></returns>
	static Vector3 FromDxLibVector(const VECTOR& vec);

	//lerp
	static Vector3 Lerp(const Vector3& start, const Vector3& end, float t);
	//Slerp//球面線形補間//ベクトルの補完//大きさが1のものを入れる
	static Vector3 Slerp(const Vector3& start, const Vector3& end, float t);

	//演算子オーバーロード
	Vector3 operator+(const Vector3& right)const;
	Vector3 operator-(const Vector3& right)const;
	Vector3 operator*(const float& right)const;
	Vector3 operator/(const float& right)const;	
	Vector3 operator=(const float& right)const;
	Vector3 operator+=(const Vector3& right);
	Vector3 operator-=(const Vector3& right);
	Vector3 operator*=(const float& right);
	Vector3 operator/=(const float& right);
public:
	float x, y, z;
};

