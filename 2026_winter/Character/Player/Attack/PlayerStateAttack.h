#pragma once
#include "PlayerState.h"
#include "PlayerEnums.h"
#include <memory>
#include "../../../Math/Vector3.h"
class Input;
class AttackCol;
class EnemyBase;

class PlayerStateAttack : public PlayerState
{
public:
	PlayerStateAttack(std::weak_ptr<Player> player,AttackType type);
	virtual ~PlayerStateAttack();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;
private:
	void AttackMoveMent();//攻撃中の移動処理
	//攻撃の方向を決める関数
	void DetermineAttackDirection();
	//ロックオン中の攻撃の方向を決める関数
	void LockOnAttackDirection();
	//ロックオンしていないときの攻撃の方向を決める関数
	void NoLockOnAttackDirection();
	//入力方向に敵がいたらそいつをターゲットにする
	void CheckNoLockOnTargetEnemy();
	void AttackInputCheck();//攻撃入力をチェックする関数
	void StartCombo(int comboIndex);//コンボを開始する関数//comboIndexは、次のコンボの段数
	void AttackFinishProcess();//攻撃が終了したときの処理//コンボの段数を初期化するなど
	int  SelectAnimInit();//アニメーションの初期化//コンボの段数によってアニメーションを変える
	void InpuctAttackSetUp();//ドロップ攻撃後の吹き飛ばし用

	void EffectCheck();//エフェクトを出すタイミング
	void SwingSeCheck();//振りのSEを出すタイミング(ComboNodeのseFrameRate/seNameを使う)
private:
	AttackType m_attackType;//攻撃のタイプ//弱攻撃か強攻撃か
	int m_nextComboIndex = -1;//次のコンボの段数
	bool m_isComboInputReserved = false;//コンボ入力を受け付けたかどうかのフラグ
	bool m_isSkillAttackReserved = false;//スキル攻撃の予約がされているかどうかのフラグ

	bool m_isTriggerdEffec = false;//エフェクトを発生させたかどうか
	bool m_isSwingSePlayed = false;//振りのSEを再生したかどうか

	std::shared_ptr<AttackCol> m_attackCol;//攻撃の当たり判定
	Vector3 m_InitVel = {};//攻撃開始時の速度を保存、上下差のある攻撃のタイムスケールに使う

	//攻撃吸い寄せ用の敵のポインタ
	std::weak_ptr<EnemyBase> m_homingEnemyTarget = {};

	

};

