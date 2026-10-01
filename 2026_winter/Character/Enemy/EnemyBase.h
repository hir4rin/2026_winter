#pragma once
#include "CharacterBase.h"
#include "State/General/EnemyIdle.h"
#include "State/General/EnemyChase.h"
#include "State/Attack/EnemyAttack.h"
#include "State/General/EnemyBack.h"
#include "State/General/EnemyCaution.h"
#include "State/Hit/EnemyKnockBack.h"
#include "State/Hit/EnemyKnockDown.h"
#include "State/General/EnemyAirStay.h"
#include "State/General/EnemyAirFall.h"
#include "State/Hit/EnemyHitDrop.h"
#include "State/Hit/EnemyDie.h"
#include "State/Hit/EnemyHitGround.h"
#include "State/Hit/EnemyHitAir.h"
#include "State/Hit/EnemyAssasined.h"
#include "State/Hit/EnemyPartBrokenKilled.h"
#include "State/Hit/EnemyVanish.h"


class Player;
class EnemyStateBase;

class EnemyBase : public CharacterBase
{
public:
	enum class HitType : int
	{
		None = -1,
		Air = 0,
		Ground = 1,
		Drop = 2,
	};

	//struct HitInfo

public:
	EnemyBase(std::weak_ptr<Player> player);
	virtual ~EnemyBase();
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	bool GetIsLifeZero()const { return m_isLifeZero; }//体力が0になったかどうかを返す

	void OnCollision(Collider& other)override;
	void OnDamage(Collider& other, AttackData& data)override;

	virtual void OnAssasined();//暗殺確定時
	virtual void OnPartBrokenKilled(PartBrokenPattern pattern);//確殺確定時
	bool GetIsExecuted()const { return m_isExecuted; }//確殺・暗殺が始まったかどうか//trueなら確殺・暗殺の対象にしない

	//部位破壊
	virtual void OnHeadBreak() {};
	virtual void OnPartBreak() { m_isPartBroken = true; };//攻撃で部位破壊したとき

	//部位破壊したかどうか
	bool GetIsPartBroken() { return m_isPartBroken; }

	std::shared_ptr<EnemyStateBase> GetCurrentState() { return m_currentState; }

	//死体の消去
	void StartVanish();//死体を消え始めさせる
	bool GetIsVanishing()const { return m_isVanishing; }//消えている途中かどうか
	bool GetIsVanished()const { return m_isVanished; }//消え終わったかどうか//trueならEnemyManagerが削除する
	virtual void SetOpacity(float rate);//モデルの不透明度を設定する(1.0で不透明、0.0で透明)



	std::shared_ptr<EnemyBase> GetSharedPtr() { return std::dynamic_pointer_cast<EnemyBase>(shared_from_this()); }
	std::weak_ptr<EnemyBase> GetWeakPtr() { return GetSharedPtr(); }
protected:
	void ApplyPos()override;//座標の適用//

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
	///distance分バックステップをする
	/// </summary>
	/// <param name="distance">バックステップの距離</param>
	/// <returns>distance分離れたらtrueを返す</returns>
	bool BackMove(Vector3 target, float distance);
	bool CanMeleeAttack(float distance);//MeleeAttackができる距離かどうか

	//intervalごとにtrueを返す関数
	bool CountInterval(float& timer, float interval);


	/// <summary>
	/// targetの方向にdistanceまで移動する関数
	/// </summary>
	/// <param name="distance">オフセット</param>
	/// <returns>到達したらtrueを返す</returns>
	bool ChasePlayer(Vector3 target, float distance);
	/// <summary>
	/// 半円上を移動、またdistance分の距離は確保する
	/// </summary>
	/// <param name="distance">保つ距離</param>
	void CautionMove(Vector3 target, float distance);


	void ToPlayerLook();//Playerの方を向く
	void FinishHitProcess();//Hitの終了処理
	//確殺、暗殺後のenemyの状態制御
	void FinisherPerformanceProcess();//確殺、暗殺後のenemyの状態制御

	//Idleの後のState遷移
	virtual std::shared_ptr<EnemyStateBase> NextAfterIdle();
	//着地後の遷移先

	void ChangeState(std::shared_ptr<EnemyStateBase> newState);//状態遷移用



protected:
	std::shared_ptr<EnemyStateBase> m_currentState;
	std::shared_ptr<EnemyStateBase> m_prevState;

	Vector3 m_targetPos;//敵の行動の指標のターゲット

	float m_attackCoolTime = 0.0f;//攻撃のクールタイム
	float m_chasingTime = 0.0f;//追いかけている時間
	float m_cautionUpdateTimer = 0.0f;
	float m_cautionTime = 0.0f;//警戒している時間
	float m_idleTime = 0.0f;//待機時間
	float m_knockBackFrame = 0;//吹き飛ばしのフレーム数
	float m_knockBackDownFrame = 0.0f;//吹き飛ばし後のダウン時間
	float m_airCount = 0.0f;//空中にいる時間//AirStayのときに使う
	bool m_isLifeZero = false;//体力が0になったか
	HitType m_hitType = HitType::None;//空中にいるかどうか

	//部位破壊したかどうか
	bool m_isPartBroken = false;
	//確殺・暗殺が始まったか(処刑済み)//アニメの終わりを待たずに対象から外すためのもの
	bool m_isExecuted = false;

	//死体の消去
	bool m_isVanishing = false;//消えている途中か
	bool m_isVanished = false;//消え終わったか



	friend class EnemyIdle;
	friend class EnemyChase;
	friend class EnemyAttack;
	friend class EnemyBack;
	friend class EnemyCaution;
	friend class EnemyKnockBack;
	friend class EnemyKnockDown;
	friend class EnemyAirStay;
	friend class EnemyAirFall;
	friend class EnemyHitDrop;
	friend class EnemyDie;
	friend class EnemyHitGround;
	friend class EnemyHitAir;
	friend class EnemyAssasined;
	friend class EnemyPartBrokenKilled;
	friend class EnemyVanish;

};

