#pragma once
#include "PlayerStateAttackBase.h"

//専用必殺技(単発)
//必殺技状態(強化状態)中にもう一度必殺技ボタン(LT)を押すと出る//出し終わったら必殺技状態は終わり
//攻撃の向きの決定と当たり判定の削除はPlayerStateAttackBaseのものを使う
//入口:Player::Update(必殺技状態中のLT)//地上のIdle/Move/攻撃系/回避から
//出口:Idle/Move/Fall
class PlayerStateUlt : public PlayerStateAttackBase
{
public:
	PlayerStateUlt(std::weak_ptr<Player> player);
	virtual ~PlayerStateUlt();

	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;
	bool IsInvincible()const override { return true; }//無敵
private:
	void CreateUltAttackCol();//必殺技の当たり判定を生成する//最初は無効
	void UpdateUltAttackCol();//アニメーションの進行率で当たり判定をON/OFFする
	void EffectCheck();//エフェクトを出すタイミング//出ていたらプレイヤーに追従させる

	//必殺技の段階//Start→Loop→Attackの順に進む
	enum class Phase
	{
		Start,//構え始め
		Loop,//構えたまま溜める
		Attack,//必殺技本体(UltAttack)
	};
	void ChangePhase(Phase phase);//段階を切り替えてアニメーションを流す//モデルに無いアニメーションの段階は飛ばす
	void UpdateAttack();//Attackの段階の更新
private:
	Phase m_phase = Phase::Start;//今の段階
	int m_loopFrame = 0;//Loopの段階に入ってからのフレーム数

	bool m_isTriggeredEffect = false;//エフェクトを出したかどうか

	bool m_isTriggeredStopAnim = false;
};
