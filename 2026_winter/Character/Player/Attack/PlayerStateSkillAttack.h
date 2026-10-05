#pragma once
#include "PlayerStateAttackBase.h"
#include "PlayerEnums.h"

class Input;
class AttackCol;
//スキル攻撃(スキル攻撃0〜3)
//地上:スキル0(打ち上げ)→自動でスキル攻撃1  空中:そのままスキル攻撃1
//通常攻撃(PlayerStateAttack)との共通処理はPlayerStateAttackBaseにある
//入口:Idle/Move/Jump/Fallから直接、またはPlayerStateAttackのコンボ中にスキル入力したとき
//出口:スキル攻撃の次の段(同じクラス)、弱・強攻撃(PlayerStateAttack)、回避・ジャンプ・確殺、Idle/Move/Fall
class PlayerStateSkillAttack :
    public PlayerStateAttackBase
{
public:
    PlayerStateSkillAttack(std::weak_ptr<Player> player);
    virtual ~PlayerStateSkillAttack();
    void Enter() override;
    void Update() override;
    void Exit() override;
	void DebugDraw()override;
private:
	void AttackInputCheck();//攻撃入力をチェックする関数
	int  SelectAnimInit();//スキル攻撃の何段目を再生するか決める
	void StartNextSkill(int nextComboIndex);//スキル攻撃の次の段へ移行する//同じクラスなのでコンボ段数を引き継ぐ
	void EffectCheck();//エフェクトを出すタイミング
	void PlayGhostEffect(int comboIndex);//スキル1,2のときモデルを消して分身エフェクトを出す
	void UpdateGhostEffect();//分身エフェクトをプレイヤーの位置・向きに合わせる
	void StopGhostEffect();//分身エフェクトを止めてモデルを表示に戻す
	Vector3 GetSkillEffectBasePos();//スキルのエフェクトを出す基準の位置//スキル0,1は打ち上げた敵→攻撃の対象→プレイヤーの順//それ以外はプレイヤー
	void InitLaunchCamera(int comboIndex);//打ち上げから続くスキル1以外は、カメラの注視点を通常に戻す
	void UpdateLaunchCamera();//スキル0が当たったら、カメラの注視点のYを打ち上げた敵に合わせる(スキル1が終わるまで)
private:
	int m_ghostEffectHandle = -1;//再生中の分身エフェクトのハンドル//-1なら再生していない
	int m_nextComboIndex = -1;//次のスキル攻撃の段数//m_nextAttackTypeがSkillAttackのときだけ使う
	AttackType m_nextAttackType = AttackType::None;//予約された次の攻撃の種類//SkillAttackなら次の段、弱・強ならPlayerStateAttackへ
	bool m_isComboInputReserved = false;//コンボ入力を受け付けたかどうかのフラグ
};

