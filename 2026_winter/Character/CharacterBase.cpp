#include "CharacterBase.h"
#include "../Math/Matrix4x4.h"
#include "hitCol.h"
#include <cassert>
#include <algorithm>
#include <cmath>


namespace
{
	constexpr float kRotationLerpRate = 0.1f;//モデルの向きを目標角度に近づける速さ//ほぼlerp
	constexpr float kTiltLerpRate = 0.2f;//モデルの傾きを目標角度に近づける速さ//ほぼlerp

	constexpr float kHeadLookMaxAngle = DX_PI_F / 3.0f;//頭を向けられる最大の角度(60度)
	constexpr float kHeadLookGiveUpAngle = DX_PI_F * 5.0f / 9.0f;//これより後ろにあるときは見るのをやめて正面に戻す(100度)
	constexpr float kHeadLookLerpRate = 0.1f;//頭の向きを目標の角度に近づける速さ//ほぼlerp
}

CharacterBase::CharacterBase()
{
}

CharacterBase::~CharacterBase()
{
}


void CharacterBase::InitHitCol(std::weak_ptr<CharacterBase> owner)
{
	m_hitCol = std::make_shared<HitCol>(owner);
}

void CharacterBase::OnTriggerEnter(Collider& other)
{
}

void CharacterBase::OnTriggerExit(Collider& other)
{
}

Vector3 CharacterBase::GetWaistPos() const
{
	if (m_waistFrame < 0)return m_rb.m_pos;
	return Vector3::FromDxLibVector(MV1GetFramePosition(m_modelHandle, m_waistFrame));
}

void CharacterBase::UpdateAngleAndPos()
{
	float targetAngle = 0.0f;//目標の角度
	if (m_targetVec.Magnitude() > 0.0f)//最初の入力されないとき以外、ここを通り、モデルの向きを変える
	{
		//モデルの移動方向にモデルの方向を近づける
		targetAngle = atan2f(m_targetVec.x, m_targetVec.z);//移動ベクトルのx成分とz成分から、プレイヤーの向きたい方向の角度を求める
		// Y軸回転行列を作成する//この工程は毎フレーム、原点からモデルの位置に移動してから、回転する行列を作成している
		//180度ずれてたので、回転角度を180度ずらす
		/* m_rotAngle = targetAngle - DX_PI_F;*/
		 //角度の差分を計算//回転角度を-90から90にするため(最短経路を選択)
		float difference = targetAngle - m_rotAngleY - DX_PI_F;
		while (difference > DX_PI_F) difference -= 2.0f * DX_PI_F;
		while (difference < -DX_PI_F) difference += 2.0f * DX_PI_F;
		//targetAngle + DX_PI_F
		m_rotAngleY += difference * kRotationLerpRate;//回転角度を少しずつ目標の角度に近づける//ほぼlerp
	}
	//モデルは、座標の位置のcenter分下で表示


	//傾きを目標に少しずつ近づける
	m_tiltAngle += (m_targetTiltAngle - m_tiltAngle) * kTiltLerpRate;

	Matrix4x4 rotY = Matrix4x4::MakeRotationY(m_rotAngleY);
	//モデルのローカルの前後軸(Z)まわりに傾ける//足元(原点)が回転の中心
	Matrix4x4 tilt = Matrix4x4::MakeRotationZ(m_tiltAngle);
	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);

	//傾ける→Y回転→移動の順
	Matrix4x4 mtx = trans * rotY * tilt;
	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));
}

void CharacterBase::ApplyPos()
{
	//座標の更新
	m_rb.m_pos += m_rb.m_vel;

	//モデルの座標を更新する
	UpdateAngleAndPos();
}

void CharacterBase::UpdateHeadLook(int headFrame, bool isLook, const Vector3& targetPos)
{
	//頭のボーンが無いモデルは何もしない
	if (headFrame < 0)return;

	//Yawは水平
	float targetAngle = 0.0f;//向かないときは0(正面)に戻す
	if (isLook)
	{
		Vector3 toTarget = targetPos - m_rb.m_pos;
		//モデルの向きとの角度の差//モデルはm_rotAngleYから180度ずれた方を向いている(UpdateAngleAndPosと同じ)
		float difference = atan2f(toTarget.x, toTarget.z) - m_rotAngleY - DX_PI_F;
		while (difference > DX_PI_F)difference -= 2.0f * DX_PI_F;
		while (difference < -DX_PI_F)difference += 2.0f * DX_PI_F;
		//後ろすぎるときは正面に戻す//クランプだけだと、真後ろを横切った瞬間に首が反対側に振り切れるため
		if (fabsf(difference) < kHeadLookGiveUpAngle)
		{
			//真後ろまで首がまわらないようにクランプ
			targetAngle = std::clamp(difference, -kHeadLookMaxAngle, kHeadLookMaxAngle);
		}
	}

	m_headAngle += (targetAngle - m_headAngle) * kHeadLookLerpRate;//少しずつ目標の角度に近づける//ほぼlerp

	//上書きを一旦外して、アニメーションの頭の行列を取得する
	MV1ResetFrameUserLocalMatrix(m_modelHandle, headFrame);

	//ほぼ正面なら何もしない
	if (fabsf(m_headAngle) < 0.001f)return;

	MATRIX localMat = MV1GetFrameLocalMatrix(m_modelHandle, headFrame);

	MATRIX parentMat = MV1GetFrameLocalWorldMatrix(m_modelHandle, MV1GetFrameParent(m_modelHandle, headFrame));
	//ワールドの上方向を親ボーンの空間に変換して回転軸にする//mixamoのボーンは軸が傾いているっぽい
	VECTOR axis = VNorm(VTransformSR(VGet(0.0f, 1.0f, 0.0f), MInverse(parentMat)));

	//移動成分を外して回転を掛ける//頭の付け根を中心に回すため
	VECTOR localPos = VGet(localMat.m[3][0], localMat.m[3][1], localMat.m[3][2]);
	localMat.m[3][0] = 0.0f;
	localMat.m[3][1] = 0.0f;
	localMat.m[3][2] = 0.0f;
	localMat = MMult(localMat, MGetRotAxis(axis, m_headAngle));
	//移動成分を戻す
	localMat.m[3][0] = localPos.x;
	localMat.m[3][1] = localPos.y;
	localMat.m[3][2] = localPos.z;
	MV1SetFrameUserLocalMatrix(m_modelHandle, headFrame, localMat);
}

const std::string& CharacterBase::GetAnimName(const std::string& key) const
{
	auto it = m_animNames.find(key);
	assert(it != m_animNames.end() && "指定されたキーが見つかりませんでした");
	return it->second;
}
