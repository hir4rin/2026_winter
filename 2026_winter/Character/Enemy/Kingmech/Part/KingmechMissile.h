#pragma once
#include "Math/Vector3.h"

//ミサイルの雨の1発分//予告の円→落下→爆発をエフェクトで見せる
//今は見た目だけ(爆発の攻撃判定はまだ無い)
class KingmechMissile
{
public:
	enum class Phase
	{
		Warning,//予告の円だけ出ている
		Falling,//上からロケットが落ちてくる
		Explode,//着弾
		Finished,//終わり//一覧から消してもらう
	};

	KingmechMissile(const Vector3& landPos);
	~KingmechMissile();

	void Update();
	bool IsFinished()const { return m_phase == Phase::Finished; }

private:
	void ChangePhase(Phase next);

	Phase m_phase = Phase::Warning;
	float m_timer = 0.0f;//今の段階の経過時間
	Vector3 m_landPos;//着弾する位置(地面の高さ)
	int m_warningEffect = -1;//予告の円
};
