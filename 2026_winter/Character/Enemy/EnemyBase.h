#pragma once
#include "CharacterBase.h"

class Player;

class EnemyBase : public CharacterBase
{
public:
	enum class EnemyState : int
	{
		Idle = 0,
		Caution = 1,
		Chase = 2,
		Attack = 3,
		Back = 4,
		Hit = 5,
		AirStay = 6,
		Fall = 7,
		Dead = 8,
		KnockDown = 9,
	};
	enum class HitType : int
	{
		None = -1,
		Air = 0,
		Ground = 1,
		Drop = 2,
	};

public:
	EnemyBase(std::weak_ptr<Player> player);
	virtual ~EnemyBase();
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	std::string GetEnemyStateString(EnemyState state);

	bool GetIsLifeZero()const { return m_isLifeZero; }//体力が0になったかどうかを返す
protected:
	//ここでやりたいこと
	//・プレイヤーを追いかける
	//・攻撃する
	//・警戒状態//Playerの位置と自分の位置から円を描くように移動
	//・距離を取る//そのまま後ろ側に移動
	std::weak_ptr<Player> m_player;//プレイヤーの弱参照
	/// 追いかけるとなったときのplayerの位置を決める(target)
	/// </summary>
	/// <returns>playerの位置を返す</returns>
	Vector3 TargetPlayerPos();
	/// <summary>
	/// targetの方向にdistanceまで移動する関数
	/// </summary>
	/// <param name="distance">オフセット</param>
	/// <returns>到達したらtrueを返す</returns>
	bool ChasePlayer(Vector3 target,float distance);
	/// <summary>
	/// 半円上を移動、またdistance分の距離は確保する
	/// </summary>
	/// <param name="distance">保つ距離</param>
	void CautionMove(Vector3 target,float distance);
	/// <summary>
	///distance分バックステップをする
	/// </summary>
	/// <param name="distance">バックステップの距離</param>
	/// <returns>distance分離れたらtrueを返す</returns>
	bool BackMove(Vector3 target, float distance);
	bool CanMeleeAttack(float distance);//MeleeAttackができる距離かどうか



	void ToPlayerLook();//Playerの方を向く
	void FinishHitProcess();//Hitの終了処理

	virtual void ChangeState(EnemyState newState) = 0;

	EnemyState m_state = EnemyState::Idle;//敵の状態
	Vector3 m_targetPos;//敵の行動の指標のターゲット

	float m_attackCoolTime = 0.0f;//攻撃のクールタイム
	float m_chasingTime = 0.0f;//追いかけている時間
	float m_cautionTime = 0.0f;//警戒している時間
	float m_idleTime = 0.0f;//待機時間
	float m_knockBackFrame = 0;//吹き飛ばしのフレーム数
	float m_knockBackDownFrame = 0.0f;//吹き飛ばし後のダウン時間
	float m_airCount = 0.0f;//空中にいる時間//AirStayのときに使う
	bool m_isLifeZero = false;//体力が0になったか
	HitType m_hitType = HitType::None;//空中にいるかどうか



};

