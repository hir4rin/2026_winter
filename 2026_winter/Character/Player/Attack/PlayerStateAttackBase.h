#pragma once
#include "PlayerState.h"
#include <memory>
#include "../../../Math/Vector3.h"
class AttackCol;
struct ComboNode;

//通常攻撃(PlayerStateAttack)とスキル攻撃(PlayerStateSkillAttack)で共通の処理をまとめた基底クラス
//攻撃の向きの決定、内部ターゲット、突進、攻撃の当たり判定の生成・削除を持つ
class PlayerStateAttackBase abstract : public PlayerState
{
public:
	PlayerStateAttackBase(std::weak_ptr<Player> player);
	virtual ~PlayerStateAttackBase() = default;
protected:
	//攻撃の方向を決める一連の処理(入力→ロックオン→内部ターゲット)をまとめて行う
	void DecideAttackDirection();
	//攻撃の方向を決める関数
	void DetermineAttackDirection();
	//ロックオン中の攻撃の方向を決める関数
	void LockOnAttackDirection();
	//ロックオンしていないときの攻撃の方向を決める関数
	void NoLockOnAttackDirection();
	//入力方向に敵がいたらそいつをターゲットにする
	void CheckNoLockOnTargetEnemy();

	void AttackMoveMent();//攻撃中の移動処理
	//上下差がある攻撃の初速を保存する//実際に動き出すのはmoveStartFrameから(AttackMoveMent)
	void InitVerticalMove(const ComboNode& node);
	//コンボノードから攻撃データと当たり判定を生成する//最初は無効
	void CreateAttackCol(const ComboNode& node);
	//攻撃の当たり判定を削除する
	void ReleaseAttackCol();
	float ResolveTransitionFrame(float csvFrame, float defaultRate);//CSVのフレームが負の値なら、デフォルトの進行率をフレームに変換して返す
protected:
	//次のコンボ段へ遷移中か//trueのときはExitでコンボ段数をリセットしない
	//同じクラスのステートへ遷移するときだけtrueにする(別クラスへ渡すと前の段のcurrentComboIndexが残ってしまう)
	bool m_isComboTransition = false;

	std::shared_ptr<AttackCol> m_attackCol;//攻撃の当たり判定
	Vector3 m_InitVel = {};//攻撃開始時の速度を保存、上下差のある攻撃のタイムスケールに使う
	float m_gravity = 0.0f;//上下差のある攻撃中の重力の累積
};
