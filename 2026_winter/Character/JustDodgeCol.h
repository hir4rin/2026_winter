#pragma once
#include "../Collider/Collider.h"
#include "CharacterBase.h"
#include <memory>
class Player;

//ジャスト回避判定//PlayerStateDodgeが回避中だけ生成する
//敵の攻撃判定に当たったら、所有者(Player)に通知する
class JustDodgeCol :
	public Collider
{
public:
	JustDodgeCol(std::weak_ptr<Player> owner);
	virtual ~JustDodgeCol();

	void OnCollision(Collider& other) override;//何もしない//通知は攻撃側(AttackCol)から受け取る
	void OnJustDodgeInterFace(Collider& other, CharacterBase::AttackData& data);//敵の攻撃が当たった時の処理//所有者に通知する

	void ApplyPos() override;
protected:
	std::weak_ptr<Player> m_owner;//当たり判定を持つプレイヤーへの弱参照
};
