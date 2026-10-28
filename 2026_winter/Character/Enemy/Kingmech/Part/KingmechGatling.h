#pragma once
#include "DxLib.h"
#include "Math/Vector3.h"
#include <memory>

class BossEnemyKingmech;

//Kingmechの右腕のガトリング//ガードブレイクの切り上げで切り落とされ、プレイヤーの手で回って投げ返されて爆発する
//動きは演出で決まっているので当たり判定は持たない(爆発はボスとの距離で見る)
class KingmechGatling
{
public:
	enum class Phase
	{
		Attached,//ボスの腕に付いている(ボスのメッシュを描く)
		Cut,//切り落とされて跳ね上がり、プレイヤーの手へ寄っていく
		Hold,//プレイヤーの手で回っている
		Thrown,//ボスへ飛んでいく
		Gone,//爆発して無くなった
	};

public:
	KingmechGatling(std::weak_ptr<BossEnemyKingmech> owner);
	virtual ~KingmechGatling();

	void Update();
	void Draw();

	void Cut();//切り落とす
	void Throw();//ボスへ投げる//Cut・Holdのときだけ

	Phase GetPhase()const { return m_phase; }

private:
	void UpdateCut();
	void UpdateHold();
	void UpdateThrown();
	void Explode();

	Vector3 GetHandPos()const;//プレイヤーの手の位置
	Vector3 GetTargetPos()const;//投げる先(ボスの体)

private:
	std::weak_ptr<BossEnemyKingmech> m_owner;//持ち主のボス
	int m_modelHandle = -1;//ガトリングのモデル(複製)
	int m_bossMeshIndex = -1;//ボスのモデルのガトリングのメッシュ
	int m_bossArmFrame = -1;//ガトリングが付いているボスの骨(lowerarm_r)
	int m_bossTargetFrame = -1;//投げる先のボスの骨

	Phase m_phase = Phase::Attached;
	MATRIX m_baseRot = {};//切り落とした瞬間の向き・大きさ(移動なし)
	Vector3 m_pos;//中心の位置
	Vector3 m_vel;//速度
	float m_spinAngle = 0.0f;//回転角度
	float m_frame = 0.0f;//今のフェーズの経過時間
	Vector3 m_cutPos;//手へ寄せ始めたときの位置(Cut)
};
