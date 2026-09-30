#pragma once
#include <cmath>

//イージング種別。CameraStateBase::BlendSettingのeasingModeで選ぶ
enum class EasingMode
{
	EaseIn,			//t^power。power>1で出だしゆっくり、<1で出だし速い(従来のカメラブレンドと同じ)
	EaseOut,		//1-(1-t)^power。最後がゆっくり止まる
	EaseInOut,		//両端ゆっくり、中間が速い
	EaseOutBack,	//目標を少し行き過ぎてから戻る(powerは使わない)。1.0を超える値を返す
	EaseOutExpo,	//最初にほぼ到達して、残りをじわっと詰める(powerは使わない)
};

namespace Easing
{
	constexpr float kBackOvershoot = 1.70158f;//EaseOutBackの行き過ぎ量(約10%)

	//tは0~1。powerはEaseIn/EaseOut/EaseInOutでのみ使う
	inline float Apply(EasingMode mode, float t, float power = 1.0f)
	{
		switch (mode)
		{
		case EasingMode::EaseIn:
			return std::pow(t, power);

		case EasingMode::EaseOut:
			return 1.0f - std::pow(1.0f - t, power);

		case EasingMode::EaseInOut:
			if (t < 0.5f)
			{
				return 0.5f * std::pow(2.0f * t, power);
			}
			return 1.0f - 0.5f * std::pow(2.0f * (1.0f - t), power);

		case EasingMode::EaseOutBack:
		{
			const float c1 = kBackOvershoot;
			const float c3 = c1 + 1.0f;
			const float u = t - 1.0f;
			return 1.0f + c3 * u * u * u + c1 * u * u;
		}

		case EasingMode::EaseOutExpo:
			return (t >= 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
		}
		return t;
	}
}
