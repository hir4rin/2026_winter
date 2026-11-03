#pragma once
#include "Math/Vector3.h"
#include <memory>

class CharacterBase;
class FixedAttackCol;

//ミサイルの雨の1発分//予告の円→落下→爆発をエフェクトで見せる//爆発の間だけ着弾位置に攻撃判定を出す
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

	KingmechMissile(std::weak_ptr<CharacterBase> owner, const Vector3& landPos);//ownerは撃ったボス
	~KingmechMissile();

	void Update();
	bool IsFinished()const { return m_phase == Phase::Finished; }

private:
	void ChangePhase(Phase next);
	void CreateExplodeCol();//爆発の攻撃判定を出す

	std::weak_ptr<CharacterBase> m_owner;//撃ったボス//攻撃判定の持ち主
	std::shared_ptr<FixedAttackCol> m_explodeCol;//爆発の攻撃判定//CollisionManagerはweak_ptrで持つのでここで持つ
	Phase m_phase = Phase::Warning;
	float m_timer = 0.0f;//今の段階の経過時間
	Vector3 m_landPos;//着弾する位置(地面の高さ)
	int m_warningEffect = -1;//予告の円
};
