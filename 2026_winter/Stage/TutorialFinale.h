#pragma once
#include <memory>
#include "Math/Vector3.h"

class Stage;
class Player;
class CameraManager;
class EndingCamera;

//チュートリアルの最後//長い縦穴(Kick_Long)で壁キックしたら、そこから上まで自動で壁キックする
//カメラはだんだん遠ざかり、上に着く前に暗転してリザルトへ
//SceneMain がチュートリアルのときだけ作る
class TutorialFinale
{
public:
	TutorialFinale(std::weak_ptr<Stage> stage, std::weak_ptr<Player> player, std::weak_ptr<CameraManager> cameraManager);
	~TutorialFinale();

	void Update();

private:
	enum class Phase
	{
		Wait,//縦穴で壁キックするのを待つ
		Climb,//自動で登る・カメラが遠ざかる
		Done,
	};

	void Start();
	std::shared_ptr<EndingCamera> GetCamera()const;
	/// <summary>
	/// カメラが後ろへ引ける距離(縦穴の中心から)。後ろの当たり判定の手前まで
	/// </summary>
	/// <param name="stage">ステージ</param>
	/// <param name="center">縦穴の中心</param>
	/// <param name="back">カメラが引く向き</param>
	/// <param name="fromDist">調べ始める距離(縦穴の端)</param>
	/// <param name="wantDist">引きたい距離</param>
	/// <param name="lowY">調べる一番下の高さ</param>
	/// <returns>引ける距離</returns>
	float CalcPullLimit(const Stage& stage, const Vector3& center, const Vector3& back, float fromDist, float wantDist, float lowY)const;

private:
	std::weak_ptr<Stage> m_stage;
	std::weak_ptr<Player> m_player;
	std::weak_ptr<CameraManager> m_cameraManager;

	Phase m_phase = Phase::Wait;
	int m_frame = 0;
	float m_topY = 0.0f;//縦穴の上の高さ
	int m_blackFrame = -1;//暗くなり始めてからのフレーム(-1ならまだ)
};
