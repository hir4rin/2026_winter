#pragma once
#include "../Collider/Collider.h"
#include "CharacterBase.h"
#include <memory>
#include <string>
#include <list>
class Player;

class AttackCol :
	public Collider
{
public:
	AttackCol(std::weak_ptr<CharacterBase> owner,const CharacterBase::AttackData& data);
	virtual ~AttackCol();
	void OnCollision(Collider& other) override;
	void ApplyPos() override;

	virtual void Update();

	std::weak_ptr<CharacterBase> GetOwner() { return m_owner; }

	void ClearHitIds() { m_hitIds.clear(); }//当たったIDのリストをクリアする//攻撃が終わったら呼ぶ
	//ノックバックだけあとから変える//多段ヒットで途中は吹っ飛ばさない、最後だけ吹っ飛ばすのに使う
	void SetKnockBack(const Vector3& power, bool isKirimomi)
	{
		m_attackData->knockBackPower = power;
		m_attackData->isKirimomi = isKirimomi;
	}
protected:
	void PlayerAttackOnCollision(Collider& other);//Playerの攻撃が当たった時の処理
	void EnemyAttackOnCollision(Collider& other);//Enemyの攻撃が当たった時の処理

	void PlayerGaugeUp(Collider& other);//Playerの攻撃が当たった時のゲージの上昇
	std::string GetHitSeName(Player& player)const;//現在のコンボインデックスから、蹴り/スキル/剣のヒットSE名を判定する
protected:
	std::weak_ptr<CharacterBase> m_owner;//当たり判定を持つキャラクターへの弱参照
	//当たったidのリスト
	std::list<int> m_hitIds;//攻撃が当たったIDのリスト(重複ヒット防止)
	//AttackDataを保持
	std::shared_ptr<CharacterBase::AttackData> m_attackData;


	int m_hitEfHandle = -1;//ヒットエフェクトのハンドル
	int m_hitEfPlayingHandle = -1;//再生中のヒットエフェクトのハンドル


	
};
