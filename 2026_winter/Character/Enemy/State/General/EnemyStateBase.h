#pragma once
#include<memory>

class EnemyBase;

class EnemyStateBase abstract
{
public:
	EnemyStateBase(std::weak_ptr<EnemyBase> owner);
	virtual ~EnemyStateBase() = default;

	virtual void Enter() = 0;//状態に入るときの処理

	virtual void Update() = 0;//状態の更新処理

	virtual void Exit() = 0;//状態から出るときの処理

	virtual void DebugDraw() {};//デバッグ描画//必要な状態でオーバーライドする

protected:
	//状態を持つ親への弱参照
	std::weak_ptr<EnemyBase> m_owner;


};

