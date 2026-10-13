#include "EffectManager.h"
#include "EffekseerForDXLib.h"
#include <algorithm>

namespace
{
	constexpr float kEffekseerFps = 60.0f;//エフェクトを作ったときのフレームレート(UpdateEffekseer3Dに渡す時間の基準)
}

int EffectManager::Play(AsyncData type, const Vector3& pos, float rotY, float scale)
{
	const int playingHandle = PlayEffekseer3DEffect(System::GetInstance().GetHandle(type));
	SetPosPlayingEffekseer3DEffect(playingHandle, pos.x, pos.y, pos.z);
	SetRotationPlayingEffekseer3DEffect(playingHandle, 0.0f, rotY, 0.0f);
	SetScalePlayingEffekseer3DEffect(playingHandle, scale, scale, scale);
	return playingHandle;
}

void EffectManager::SetOwnSpeed(int playingHandle, float speed)
{
	m_ownSpeedEffects.push_back({ playingHandle, speed });
}

void EffectManager::Update()
{
	const float timeScale = System::GetInstance().GetTimeScale();

	//再生が終わったエフェクトは外す//IsEffekseer3DEffectPlayingは再生中なら0を返す
	std::erase_if(m_ownSpeedEffects, [](const OwnSpeedEffect& effect)
		{
			return IsEffekseer3DEffectPlaying(effect.playingHandle) != 0;
		});

	//全体はタイムスケールの速さで進むので、タイムスケールで割って自分の速さにする
	for (const auto& effect : m_ownSpeedEffects)
	{
		float speed = effect.speed;
		//タイムスケールが0(ヒットストップなど)のときは0で割らないように、割らずにそのまま使う
		if (timeScale > 0.0f)
		{
			speed /= timeScale;
		}
		SetSpeedPlayingEffekseer3DEffect(effect.playingHandle, speed);
	}

	//再生中のエフェクトを進める//引数は1フレームで進める秒数なので、60fps基準の1/60秒にタイムスケールを掛ける
	UpdateEffekseer3D(timeScale / kEffekseerFps);
}
