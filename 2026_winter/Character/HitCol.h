#pragma once
#include "../Collider/Collider.h"
#include "CharacterBase.h"
#include <memory>
class CharacterBase;
struct AttackData;

class HitCol :
    public Collider
{
public:
	HitCol(std::weak_ptr<CharacterBase> owner);
	virtual ~HitCol();

	void OnCollision(Collider& other) override;
	void OnDamageInterFace(Collider& other, CharacterBase::AttackData& data);//ダメージを受けた時の処理//HitColは何もしない
	void SetTimeScaleInterFace(float timeScale, float time);//タイムスケールのセット//HitColは何もしない

	void ApplyPos() override;
protected:
	std::weak_ptr<CharacterBase> m_owner;//当たり判定を持つキャラクターへの弱参照
};

