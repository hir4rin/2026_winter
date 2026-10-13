#pragma once
#include "AttackCol.h"

//スキル用の攻撃判定//プレイヤーに追従しないで、SetPosで渡した位置に置く
//地上スキル1はプレイヤーが上昇してるけど、分身は敵のところで斬ってるので判定を分身側に置きたい
//当たった時の処理(ダメージ、ゲージ、SE、isHit)はAttackColのをそのまま使う
class SkillAttackCol :
	public AttackCol
{
public:
	using AttackCol::AttackCol;//コンストラクタはAttackColのまま

	void SetPos(const Vector3& pos) { m_targetPos = pos; }//判定を置く位置//毎フレーム渡す
	//AttackColはプレイヤーの位置で上書きしてくるので、ここだけ差し替える
	//高さはColInitのoffsetで足す
	void ApplyPos() override { m_rb.m_pos = m_targetPos; }
private:
	Vector3 m_targetPos = {};//判定を置く位置
};
