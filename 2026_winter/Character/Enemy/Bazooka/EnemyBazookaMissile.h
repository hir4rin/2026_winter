#pragma once
#include "Math/Vector3.h"
#include <memory>

class CharacterBase;
class FixedAttackCol;
class EnemyBazookaMissileCol;

//敵が撃つミサイル
//着弾点まで割合で進み、横と縦をsinで揺らす//途中でプレイヤーや物に触れたら爆発
class EnemyBazookaMissile
{
public:
	enum class Phase
	{
		Fly,//飛んでいる
		Explode,//爆発中//判定が出ている
		Finished,//終わり//一覧から消してもらう
	};

	EnemyBazookaMissile(std::weak_ptr<CharacterBase> owner, const Vector3& startPos, const Vector3& targetPos);
	~EnemyBazookaMissile();

	void Init();
	void Update();
	bool IsFinished()const { return m_phase == Phase::Finished; }

private:
	void UpdateFly();
	void UpdateFlyEffect();//弾のエフェクトを位置と進む向きに合わせる
	Vector3 CalcFlyPos(float rate)const;//割合のときの位置
	void Explode(const Vector3& pos);//その場で爆発
	void CreateExplodeCol();//爆発の攻撃判定を出す

private:
	std::weak_ptr<CharacterBase> m_owner;//撃った敵//攻撃判定の持ち主
	std::shared_ptr<EnemyBazookaMissileCol> m_flyCol;//飛んでいる間の判定//触れたら爆発
	std::shared_ptr<FixedAttackCol> m_attackCol;//爆発の攻撃判定//CollisionManagerはweak_ptrで持つのでここで持つ
	Phase m_phase = Phase::Fly;
	Vector3 m_pos;//ミサイルの位置
	Vector3 m_prevPos;//前のフレームの位置//ステージとの当たりと向きに使う
	Vector3 m_startPos;//発射位置
	Vector3 m_targetPos;//着弾位置
	float m_rate = 0.0f;//進んだ割合(0〜1)
	float m_distance = 0.0f;//発射位置から着弾位置までの距離
	float m_timer = 0.0f;//爆発の経過時間

	//sinの揺れ
	Vector3 m_right;//横の向き
	Vector3 m_up;//縦の向き
	float m_sideWidth = 0.0f;//横の揺れの幅//符号で左右
	float m_upWidth = 0.0f;//縦の揺れの幅

	int m_warningEffect = -1;//着弾点の予告
	int m_flyEffect = -1;//弾(本体・炎・煙)
};
