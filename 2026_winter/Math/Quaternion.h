#pragma once
#include"DxLib.h"
#include "Matrix4x4.h"
class Quaternion
{
public:
	Quaternion();
	//クォータニオンの**単位元（恒等クォータニオン）**は (0, 0, 0, 1) です。
	Quaternion(float i, float j, float k, float w0 = 0);//実部はデフォルトで0//純粋クォータニオンを作るために実部は0にする//不変の時は実部は1になる
	virtual ~Quaternion();

	Quaternion operator*(const Quaternion other) const;
	//クォータニオンの回転
	static VECTOR RotateQuaternion(VECTOR axis, VECTOR prev, float theta);//axisは回転軸、prevは回転前のベクトル、thetaは回転角度
	//クォータニオンを行列に変換する関数
	Matrix4x4 ToMatrix() const;


private:
	//クォータニオンのi,j,k
	float q1;//iの部分
	float q2;//jの部分
	float q3;//kの部分
	float w;//実数
};

