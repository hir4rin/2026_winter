#pragma once
#include <memory>
#include <vector>
#include "Stage.h"

class Player;
class CameraManager;
class EndingCamera;

//列車ステージのエンディング//トンネルを抜けて空が開け、クレジット・ロゴ(タイトルコール)
//ロゴの後に少し歩ける → ミサイルに囲まれる → 爆風の中から飛び出して落ちる → 暗転してリザルト
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
		Walk,//少し歩ける
		Ambush,//ミサイルに囲まれる
		Jump,//爆風の中から飛び出して落ちる・暗転
		Done,
	};

	//ミサイル1発(予告 → 落ちる → 爆発)
	struct AmbushMissile
	{
		Vector3 pos;//着弾点
		int startFrame = 0;//予告を出すフレーム(囲まれてから)
		float scale = 1.0f;//エフェクトの大きさ
		bool isCenter = false;//最後のプレイヤーの足元の1発
		int warning = -1;//予告の円(再生中)
	};

	void ChangePhase(Phase phase);
	//Y長押しで飛ばす
	void UpdateSkip();
	void StartSkip();
	void StartAmbush();
	void UpdateAmbush();
	void UpdateMissiles();
	void StartJump();
	//エフェクトを置いて再生する(1/20で作ったものは scale 20)
	static int PlayEffect(int effect, const Vector3& pos, float scale);
	//プレイヤーの足元から左右へ床の端を探して、跳ぶ向きを決める
	void FindJumpEdge();
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

	//飛び降りる
	Vector3 m_jumpOutDir = Vector3(1.0f, 0.0f, 0.0f);//跳ぶ向き
	float m_edgeDist = 0.0f;//足元から床の端まで
	int m_radioIndex = -1;//無線の字幕
	int m_radioFrame = 0;
	//ミサイル
	std::vector<AmbushMissile> m_missiles;
	int m_missileClock = 0;//囲まれてからのフレーム
	Vector3 m_ambushCenter;//囲まれた場所(プレイヤーの足元)
	int m_warningEffect = -1;
	int m_fallEffect = -1;
	int m_explodeEffect = -1;
	int m_leapBurstEffect = -1;//煙を突き抜ける
	int m_fallTrailEffect = -1;//落ちる間の煙の尾・風の筋
	int m_fallTrailPlaying = -1;

	//スキップ
	int m_skipHoldFrame = 0;//長押ししているフレーム
	int m_skipPromptFrame = 0;//案内を出してからのフレーム
	int m_skipFrame = -1;//飛ばし始めてからのフレーム(-1なら飛ばしていない)
};
