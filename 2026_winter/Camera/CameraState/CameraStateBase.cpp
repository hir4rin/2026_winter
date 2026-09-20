#include "CameraStateBase.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr int kShakeRandMax = 200;//カメラ揺れのランダム値の最大
	constexpr float kShakeRandNormalize = 100.0f;//ランダム値を-1.0~1.0の範囲に正規化するための除数
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
	m_shakeTimer -= 1.0;//

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
	m_blendElapsed += 1.0f;
	//経過時間の割合//0.0だったら1.0経過していることにする
	float t = (m_activeBlend.duration > 0.0f) ? m_blendElapsed / m_activeBlend.duration : 1.0f;
	t = std::clamp(t, 0.0f, 1.0f);
	//イージング //0～1の範囲で乗倍する
	float tEased = std::pow(t, m_activeBlend.easingPower);

	switch (m_activeBlend.mode)
	{
	case BlendSetting::Mode::None:
		m_pos = rawPos;
		m_target = rawTarget;
		break;

	case BlendSetting::Mode::Lerp:
		m_pos = Vector3::Lerp(m_startPos, rawPos, tEased);
		m_target = Vector3::Lerp(m_startTarget, rawTarget, tEased);
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
		//方向をSlerpかけて、距離をかけて、座標を算出
		m_pos = Vector3::Slerp(dirStart, dirTarget, tEased) * dist + pivot;

		//注視点はlerp
		m_target = Vector3::Lerp(m_startTarget, rawTarget, tEased);
		break;
	}
	default:
		break;
	}

}

void CameraStateBase::ResetBlend(const Vector3& startPos, const Vector3& startTarget)
{
	m_startPos = startPos;
	m_startTarget = startTarget;
	m_blendElapsed = 0.0f;
	m_activeBlend = GetBlendSetting();

	//ブレンド開始直後の見た目が飛ばないように、初期値をそのまま今の表示値にする
	m_pos = startPos;
	m_target = startTarget;
}

