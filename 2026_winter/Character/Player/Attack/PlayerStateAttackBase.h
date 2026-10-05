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

	//多段ヒットの処理
	// //今どの区間か見て、区間が変わったらヒット履歴クリア、最後の区間だけノックバック
	//判定を引数でもらうので、m_attackColでもスキル用の判定でも使える//戻り値はどこかの区間に入ってるか
	bool UpdateAttackWindow(AttackCol& col, const ComboNode& node);
	//下降攻撃ver//m_dropAttackCol用//区間が変わったらヒット履歴をクリアするだけ
	//ノックバックは作るときに0にしてるので触らない//敵をEnemyHitDropに入れるのはずっとONのm_attackCol
	bool UpdateDropAttackWindow(AttackCol& col, const ComboNode& node);

	//上下差がある攻撃の初速を保存する//実際に動き出すのはmoveStartFrameから(AttackMoveMent)
	void InitVerticalMove(const ComboNode& node);
	//コンボノードから攻撃データと当たり判定を生成する//最初は無効
	void CreateAttackCol(const ComboNode& node);
	//攻撃の当たり判定を削除する
	void ReleaseAttackCol();
	//ドロップ攻撃の着地時に吹き飛ばしの当たり判定を置く//通常攻撃とスキル攻撃で共通
	void InpuctAttackSetUp();
	float ResolveTransitionFrame(float csvFrame, float defaultRate);//CSVのフレームが負の値なら、デフォルトの進行率をフレームに変換して返す
protected:
	//次のコンボ段へ遷移中か//trueのときはExitでコンボ段数をリセットしない
	//同じクラスのステートへ遷移するときだけtrueにする(別クラスへ渡すと前の段のcurrentComboIndexが残ってしまう)
	bool m_isComboTransition = false;

	std::shared_ptr<AttackCol> m_attackCol;//攻撃の当たり判定
	std::shared_ptr<AttackCol> m_dropAttackCol;//下降攻撃の追加ヒット用の判定//ノックバック0でダメージだけ//下降攻撃以外はnullptr
	int m_lastAttackWindow = -1;//前のフレームで何番目の判定区間にいたか//-1なら判定OFF中//区間が変わったのを見る用
	Vector3 m_InitVel = {};//攻撃開始時の速度を保存、上下差のある攻撃のタイムスケールに使う
	float m_gravity = 0.0f;//上下差のある攻撃中の重力の累積
};
