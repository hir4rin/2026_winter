#pragma once
#include "PlayerState.h"
#include "PlayerEnums.h"

class Input;
class AttackCol;
//PlaeyrStateAttackとほぼ同じだが、スキルと通常の判別が面倒だったので分けた



class PlayerStateSkillAttack :
    public PlayerState
{
public:
    PlayerStateSkillAttack(std::weak_ptr<Player> player);
    virtual ~PlayerStateSkillAttack();
    void Enter() override;
    void Update() override;
    void Exit() override;
	void DebugDraw()override;
private:
	void AttackMoveMent();//攻撃中の移動処理
	void DetermineAttackDirection();//攻撃の方向を決める関数
	void AttackInputCheck();//攻撃入力をチェックする関数
	void StartCombo(int comboIndex);//コンボを開始する関数//comboIndexは、次のコンボの段数
	void AttackFinishProcess();//攻撃が終了したときの処理//コンボの段数を初期化するなど
	int  SelectAnimInit();//アニメーションの初期化//コンボの段数によってアニメーションを変える
private:
    AttackType m_attackType;//攻撃のタイプ//増えるかは怪しい
	int m_nextComboindex = -1;//次のコンボの段数
	bool m_isComboInputReserved = false;//コンボ入力を受け付けたかどうかのフラグ
	std::shared_ptr<AttackCol> m_attackCol;//攻撃の当たり判定
    
};

