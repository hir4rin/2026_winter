#pragma once//一回しかインクルードされないようにするためのプリプロセッサ命令
#include <cmath>
#include "DxLib.h"

//class DxLib;

class Matrix4x4
{
	//外部用
	friend VECTOR operator*(VECTOR& vec, Matrix4x4 it);

	//常に単項演算子系は左の文字が呼ぶ
public:
	//引き数なしコンストラクタ
	Matrix4x4() :
		m00(0), m01(0), m02(0), m03(0),
		m10(0), m11(0), m12(0), m13(0),
		m20(0), m21(0), m22(0), m23(0),
		m30(0), m31(0), m32(0), m33(0)
	{
	}
	//引き数付きコンストラクタ
	Matrix4x4(float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33) :
		m00(m00), m01(m01), m02(m02), m03(m03),
		m10(m10), m11(m11), m12(m12), m13(m13),
		m20(m20), m21(m21), m22(m22), m23(m23),
		m30(m30), m31(m31), m32(m32), m33(m33)
	{
	}
	//�P�����Z�q
	//+a
	Matrix4x4 operator+() const
	{
		return *this;
	}
	//-a
	Matrix4x4 operator-() const
	{
		Matrix4x4 a{};
		a.m00 = -m00; a.m10 = -m10; a.m20 = -m20; a.m30 = -m30;
		a.m01 = -m01; a.m11 = -m11; a.m21 = -m21; a.m31 = -m31;
		a.m02 = -m02; a.m12 = -m12; a.m22 = -m22; a.m32 = -m32;
		a.m03 = -m03; a.m13 = -m13; a.m23 = -m23; a.m33 = -m33;
		return a;
	}
	//�s��̉��Z�Ɨv�f���Ƃɍs��
	Matrix4x4 operator+(Matrix4x4 it) const
	{
		return Matrix4x4(m00 + it.m00, m01 + it.m01, m02 + it.m02, m03 + it.m03,
			m10 + it.m10, m11 + it.m11, m12 + it.m12, m13 + it.m13,
			m20 + it.m20, m21 + it.m21, m22 + it.m22, m23 + it.m23,
			m30 + it.m30, m31 + it.m31, m32 + it.m32, m33 + it.m33);
	}
	Matrix4x4 operator+=(Matrix4x4 it)
	{
		m00 += it.m00; m01 += it.m01; m02 += it.m02; m03 += it.m03;
		m10 += it.m10; m11 += it.m11; m12 += it.m12; m13 += it.m13;
		m20 += it.m20; m21 += it.m21; m22 += it.m22; m23 += it.m23;
		m30 += it.m30; m31 += it.m31; m32 += it.m32; m33 += it.m33;
		return *this;
	}

	//�s��̌��Z�͗v�f���Ƃɍs��
	Matrix4x4 operator-(Matrix4x4 it) const
	{
		return Matrix4x4(m00 - it.m00, m01 - it.m01, m02 - it.m02, m03 - it.m03,
			m10 - it.m10, m11 - it.m11, m12 - it.m12, m13 - it.m13,
			m20 - it.m20, m21 - it.m21, m22 - it.m22, m23 - it.m23,
			m30 - it.m30, m31 - it.m31, m32 - it.m32, m33 - it.m33);
	}
	Matrix4x4 operator-=(Matrix4x4 it)
	{
		m00 -= it.m00; m01 -= it.m01; m02 -= it.m02; m03 -= it.m03;
		m10 -= it.m10; m11 -= it.m11; m12 -= it.m12; m13 -= it.m13;
		m20 -= it.m20; m21 -= it.m21; m22 -= it.m22; m23 -= it.m23;
		m30 -= it.m30; m31 -= it.m31; m32 -= it.m32; m33 -= it.m33;
		return *this;
	}
	//�s���float�̏�Z
	Matrix4x4 operator*(float it) const
	{
		return Matrix4x4(m00 * it, m01 * it, m02 * it, m03 * it,
			m10 * it, m11 * it, m12 * it, m13 * it,
			m20 * it, m21 * it, m22 * it, m23 * it,
			m30 * it, m31 * it, m32 * it, m33 * it);
	}
	//�s��ƍs��
	Matrix4x4 operator*(Matrix4x4 it) const
	{
		return Matrix4x4(
			m00 * it.m00 + m01 * it.m10 + m02 * it.m20 + m03 * it.m30,
			m00 * it.m01 + m01 * it.m11 + m02 * it.m21 + m03 * it.m31,
			m00 * it.m02 + m01 * it.m12 + m02 * it.m22 + m03 * it.m32,
			m00 * it.m03 + m01 * it.m13 + m02 * it.m23 + m03 * it.m33,
			m10 * it.m00 + m11 * it.m10 + m12 * it.m20 + m13 * it.m30,
			m10 * it.m01 + m11 * it.m11 + m12 * it.m21 + m13 * it.m31,
			m10 * it.m02 + m11 * it.m12 + m12 * it.m22 + m13 * it.m32,
			m10 * it.m03 + m11 * it.m13 + m12 * it.m23 + m13 * it.m33,
			m20 * it.m00 + m21 * it.m10 + m22 * it.m20 + m23 * it.m30,
			m20 * it.m01 + m21 * it.m11 + m22 * it.m21 + m23 * it.m31,
			m20 * it.m02 + m21 * it.m12 + m22 * it.m22 + m23 * it.m32,
			m20 * it.m03 + m21 * it.m13 + m22 * it.m23 + m23 * it.m33,
			m30 * it.m00 + m31 * it.m10 + m32 * it.m20 + m33 * it.m30,
			m30 * it.m01 + m31 * it.m11 + m32 * it.m21 + m33 * it.m31,
			m30 * it.m02 + m31 * it.m12 + m32 * it.m22 + m33 * it.m32,
			m30 * it.m03 + m31 * it.m13 + m32 * it.m23 + m33 * it.m33
		);
	}
	//�s��ƍs��
	Matrix4x4 operator*=(Matrix4x4 it)
	{
		*this = *this * it;
		return *this;
	}
	//�s���float�̊���Z
	Matrix4x4 operator/(float it) const
	{
		return Matrix4x4(m00 / it, m01 / it, m02 / it, m03 / it,
			m10 / it, m11 / it, m12 / it, m13 / it,
			m20 / it, m21 / it, m22 / it, m23 / it,
			m30 / it, m31 / it, m32 / it, m33 / it);
	}
	//X����]�s���쐬����֐�
	static Matrix4x4 MakeRotationX(float angle)
	{
		float cosA = cosf(angle);
		float sinA = sinf(angle);
		return Matrix4x4(1, 0, 0, 0,
						0, cosA, -sinA, 0,
						0, sinA, cosA, 0,
						0, 0, 0, 1);
	}
	//Y����]�s���쐬����֐�
	static Matrix4x4 MakeRotationY(float angle)
	{
		float cosA = cosf(angle);
		float sinA = sinf(angle);
		return Matrix4x4(cosA, 0, sinA, 0,
						0, 1, 0, 0,
						-sinA, 0, cosA, 0,
						0, 0, 0, 1);
	}
	//Z����]�s���쐬����֐�
	static Matrix4x4 MakeRotationZ(float angle)
	{
		float cosA = cosf(angle);
		float sinA = sinf(angle);
		return Matrix4x4(cosA, -sinA, 0, 0,
						sinA, cosA, 0, 0,
						0, 0, 1, 0,
						0, 0, 0, 1);
	}
	//DxLib�͍s��ł͂Ȃ��A��s�Ȃ̂ŁA�s���]�u����֐�
	 Matrix4x4 Transpose() const
	{
		return Matrix4x4(m00, m10, m20, m30,
						m01, m11, m21, m31,
						m02, m12, m22, m32,
						m03, m13, m23, m33);
	}
	 //DxLib����󂯎�����s���s��N���X�ɕϊ�����֐�
	  static Matrix4x4 FromDxLibMatrix(const MATRIX& dxMat)
	  {
		  return Matrix4x4(dxMat.m[0][0], dxMat.m[1][0], dxMat.m[2][0], dxMat.m[3][0],
			  dxMat.m[0][1], dxMat.m[1][1], dxMat.m[2][1], dxMat.m[3][1],
			  dxMat.m[0][2], dxMat.m[1][2], dxMat.m[2][2], dxMat.m[3][2],
			  dxMat.m[0][3], dxMat.m[1][3], dxMat.m[2][3], dxMat.m[3][3]);

	  }

	 //������s��N���X��DxLib�̍s��ɕϊ�����֐�
	  static MATRIX ToDxLibMatrix(const Matrix4x4& mat)
	  {
		  return MATRIX{ mat.m00, mat.m10, mat.m20, mat.m30,
						  mat.m01, mat.m11, mat.m21, mat.m31,
						  mat.m02, mat.m12, mat.m22, mat.m32,
						  mat.m03, mat.m13, mat.m23, mat.m33 };
	  }







private:
	float m00, m01, m02, m03;
	float m10, m11, m12, m13;
	float m20, m21, m22, m23;
	float m30, m31, m32, m33;


};




