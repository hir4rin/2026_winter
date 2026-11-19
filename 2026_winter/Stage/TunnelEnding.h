#pragma once
#include <memory>
#include "Stage.h"

class Player;
class CameraManager;
class EndingCamera;

//列車ステージのエンディング//トンネルを抜けて空が開け、クレジット・ロゴを出してリザルトへ
//最後のフェーズの遷移表の event が Ending のときに SceneMain が作る
class TunnelEnding
{
public:
	TunnelEnding(std::weak_ptr<Stage> stage, std::weak_ptr<Player> player, std::weak_ptr<CameraManager> cameraManager);
	~TunnelEnding();

	void Update();

private:
	enum class Phase
	{
		Wait,//倒した余韻
		Approach,//出口が近づく
		WhiteOut,//抜けて霧に包まれる
		Clear,//霧が晴れる
		Gaze,//見上げたまま
		Credit,//カメラが引いて歩ける・クレジット
		Logo,
		Done,
	};

	void ChangePhase(Phase phase);
	void UpdateFog();
	void UpdateScreen();
	std::shared_ptr<EndingCamera> GetCamera()const;
	static Stage::FogSetting LerpFog(const Stage::FogSetting& a, const Stage::FogSetting& b, float t);

private:
	std::weak_ptr<Stage> m_stage;
	std::weak_ptr<Player> m_player;
	std::weak_ptr<CameraManager> m_cameraManager;

	Phase m_phase = Phase::Wait;
	int m_frame = 0;//今のフェーズに入ってからのフレーム(実フレーム)
	float m_exitStartDistance = 1.0f;//出口を置いたときの距離
	Stage::FogSetting m_fromFog;//フェーズに入ったときのフォグ
	Stage::FogSetting m_fog;//今のフォグ
	int m_bgR = 0;//背景の色(フォグと同じく変える)
	int m_bgG = 0;
	int m_bgB = 0;
};
