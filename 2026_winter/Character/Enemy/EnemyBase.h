#pragma once
#include "Character/CharacterBase.h"
#include "Character/Enemy/State/Locomotion/EnemyIdle.h"
#include "Character/Enemy/State/Locomotion/EnemyChase.h"
#include "Character/Enemy/State/Defence/EnemyGuard.h"
#include "Character/Enemy/State/Defence/EnemyGuardBreak.h"
#include "Character/Enemy/State/Defence/EnemyParry.h"
#include "State/Attack/EnemyAttack.h"
#include "State/Attack/EnemyAttackPre.h"
#include "State/Attack/EnemyAttackEnd.h"
#include "Character/Enemy/State/Locomotion/EnemyBack.h"
#include "Character/Enemy/State/Locomotion/EnemyCaution.h"
#include "Character/Enemy/State/Locomotion/EnemyPatrol.h"
#include "State/Hit/EnemyKnockBack.h"
#include "State/Hit/EnemyKnockDown.h"
#include "Character/Enemy/State/Hit/EnemyAirStay.h"
#include "Character/Enemy/State/Hit/EnemyAirFall.h"
#include "State/Hit/EnemyHitDrop.h"
#include "State/Hit/EnemyDie.h"
#include "State/Hit/EnemyHitGround.h"
#include "State/Hit/EnemyHitAir.h"
#include "State/Hit/EnemyAssasined.h"
#include "State/Hit/EnemyPartBrokenKilled.h"
#include "State/Hit/EnemyVanish.h"
#include "Main/Game.h"
#include "Stage/Phase/EnemySpawnData.h"


class Player;
class EnemyStateBase;

/// <summary>
/// 敵の攻撃1つ分のデータ//EnemySwordmanAttack.csvの1行
/// 時間・フレームの値は、負の値なら今までのコードの値(進行度など)を使う
/// </summary>
struct EnemyAttackNode
{
	std::string name;//攻撃の名前(デバッグ用)
	int index = -1;//攻撃の番号

	//予備動作(EnemyAttackPre)
	std::string preAnimName;//予備動作のアニメーション(EnemySwordmanAnim.csvのkey)
	float preAnimTimeScale = 1.0f;//予備動作のアニメーションの再生速度
	float preAnimEndFrame = -1.0f;//予備動作のアニメーションを止めるフレーム//負の値なら最後まで
	float preBlendFrame = -1.0f;//予備動作のブレンドのフレーム数//負の値ならデフォルト(20F)
	float preTime = 0.0f;//予備動作の長さ(フレーム)//これがたったら攻撃へ
	float preLookTime = 0.0f;//プレイヤーの方を向き続けるフレーム//これ以降は向きを固定する

	//攻撃(EnemyAttack)
	std::string animName;//攻撃のアニメーション(EnemySwordmanAnim.csvのkey)
	float animTimeScale = 1.0f;//攻撃のアニメーションの再生速度
	float animEndFrame = -1.0f;//攻撃のアニメーションを止めるフレーム//負の値なら最後まで
	float attackFinishFrame = -1.0f;//攻撃を終えて後隙へ移るフレーム//負の値ならアニメの最後まで
	float blendFrame = -1.0f;//ブレンドのフレーム数//負の値ならデフォルト(20F)
	float attackColStartFrame = -1.0f;//当たり判定ON開始(フレーム)//負の値なら進行度0.3
	float attackColEndFrame = -1.0f;//当たり判定ON終了(フレーム)//負の値なら進行度0.5
	float moveSpeedBeforeCol = 0.0f;//判定ONまでの踏み込み速度
	float moveSpeedDuringCol = 0.0f;//判定中の踏み込み速度//判定が終わったら止まる
	float attackRange = 0.0f;//この距離に入ったら攻撃を選ぶ

	//攻撃のデータ(AttackDataに渡す)
	float attackPower = 0.0f;//攻撃力
	float brokenRate = 0.0f;//部位破壊率(%)
	float knockBackXZ = 0.0f;//吹き飛ばし(横)
	float knockBackY = 0.0f;//吹き飛ばし(縦)
	bool isKirimomi = false;//キリモミ吹っ飛びするかどうか
	float hitStopTime = 0.0f;//ヒットストップ(秒)

	//当たり判定
	float colRadius = 0.0f;//判定の半径
	float colForwardOffset = 0.0f;//判定を前に出す距離(AttackCol::ApplyPosでさらに前に出る)
	float colOffsetY = 0.0f;//判定の高さ

	//攻撃の後
	float attackEndTimeFrame = 0.0f;//後隙(EnemyAttackEnd)の長さ(フレーム)
	float coolTime = 0.0f;//攻撃後のクールタイム(フレーム)

	//SE
	std::string seName;//SEの名前//空ならなし
	float seFrameRate = -1.0f;//SEを鳴らす進行度//負の値なら鳴らさない

	int nextAttack = -1;//連撃するときの次のindex//負の値なら連撃しない
};

//敵の攻撃CSVの列番号
//ComboNodeTypeと名前(Noneなど)がぶつからないように、enumを構造体の中に入れている
struct EnemyAttackNodeType
{
	enum : int
	{
		Name = 0,
		Index,
		PreAnimName,
		PreAnimTimeScale,
		PreAnimEndFrame,
		PreBlendFrame,
		PreTime,
		PreLookTime,
		AnimName,
		AnimTimeScale,
		AnimEndFrame,
		AttackFinishFrame,
		BlendFrame,
		AttackColStartFrame,
		AttackColEndFrame,
		MoveSpeedBeforeCol,
		MoveSpeedDuringCol,
		AttackRange,
		AttackPower,
		BrokenRate,
		KnockBackXZ,
		KnockBackY,
		IsKirimomi,
		HitStopTime,
		ColRadius,
		ColForwardOffset,
		ColOffsetY,
		AttackEndTimeFrame,
		CoolTime,
		SeName,
		SeFrameRate,
		NextAttack,
		Size,//列数
	};
};

/// <summary>
/// 敵の種類ごとのステータス//EnemyStatus.csvの1行
/// </summary>
struct EnemyStatus
{
	std::string type;//敵の種類(Stage〜.csvのtypeと同じ名前)

	//基本
	int hp = 0;//体力
	float moveSpeed = 0.0f;//追いかける速度
	float backSpeed = 0.0f;//距離を取るときの速度
	float patrolSpeed = 0.0f;//巡回の速度
	float cautionSpeed = 0.0f;//警戒移動(回り込み)の速度

	//当たり判定
	float bodyColOffsetY = 0.0f;//体のカプセルの足のオフセット
	float bodyColHeight = 0.0f;//体のカプセルの頭の高さ
	float bodyColRadius = 0.0f;//体のカプセルの半径
	float hitColOffsetY = 0.0f;//やられ判定の高さ
	float hitColRadius = 0.0f;//やられ判定の半径

	//ガード
	float guardRate = 0.0f;//弱攻撃をガードする確率(%)
	float guardTime = 0.0f;//ガードを続ける時間(フレーム)
	float parryGauge = 0.0f;//弾きになるガードゲージの量
	float parryTime = 0.0f;//弾きの長さ(フレーム)
	float guardBreakTime = 0.0f;//ガードブレイクの長さ(フレーム)
	int forceHitCount = 0;//被弾後にガードせず必ず当たる弱攻撃の数
	float forceHitResetTime = 0.0f;//確定ヒットが解除されるまでの時間(フレーム)

	//部位破壊
	float partBreakRate = 1.0f;//被部位破壊率(倍率)//攻撃の部位破壊率に掛ける

	//AIの判断
	float backDistance = 0.0f;//攻撃後に距離を取る距離
	float chaseRate = 0.0f;//Idleの後にChaseを選ぶ確率(%)//残りはCaution
	float idleTime = 0.0f;//Idleの最大時間(フレーム)
	float cautionTime = 0.0f;//Cautionの最大時間(フレーム)
	float chaseTargetUpdateTime = 0.0f;//追跡中にターゲットを更新する間隔(フレーム)

	//索敵
	float viewLength = 0.0f;//視界の長さ
	float viewHalfAngleDeg = 0.0f;//視野の半分の角度(度)

	//被弾
	float knockDownTime = 0.0f;//ダウンしている時間(フレーム)
	float knockBackTime = 0.0f;//吹っ飛び後の時間(フレーム)

	//見た目(部位破壊)
	int leftArmFrame = -1;//左腕のボーン番号
	int headFrame = -1;//頭のボーン番号
	float leftArmModelScale = 1.0f;//左腕パーツモデルのスケール補正
	float headModelScale = 1.0f;//頭パーツモデルのスケール補正
};

//EnemyStatus.csvの列番号
struct EnemyStatusType
{
	enum : int
	{
		Type = 0,
		Hp,
		MoveSpeed,
		BackSpeed,
		PatrolSpeed,
		CautionSpeed,
		BodyColOffsetY,
		BodyColHeight,
		BodyColRadius,
		HitColOffsetY,
		HitColRadius,
		GuardRate,
		GuardTime,
		ParryGauge,
		ParryTime,
		GuardBreakTime,
		ForceHitCount,
		ForceHitResetTime,
		PartBreakRate,
		BackDistance,
		ChaseRate,
		IdleTime,
		CautionTime,
		ChaseTargetUpdateTime,
		ViewLength,
		ViewHalfAngleDeg,
		KnockDownTime,
		KnockBackTime,
		LeftArmFrame,
		HeadFrame,
		LeftArmModelScale,
		HeadModelScale,
		Size,//列数
	};
};

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

	struct PatrolPoint
	{
		Vector3 pos;
		float waitTime = 0.0f;//このポイントにたどり着いてから、次のポイントへ出発するまでの待機フレーム数//0なら待たずに出発、負の値ならずっと待機
		int index = -1;//パトロールポイントのインデックス//EnemyManagerで管理するために使う
	};

	struct GuardInfo
	{
		float GurardGauge = 0.0f;
		float GuardTimer = 0.0f;
	};

public:
	EnemyBase(std::weak_ptr<Player> player);
	virtual ~EnemyBase();
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	//デバッグ描画//頭の上にステート名・HP・クールタイム、足元に向きと踏み込み先の線を出す
	//ステートごとの3D描画(巡回経路など)もここから呼ぶ//文字を出すので、3D・UIを描き終わった後に呼ぶこと
	virtual void DebugDraw();
	bool GetIsLifeZero()const { return m_isLifeZero; }//体力が0になったかどうかを返す

	void OnCollision(Collider& other)override;
	void OnDamage(Collider& other, AttackData& data)override;

	virtual void OnAssasined();//暗殺確定時
	virtual void OnPartBrokenKilled(PartBrokenPattern pattern);//確殺確定時
	bool GetIsExecuted()const { return m_isExecuted; }//確殺・暗殺が始まったかどうか//trueなら確殺・暗殺の対象にしない
	bool GetIsPlayerFound()const { return m_isPlayerFound; }//プレイヤーを発見したかどうか//trueなら暗殺の対象にしない

	//部位破壊
	virtual void OnHeadBreak() {};
	virtual void OnPartBreak() { m_isPartBroken = true; };//攻撃で部位破壊したとき

	//部位破壊したかどうか
	bool GetIsPartBroken() { return m_isPartBroken; }

	//頭を敵のほうに向かせる
	void UpdateLookAtPlayer(int headFrame);

	//PatrollStateに設定
	void SetPatrolState() { ChangeState(std::make_shared<EnemyPatrol>(GetSharedPtr())); }

	std::shared_ptr<EnemyStateBase> GetCurrentState() { return m_currentState; }

	//攻撃のデータ(EnemySwordmanAttack.csvなど)//indexから攻撃1つ分のデータを取得する
	const EnemyAttackNode& GetAttackNode(int index)const { return m_attackList[index]; }
	//種類ごとのステータス(EnemyStatus.csv)
	const EnemyStatus& GetStatus()const { return m_status; }
	

	//死体の消去
	void StartVanish();//死体を消え始めさせる
	bool GetIsVanishing()const { return m_isVanishing; }//消えている途中かどうか
	bool GetIsVanished()const { return m_isVanished; }//消え終わったかどうか//trueならEnemyManagerが削除する
	virtual void SetOpacity(float rate);//モデルの不透明度を設定する(1.0で不透明、0.0で透明)

	//巡回ルートの設定//routeIdはPatrolRouteX.csvのXの番号
	void SetPatrolRoute(int routeId);

	//敵配置CSV(EnemySpawnData)から設定する用----------------------------------------
	//向きを設定する//rotYDegは0で+Zを向く(Unityと同じ)
	void SetFacing(float rotYDeg);
	//巡回ルートを直接設定する
	void SetPatrolPoints(const std::vector<EnemySpawnRoutePoint>& route);
	//その場で見張る//今いる位置に、ずっと待機する巡回ポイントを1つだけ置く(視界でプレイヤーを探す処理を巡回と共通にするため)
	void SetGuardPoint();
	//グループ(同じgroupIdの敵)
	void SetGroupId(const std::string& groupId) { m_groupId = groupId; }
	const std::string& GetGroupId()const { return m_groupId;}
	//同じグループの敵がプレイヤーを見つけたときに呼ばれる//巡回・見張り中なら追跡を始める
	void OnAlerted();
	//------------------------------------------------------------------------------

	//デバッグ用//trueの間はIdleのままにする//被弾のStateには遷移するが、IdleからChaseなどには遷移しない
	void SetDebugIdle(bool isDebugIdle);



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

	virtual int SelectAttackIndex();//今の距離で出せる攻撃のindexを返す(無ければ-1)
	float GetMaxAttackRange()const;//一番長いattackRange(Chase・Cautionの距離)

	//intervalごとにtrueを返す関数
	bool CountInterval(float& timer, float interval);


	/// <summary>
	/// targetの方向にdistanceまで移動する関数
	/// </summary>
	/// <param name="distance">オフセット</param>
	/// <returns>到達したらtrueを返す</returns>
	bool ChaseTarget(Vector3 target, float distance,float speed);
	/// <summary>
	/// 半円上を移動、またdistance分の距離は確保する
	/// </summary>
	/// <param name="distance">保つ距離</param>
	void CautionMove(Vector3 target, float distance);


	void ToPlayerLook();//Playerの方を向く
	void FinishHitProcess();//Hitの終了処理
	void PlayBloodSplash(Collider& other);//血しぶきを再生する//otherは当たった攻撃の判定
	//確殺、暗殺後のenemyの状態制御
	void FinisherPerformanceProcess();//確殺、暗殺後のenemyの状態制御

	//判断のState遷移------------------------------------------------------------
	//「次に何をするか」を決めるところだけを敵側の仮想関数にしている(敵の種類ごとに上書きできる)
	//打ち上げ→落下→着地のような、どの敵でも同じ流れの遷移は各Stateの中に直接書いたまま
	//ここの初期の遷移先は、仮想関数にする前の各Stateの遷移と同じ
	virtual std::shared_ptr<EnemyStateBase> NextAfterIdle();//Idleの待機時間が終わった
	virtual std::shared_ptr<EnemyStateBase> NextOnArrived();//Chaseで攻撃距離に入った、または目的地に着いた
	virtual std::shared_ptr<EnemyStateBase> NextAfterCaution();//Cautionの警戒時間が終わった
	virtual std::shared_ptr<EnemyStateBase> NextAfterAttack();//攻撃のモーションが終わった
	virtual std::shared_ptr<EnemyStateBase> NextAfterAttackEnd();//攻撃の後隙が終わった
	virtual std::shared_ptr<EnemyStateBase> NextAfterParry();//弾きが終わった
	virtual std::shared_ptr<EnemyStateBase> NextOnPlayerFound();//巡回・見張り中にプレイヤーを見つけた(グループの仲間が見つけたときも)
	virtual std::shared_ptr<EnemyStateBase> NextAfterRecover();//被弾・ダウン・着地・ガード・ガードブレイクから立ち直った
	//----------------------------------------------------------------------------

	void ChangeState(std::shared_ptr<EnemyStateBase> newState);//状態遷移用
	//攻撃の流れ(予備動作・攻撃・後隙)のステートか
	bool IsAttackFlowState(const std::shared_ptr<EnemyStateBase>& state)const;

	//攻撃CSVの生データ(DataManagerで読み込んだもの)からm_attackListを作る//各敵のコンストラクタで呼ぶ
	void InitializeAttackList(const std::vector<std::vector<std::string>>& rawData);
	//ステータスの生データからtypeの行を探してm_statusを作る//各敵のコンストラクタで呼ぶ
	void InitializeStatus(const std::vector<std::vector<std::string>>& rawData, const std::string& type);

private:
	//ステートの文字色
	unsigned int GetDebugCategoryColor(EnemyStateBase::DebugCategory category);

	//影付きで中央に文字を描く
	void DrawDebugTextCenter(int x, int y, const std::string& text, unsigned int color);

protected:
	std::shared_ptr<EnemyStateBase> m_currentState;
	std::shared_ptr<EnemyStateBase> m_prevState;

	Vector3 m_targetPos;//敵の行動の指標のターゲット

	float m_attackCoolTime = 0.0f;//攻撃のクールタイム
	int m_forceHitCount = 0;//残りの確定ヒット数
	float m_forceHitTimer = 0.0f;//最後に被弾してからの時間
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

	//巡回ルートのデータ
	std::vector<PatrolPoint> m_patrolPoints;
	int m_currentPatrolIndex = -1;//現在の巡回ポイントのインデックス
	bool m_isPlayerFound = false;//プレイヤーを発見したか(一度見つけたらtrueのまま)
	std::string m_groupId;//同じグループの敵がプレイヤーを見つけたら一緒に気づく//空ならグループ無し


	//ガード
	GuardInfo m_guardInfo;

	//攻撃
	std::vector<EnemyAttackNode> m_attackList;//攻撃のデータ//要素の番号=攻撃のindex//各敵のコンストラクタでDataManagerから入れる
	int m_currentAttackIndex = 0;//今出している攻撃のindex//AttackPreで決めて、Attack・AttackEndでも同じものを使う

	//ステータス
	EnemyStatus m_status;//種類ごとのステータス//各敵のコンストラクタでInitializeStatusを呼んで入れる

	
	//頭
	bool m_isLookAtPlayer = false;//頭をプレイヤーの方に向けるか

	bool m_isDebugIdle = false;//デバッグ用//trueならIdleから他のStateに遷移しない



	friend class EnemyIdle;
	friend class EnemyChase;
	friend class EnemyAttack;
	friend class EnemyAttackPre;
	friend class EnemyAttackEnd;
	friend class EnemyBack;
	friend class EnemyCaution;
	friend class EnemyPatrol;
	friend class EnemyGuard;
	friend class EnemyGuardBreak;
	friend class EnemyParry;
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



