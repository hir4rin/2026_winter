#pragma once
#include "PlayerStateAttackBase.h"
#include "PlayerEnums.h"
#include <memory>
#include "../../../Math/Vector3.h"
class Input;
class AttackCol;
class EnemyBase;

//通常攻撃(弱・強・空中)//スキル攻撃はPlayerStateSkillAttack
class PlayerStateAttack : public PlayerStateAttackBase
{
public:
	PlayerStateAttack(std::weak_ptr<Player> player,AttackType type);
	virtual ~PlayerStateAttack();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;
private:
	//空中攻撃5用//入力方向に地上の敵がいたらそいつをターゲットにする
	void CheckNoLockOnTargetEnemyForAirAttack5();
	//プレイヤーの近くに空中の敵がいるか
	bool SearchNearbyAirEnemy();
	void AttackInputCheck();//攻撃入力をチェックする関数
	void StartCombo(int comboIndex);//コンボを開始する関数//comboIndexは、次のコンボの段数
	void AttackFinishProcess();//攻撃が終了したときの処理//コンボの段数を初期化するなど
	int  SelectAnimInit();//アニメーションの初期化//コンボの段数によってアニメーションを変える
	void InpuctAttackSetUp();//ドロップ攻撃後の吹き飛ばし用

	void SwingSeCheck();//振りのSEを出すタイミング(ComboNodeのseFrameRate/seNameを使う)
private:
	AttackType m_attackType;//攻撃のタイプ//弱攻撃か強攻撃か
	int m_nextComboIndex = -1;//次のコンボの段数
	bool m_isComboInputReserved = false;//コンボ入力を受け付けたかどうかのフラグ
	bool m_isSkillAttackReserved = false;//スキル攻撃の予約がされているかどうかのフラグ

	bool m_isSwingSePlayed = false;//振りのSEを再生したかどうか


};

