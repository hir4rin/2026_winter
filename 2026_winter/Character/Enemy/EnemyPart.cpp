#include "EnemyPart.h"
#include "../System.h"
#include "../../Game.h"

namespace
{
	constexpr float kRestitution = 0.4f;//地面に着いたときの跳ね返り具合//0だと跳ねない
	constexpr float kSpinSpeed = 0.1f;//回転する速さ(ラジアン/フレーム)
	constexpr float kSpinAxisShiftRange = 0.002f;//回転軸を変化させるランダム量//毎フレームの変化を小さくして、軸が少しずつ動くようにする
}

EnemyPart::EnemyPart(int modelHandle)
	:m_modelHandle(modelHandle)
{
}

EnemyPart::~EnemyPart()
{
}

void EnemyPart::OnCollision(Collider& other)
{
	//地面・壁との押し戻し以外は何もしない
}

void EnemyPart::ApplyPos()
{
	m_rb.m_pos += m_rb.m_vel;
}

void EnemyPart::Break(const MATRIX& baseMat, const Vector3& startPos, const Vector3& initialVel)
{
	m_baseMat = baseMat;
	m_rb.m_pos = startPos;
	m_vel = initialVel;
	m_spinAngle = 0.0f;
	m_isLanded = false;

	//適当な軸から回転を始める(以後Updateで少しずつ向きを変化させる)
	m_spinAxis = Vector3(
		static_cast<float>(GetRand(200) - 100),
		static_cast<float>(GetRand(200) - 100),
		static_cast<float>(GetRand(200) - 100)).Normalize();

	SetIsActive(true);
}

void EnemyPart::Update()
{
	if (!GetIsActive())return;

	float timeScale = System::GetInstance().GetTimeScale();

	//前フレームの当たり判定の結果、地面に着いていてまだ沈み込む速度なら跳ね返す
	//(重力は止めずに毎フレームかけ続けるので、跳ねる勢いは自然に収まっていく)
	if (IsFloor() && m_vel.y < 0.0f)
	{
		m_vel.y *= -kRestitution;
		m_vel.x *= kRestitution;
		m_vel.z *= kRestitution;
		m_isLanded = true;//初めて着地したら、以後は回転だけ止める
	}

	//重力
	m_vel.y += -Game::kGravity * timeScale;

	if (!m_isLanded)
	{
		//回転軸をランダムに少しずつ変化させる(ずっと同じ軸だと不自然な回り方になるため)
		Vector3 axisShift = Vector3(
			static_cast<float>(GetRand(200) - 100),
			static_cast<float>(GetRand(200) - 100),
			static_cast<float>(GetRand(200) - 100)) * kSpinAxisShiftRange;
		m_spinAxis = (m_spinAxis + axisShift).Normalize();

		m_spinAngle += kSpinSpeed * timeScale;
	}

	//RigidBodyのvelはCollisionManagerが押し戻し等で書き換えるため、毎フレーム自前の速度で上書きする
	m_rb.m_vel = m_vel;
}

void EnemyPart::Draw()
{
	if (!GetIsActive())return;

	MATRIX spinMat = MGetRotAxis(m_spinAxis.ToDxLibVector(), m_spinAngle);
	MATRIX mat = MMult(spinMat, m_baseMat);

	VECTOR pos = GetWorldPos().ToDxLibVector();
	mat.m[3][0] = pos.x;
	mat.m[3][1] = pos.y;
	mat.m[3][2] = pos.z;

	MV1SetMatrix(m_modelHandle, mat);
	MV1DrawModel(m_modelHandle);
}
