#pragma once
#include <memory>
#include "../../../Collider/Collider.h"

class Player;
class Vector3;

class PlayerState abstract
{
public:
	PlayerState(std::weak_ptr<Player> owner);
	virtual ~PlayerState() = default;

	virtual void Enter() = 0;//状態に入るときの処理
	
	virtual void Update() = 0;//状態の更新処理

	virtual void Exit() = 0;//状態から出るときの処理

	virtual void DebugDraw() {};//デバッグ描画//必要な状態でオーバーライドする

	//壁判定のレイ(kWallCheckDistance)のデバッグ描画//当たっていたら緑、当たっていなかったら赤
	void DebugDrawWallCheck();

protected:

	//状態を持つプレイヤーへの弱い参照
	//状態からプレイヤーの情報にアクセスするためのもの
	//循環参照を避けるために弱い参照を使う
	std::weak_ptr<Player> m_owner;

	//カメラと入力方向からPlayerの移動方向を決める
	void HandlerInput();
	//XZ平面の速度制限
	void ClampSpeed();

	//壁と当たったかどうかの判定
	bool CheckWall();
	//次のフレームで壁と当たるかどうかの判定
	bool CheckNextFrameWall();
	//プレイヤーが指定したゾーン(WallKickZone/WallRunZone)の中にいるか
	bool IsInWallZone(Collider::ColRole zoneRole);

	//壁判定のレイの本数(正面・左・右)
	static constexpr int kWallRayNum = 3;
	//壁判定のレイの始点と終点を求める//CheckWallとデバッグ描画で同じものを使う
	//index 0:正面 1:左 2:右
	bool GetWallCheckRay(int index, Vector3& rayStart, Vector3& rayEnd);

};

