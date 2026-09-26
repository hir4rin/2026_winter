#pragma once
#include "../Collider/Collider.h"
#include "../Animation.h"
#include <unordered_map>

class HitCol;

class CharacterBase : public Collider
{
public:
	//防御力//やられに移行するかに使うよう
	enum class Armor
	{
		None = -1,
		Low,
		Middle,
		High,
	};
	//攻撃力//やられに移行させるかに使うよう
	enum class AttackDamagePower
	{
		None = -1,
		Low,
		Middle,
		High,
	};

	//攻撃パラメーター
	struct AttackData
	{
		float attackPower;//攻撃力
		Vector3 knockBackPower;//攻撃が当たったときの吹き飛ばしの力//YがY方向の吹き飛ばしの力、Xが水平面での吹き飛ばしの力
		float knockBackFrame;//攻撃が当たった時の吹き飛ばす時間の割合
		float hitStopTime;//攻撃が当たったときのヒットストップの時間
		float kAttackColOffset;//攻撃判定を前に出す距離
		bool isKirimomi;//吹っ飛ぶかどうか
	};

	//被ダメ情報
	struct HitInfo
	{
		Vector3 knockBackVel;
		float duration = 0.0f;
		bool isKirimomi = false;
	};


public:
    CharacterBase();
    virtual ~CharacterBase();
    //キャラクターの更新処理
    virtual void Update() = 0;
    //キャラクターの描画処理
	virtual void Draw() = 0;

	/// <summary>
	/// やられ判定の生成を行う
	/// </summary>
	/// <param name="owner"></param>
	void InitHitCol(std::weak_ptr<CharacterBase> owner);//やられ判定の初期化//継承先で呼ぶ

	Vector3 GetTargetVec() const { return m_targetVec; }

	int GetModelHandle() const { return m_modelHandle; }
	Animation& GetAnimation() { return m_anim; }//アニメーションの取得
	Vector3 GetForward() const { return forward; }//前方向のベクトルを返す
	//const AttackData& GetAttackData() { return m_attackData; }//攻撃データの取得
	virtual void OnDamage(Collider& other, AttackData& data) = 0;//ダメージを受けた時の処理

	bool GetIsDead()const { return m_isDead; }//死んでいるかどうかのフラグを返す
	std::shared_ptr<HitCol> GetHitCol() { return m_hitCol; }//やられ判定の取得

	//向き(Y軸回転)を直接セットする//演出などで座標と一緒に向きも固定したいときに使う
	void SetRotY(float angleY) { m_rotAngleY = angleY; }

	int GetHp()const { return m_hp; }//体力の取得

	void OnTriggerEnter(Collider& other)override;
	void OnTriggerExit(Collider& other)override;

protected:
	void UpdateAngleAndPos();//回転角度と座標の更新//
	void ApplyPos()override;

	int m_modelHandle = -1;//モデルのハンドル
	int m_hp = -1;//体力
	float m_rotAngleY = 0.0f;//回転角度
	float m_rotAngleX = 0.0f;//回転角度
	float m_targetAngleY = 0.0f;//目標の回転角度//回転を滑らかにするためのもの
	float m_accumulatedGravity = 0.0f;//重力の累積値


	Vector3 m_targetVec = {};//移動したい方向のベクトル
	//移動方向//カメラの向き
	Vector3 forward = Vector3(0, 0, 0);
	Vector3 down = Vector3(0, 0, 0);
	Vector3 left = Vector3(0, 0, 0);
	Vector3 right = Vector3(0, 0, 0);

	Animation m_anim;//アニメーション(アニメーションブレンド、アニメーション再生など)//今回はフレームで管理するように後で修正がいる
	std::unordered_map<std::string, std::string> m_animNames;//アニメーションの名前を管理
	const std::string& GetAnimName(const std::string& key)const;//アニメーションの名前を取得する

	std::shared_ptr<HitCol> m_hitCol;//やられ判定


	Armor m_armor = Armor::None;//アーマー
	AttackDamagePower m_attackPower = AttackDamagePower::None;//攻撃力
	AttackData m_attackData = {};//攻撃時はデータを入れて渡し、被弾時はデータを受け取る

	bool m_isGround = true;//地面にいるかどうか
	bool m_isHit = false;//攻撃を喰らったか
	bool m_isDead = false;//死んでいるか
	bool m_isDieOut = false;//吹っ飛び途中で消える処理を行うかどうか
	bool m_isAttackHit = false;//攻撃が当たったか

};

