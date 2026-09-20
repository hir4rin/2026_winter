#pragma once
#include <memory>

class Player;

class PlayerState abstract
{
public:
	PlayerState(std::weak_ptr<Player> owner);
	virtual ~PlayerState() = default;

	virtual void Enter() = 0;//状態に入るときの処理
	
	virtual void Update() = 0;//状態の更新処理

	virtual void Exit() = 0;//状態から出るときの処理

	virtual void DebugDraw() {};//デバッグ描画//必要な状態でオーバーライドする

protected:

	//状態を持つプレイヤーへの弱い参照
	//状態からプレイヤーの情報にアクセスするためのもの
	//循環参照を避けるために弱い参照を使う
	std::weak_ptr<Player> m_owner;

	//カメラと入力方向からPlayerの移動方向を決める
	void HandlerInput();
	//XZ平面の速度制限
	void ClampSpeed();

};

