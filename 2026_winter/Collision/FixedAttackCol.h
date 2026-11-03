#pragma once
#include "AttackCol.h"

//決めた位置から動かない攻撃判定(ミサイルの着弾など)//AttackColは毎フレーム持ち主の位置へ動くので、ApplyPosだけ変える
class FixedAttackCol :
	public AttackCol
{
public:
	FixedAttackCol(std::weak_ptr<CharacterBase> owner, const CharacterBase::AttackData& data, const Vector3& pos);
	virtual ~FixedAttackCol();

	void ApplyPos() override;
private:
	Vector3 m_fixedPos;//判定の位置
};
