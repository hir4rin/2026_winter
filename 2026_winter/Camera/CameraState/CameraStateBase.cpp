#include "CameraStateBase.h"
#include <algorithm>
#include <cmath>
#include "../System.h"

namespace
{
	constexpr int kShakeRandMax = 200;//カメラ揺れのランダム値の最大
	constexpr float kShakeRandNormalize = 100.0f;//ランダム値を-1.0~1.0の範囲に正規化するための除数

	constexpr float kChaseArriveDistance = 0.1f;//isChase時、目標とこれ以下の距離になったら追いついたとみなす
	constexpr float kPivotEpsilon = 0.0001f;//pivotとの距離がこれ以下だと方向が求まらないので直線で追う
}

CameraStateBase::CameraStateBase(std::weak_ptr<CameraManager> owner) :
	m_owner(owner)
{
}

CameraStateBase::~CameraStateBase()
{
}

void CameraStateBase::Draw()
{
}

void CameraStateBase::FixCameraPos()
{

}

void CameraStateBase::StartCameraShake(float power, float time)
{
	m_shakePower = power;
	m_shakeTimer = time;
	m_shakeTimerMax = time;
	m_isShaking = true;
}

Vector3 CameraStateBase::CameraShakeUpdate()
{
	if (m_shakeTimer <= 0.0f)
	{
		m_isShaking = false;
		return Vector3();
	}
	m_shakeTimer -= 1.0f * System::GetInstance().GetTimeScale();//

	float progress = m_shakeTimer / m_shakeTimerMax;//揺れの進行度合いを0から1の範囲で表す
	float currentPower = m_shakePower * progress;//現在の揺れの強さを計算する

	float magX = (GetRand(kShakeRandMax) / kShakeRandNormalize - 1.0f) * currentPower;
	float magY = (GetRand(kShakeRandMax) / kShakeRandNormalize - 1.0f) * currentPower;
	Vector3 mag = Vector3(magX, magY, 0.0f);

	return mag;


}

void CameraStateBase::CameraSetting()
{
	//そのフレームのカメラをセット
}

void CameraStateBase::UpdateBlend(const Vector3& rawPos, const Vector3& rawTarget)
{
	//カメラにタイムスケールを掛けるかどうかは審議
	//m_blendElapsed += 1.0f * System::GetInstance().GetTimeScale();
	m_blendElapsed += 1.0f;




	//経過時間の割合//0.0だったら1.0経過していることにする
	float t = (m_activeBlend.duration > 0.0f) ? m_blendElapsed / m_activeBlend.duration : 1.0f;
	t = std::clamp(t, 0.0f, 1.0f);
	//イージング //EaseOutBackは1.0を超えて行き過ぎることがある
	float tEased = Easing::Apply(m_activeBlend.easingMode, t, m_activeBlend.easingPower);

	//このフレームのイージング上の点(isChaseでなければそのまま使う、isChaseならこれを追いかける)
	Vector3 easedPos = rawPos;
	Vector3 easedTarget = rawTarget;

	switch (m_activeBlend.mode)
	{
	case BlendSetting::Mode::None:
		easedPos = rawPos;
		easedTarget = rawTarget;
		break;

	case BlendSetting::Mode::Lerp:
		easedPos = Vector3::EaseLerp(m_startPos, rawPos, t, m_activeBlend.easingMode, m_activeBlend.easingPower);
		easedTarget = Vector3::EaseLerp(m_startTarget, rawTarget, t, m_activeBlend.easingMode, m_activeBlend.easingPower);
		break;

	case BlendSetting::Mode::Slerp:
	{
		const Vector3& pivot = m_activeBlend.pivot;

		Vector3 dirStart = (m_startPos - pivot).Normalize();
		Vector3 dirTarget = (rawPos - pivot).Normalize();
		float distStart = (m_startPos - pivot).Magnitude();
		float distTarget = (rawPos - pivot).Magnitude();

		//距離の部分をlerp
		float dist = std::lerp(distStart, distTarget, tEased);
		//方向を水平回転で補間して(上を通らない)、距離をかけて、座標を算出
		easedPos = Vector3::EaseOrbitLerp(dirStart, dirTarget, t, m_activeBlend.easingMode, m_activeBlend.easingPower, m_activeBlend.orbitDirection) * dist + pivot;

		//注視点はlerp
		easedTarget = Vector3::EaseLerp(m_startTarget, rawTarget, t, m_activeBlend.easingMode, m_activeBlend.easingPower);
		break;
	}
	default:
		break;
	}

	if (!m_activeBlend.isChase)
	{
		m_pos = easedPos;
		m_target = easedTarget;
		return;
	}

	//イージングの点を追いかける(たどり着かない)
	ChaseStep(m_pos, m_target, easedPos, easedTarget, m_activeBlend);

	//イージングが終わった後、目標にほぼ重なったらそこで終了(見た目では分からない距離なのでスナップする)
	if (m_blendElapsed >= m_activeBlend.duration &&
		(rawPos - m_pos).Magnitude() < kChaseArriveDistance &&
		(rawTarget - m_target).Magnitude() < kChaseArriveDistance)
	{
		m_pos = rawPos;
		m_target = rawTarget;
		m_isChaseArrived = true;
	}
}

void CameraStateBase::ChaseStep(Vector3& pos, Vector3& target, const Vector3& goalPos, const Vector3& goalTarget, const BlendSetting& setting)
{
	const float rate = std::clamp(setting.chaseRate, 0.0f, 1.0f);

	//注視点は直線で追いかける
	target = Vector3::Lerp(target, goalTarget, rate);

	//Slerp以外は直線で追いかける
	if (setting.mode != BlendSetting::Mode::Slerp)
	{
		pos = Vector3::Lerp(pos, goalPos, rate);
		return;
	}

	//Slerpはpivot中心に、距離と水平方向を別々に追いかける(直線で追うと回り込みの内側をショートカットしてしまうため)
	const Vector3& pivot = setting.pivot;
	float distNow = (pos - pivot).Magnitude();
	float distGoal = (goalPos - pivot).Magnitude();
	//pivotと重なっていると方向が求まらないので直線で追う
	if (distNow < kPivotEpsilon || distGoal < kPivotEpsilon)
	{
		pos = Vector3::Lerp(pos, goalPos, rate);
		return;
	}

	float dist = std::lerp(distNow, distGoal, rate);
	//追いかける1歩は小さいので最短側でよい(回す向きはイージングの点の側ですでに決まっている)
	pos = Vector3::OrbitLerp((pos - pivot).Normalize(), (goalPos - pivot).Normalize(), rate, OrbitDirection::Shortest) * dist + pivot;
}

void CameraStateBase::ResetBlend(const Vector3& startPos, const Vector3& startTarget)
{
	m_startPos = startPos;
	m_startTarget = startTarget;
	m_blendElapsed = 0.0f;
	m_activeBlend = GetBlendSetting();
	m_isChaseArrived = false;

	//ブレンド開始直後の見た目が飛ばないように、初期値をそのまま今の表示値にする
	m_pos = startPos;
	m_target = startTarget;
}

