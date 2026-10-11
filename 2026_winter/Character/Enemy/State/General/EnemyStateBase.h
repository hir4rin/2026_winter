#pragma once
#include<memory>
#include<string>

class EnemyBase;

class EnemyStateBase abstract
{
public:
	//デバッグ表示の文字色を分けるためのステートの種類
	enum class DebugCategory
	{
		Move,//待機・移動(白)
		AttackPre,//予備動作(オレンジ)
		Attack,//攻撃(赤)
		Recovery,//攻撃の後隙(緑)//プレイヤーが反撃できる時間
		Hit,//被弾・空中(黄)
		Guard,//ガード(水色)
		Dead,//死亡・処刑(灰)
	};

public:
	EnemyStateBase(std::weak_ptr<EnemyBase> owner);
	virtual ~EnemyStateBase() = default;

	virtual void Enter() = 0;//状態に入るときの処理

	virtual void Update() = 0;//状態の更新処理

	virtual void Exit() = 0;//状態から出るときの処理

	virtual void DebugDraw() {};//3Dのデバッグ描画(経路や視界など)//必要な状態でオーバーライドする//文字はEnemyBase::DebugDrawが頭の上に出す

	virtual const char* GetName()const = 0;//デバッグ表示用のステート名
	virtual DebugCategory GetDebugCategory()const { return DebugCategory::Move; }//デバッグ表示の文字色
	virtual std::string GetDebugInfo()const { return {}; }//ステート名の横に出す情報(タイマーなど)//必要な状態でオーバーライドする

protected:
	//状態を持つ親への弱参照
	std::weak_ptr<EnemyBase> m_owner;


};

