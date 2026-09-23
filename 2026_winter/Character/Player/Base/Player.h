#pragma once
#include "../../CharacterBase.h"
#include "PlayerEnums.h"
#include "PlayerStateIdle.h"//他のStateから他のStateに遷移するため(便利)
#include "PlayerStateMove.h"//以下同文
#include "PlayerStateJump.h"//以下同文
#include "PlayerStateFall.h"//以下同文
#include "PlayerStateHit.h"//以下同文
#include "PlayerStateAttack.h"//以下同文
#include "PlayerStateSkillAttack.h"//以下同文
#include "PlayerStateUlt.h"//以下同文
#include "PlayerStateDie.h"//以下同文
#include "PlayerStateDashAttack.h"//以下同文
#include "PlayerStateResultMove.h"//以下同文
#include "../../../DataLoader/AnimData.h"
#include <memory>

class PlayerState;
class Camera;
class Weapon;
class AttackCol;
class CameraManager;
class EnemyBase;
class EnemyManager;
class LockOnManager;

struct ComboNode
{

	std::string animName;//アニメーションの名
	int  modelType = 0;//モデルの種類//0:通常モデル、1:攻撃モデル
	AttackType type = AttackType::None;//攻撃するタイプ
	int index = -1;//攻撃の種類を管理するための変数
	float attackPower = 0;//攻撃力
	float moveFrame = -1;//突進する時間
	float moveSpeedX = 0;//前方向に突進する速度
	float moveSpeedY = 0;//垂直方向の速度
	std::vector<int> nextWeakAttack;//弱攻撃ボタンでつながる次のコンボ番号
	std::vector<int> nextHeavyAttack;//強攻撃ボタンでつながる次のコンボ番号
	float knockBackXZ = 0;//XZ方向のノックバックの距離//攻撃を受けたときに、どれくらいふっとぶか
	float knockBackY = 0;//Y方向のノックバックの距離//攻撃を受けたときにY軸に飛ぶ量
	bool isKirimomi = false;//吹っ飛ぶかどうか//吹っ飛ばない攻撃は、相手を引き寄せるような攻撃にする<-かなりあり！！！！！！！！
	float seFrameRate = -1;//攻撃のSEを鳴らすフレームの割合//アニメーションの再生時間に対する割合で指定
	std::string seName;//攻撃のSEの名前
	float attackColStartRate = 0.0f;//攻撃の当たり判定を有効にするアニメーション進行率
	float attackColEndRate = 0.0f;//攻撃の当たり判定を無効にするアニメーション進行率
	float endFrame = -1.0f;//アニメーションの最終フレーム//負の値なら総フレーム数

};
struct ComboInfo
{

	int currentComboIndex = -1;//現在のコンボの段数//攻撃の段数を管理するための変数
	bool isHit = false;//攻撃が当たったかどうか//当たっていたら動きを止める
	bool isAirAttack = false;//空中で攻撃をしたかどうか
	bool isAirSkillAttack = false;//空中でスキル攻撃をしたかどうか
	int SkillGauge = 0;//スキルゲージ
	int UltGauge = 0;//必殺技ゲージ
	//int nextComboIndex = -1;//次のコンボの段数

};

//いずれ行と列のなまえにしたさがある
enum ComboNodeType : int
{
	None = 0,
	AnimName = 1,
	Model = 2,
	Type = 3,
	Index = 4,
	AttackPower = 5,
	MoveTimeRate = 6,
	MoveSpeedX = 7,
	MoveSpeedY = 8,
	NextLightAttack = 9,
	NextHeavyAttack = 10,
	knockBackXZ = 11,
	knockBackY = 12,
	IsKirimomi = 13,
	SeFrameRate = 14,
	SeName = 15,
	AttackColStartRate = 16,
	AttackColEndRate = 17,
	EndFrame = 18,
	Size = 19,

};
namespace ComboIndex
{
	constexpr int None = -1;
	constexpr int LightAttack1 = 0;
	constexpr int LightAttack2 = 1;
	constexpr int LightAttack3 = 2;
	constexpr int LightAttack4 = 3;
	constexpr int LightAttack5 = 4;

	constexpr int HeavyAttack1 = 5;
	constexpr int HeavyAttack2 = 6;

	constexpr int upAttack = 7;

	constexpr int AirAttack1 = 8;
	constexpr int AirAttack2 = 9;
	constexpr int AirAttack3 = 10;
	constexpr int AirAttack4 = 11;

	constexpr int AirHeavyAttack1 = 12;

	constexpr int DashAttack = 13;
	constexpr int SkillAttack1 = 14;
	constexpr int SkillAttack2 = 15;
	constexpr int SkillAttack3 = 16;

};

struct DamageInfo
{
	float damageTimer = 0.0f;//被ダメ後無敵時間
	const float kDamageTime = 180.0f;//被ダメ後無敵時間の長さ
};

enum class WaveNumForPlayer : int
{
	Wave1 = 0,
	Wave2 = 1,
	Wave3 = 2,
	WaveSize = 3,
};

class Player : public CharacterBase//Playerクラスのインスタンスから、Playerクラスのshared_ptrを取得できるようになる
{
public:
	Player();
	virtual ~Player();

	void Init();
	void SetCameraManager(std::weak_ptr<CameraManager> cameraManager) { m_cameraManager = cameraManager; }//カメラマネージャーのセット
	void SetEnemyManager(std::weak_ptr<EnemyManager> enemyManager) { m_enemyManager = enemyManager; }//EnemyManagerのセット

	void Update() override {}//CharacterBase::Updateの実装(実際の更新はUpdate(Camera&)で行う)
	void Update(Camera& camera);
	void Draw();
	void EffectDraw();

	void SetPos(Vector3 pos) { m_rb.m_pos = pos; };//座標のセット
	void SetIsTitleMode(bool isTitleMode) { m_isTitleMode = isTitleMode; }//タイトル画面かどうかのセット//trueの場合、移動方向をカメラ基準ではなく固定軸にする

	void OnCollision(Collider& other) override;
	void OnDamage(Collider& other, AttackData& data) override;

	void OnAttackHit(int otherId);//攻撃が当たった時の処理


	ComboInfo& GetComboInfo() { return m_comboInfo; }//攻撃コンボの情報を取得する
	const ComboNode& GetComboNode(int comboIndex)const { return m_comboChain[comboIndex]; }//コンボの段数からComboNodeを取得する

	float GetCameraRockOnRange()const { return kPlayerRockOnRange; }//ロックオンする範囲を返す

	//スキルゲージの増減
	void AddSkillGauge(int value);//スキル増減
	//必殺技ゲージの増減
	void AddUltGauge(int value);//必殺技増減
	//ゲージの取得
	int GetSkillGauge()const { return m_comboInfo.SkillGauge; }//スキルゲージの取得
	int GetUltGauge()const { return m_comboInfo.UltGauge; }//必殺技ゲージの取得
	//血殺(必殺技ゲージ)を最大にする//ラストヒットイベント演出用
	void MaxUltGauge() { m_comboInfo.UltGauge = kMaxGaugeValue; }

	//鴉状態かどうか
	bool GetIsRaven()const { return m_isRaven; }

	//ターゲットしている敵の取得
	std::weak_ptr<EnemyBase> GetTargetEnemy()const;
	//内部ロックオンのために
	std::weak_ptr<CameraManager> GetCameraManager()const { return m_cameraManager; }
	//敵ターゲットを保存
	std::weak_ptr<LockOnManager> GetLockOnManager()const { return m_lockOnManager; }
	void SetLockOnManager(std::weak_ptr<LockOnManager> lockOnMgr) { m_lockOnManager = lockOnMgr; }

	//プレイヤーの移動制限
	void SetLimitPlayerArea(int num,bool value) { m_isWaveArea[num] = value; }

	//リザルト集計用//攻撃が敵に当たった時に呼ぶ(与えたダメージ、コンボ数(総ヒット数)を加算する)
	void AddAttackResult(float damage) { m_totalDamageDealt += damage; m_totalHitCount++; }
	int GetTotalDamageDealt()const { return static_cast<int>(m_totalDamageDealt); }//リザルト用//与えた合計ダメージ
	int GetTotalHitCount()const { return m_totalHitCount; }//リザルト用//コンボ数(総ヒット数)
	int GetDamageTakenCount()const { return m_damageTakenCount; }//リザルト用//被弾回数

	//ラストヒットイベント演出用:外部からIdleステートに遷移させるだけの専用関数(汎用化はしない)
	void ForceIdleState();

	//色変え
	void SetResultUp();
	void ChangeResultMove();

private:
	/// <summary>
	/// 状態を変更する関数
	/// </summary>
	/// <param name="newState"></param>
	void ChangeState(std::shared_ptr<PlayerState> newState);//状態遷移の関数//

	void InitializeComboChain();//CSVからコンボデータの読み込みをする
	void UpdateAngle();//回転処理
	void WingUpdate();//鴉状態の羽の更新
	void ApplyPos()override;//座標の適用//Playerクラスでは、座標に加えて、首のボーンの回転も適用する
	void ApplyPosWithAttackModel();//アタックモデルにも適用

	bool CanSkillAttack(bool changeGauge = true);//スキル攻撃ができるかどうか//trueならゲージを減らす
	bool CanUltAttack();//必殺技攻撃ができるかどうか//trueならゲージを減らす

	std::shared_ptr<Player> GetSharedPtr() {return std::dynamic_pointer_cast<Player>(shared_from_this());}

	std::weak_ptr<Player> GetWeakPtr() {return GetSharedPtr();}

private:
	//コンボチェーン
	std::vector<ComboNode> m_comboChain = {};//コンボのデータ
	ComboInfo m_comboInfo = {};//コンボの情報//現在のコンボの段数などを管理するためのもの
	DamageInfo m_damageInfo = {};//被ダメ後無敵時間の情報
	std::shared_ptr<AttackCol> m_burstAttackCol;//吹き飛ばしようのCollider


	//リザルト集計用
	float m_totalDamageDealt = 0.0f;//与えた合計ダメージ
	int m_totalHitCount = 0;//コンボ数(総ヒット数)
	int m_damageTakenCount = 0;//被弾回数

	bool m_isWaveArea[static_cast<int>(WaveNumForPlayer::WaveSize)] = {false};//ウェーブごとに敵がスポーンしたかどうかのフラグ//EnemyManagerのフラグと同じものを持つ

	std::shared_ptr<PlayerState> m_currentState;//プレイヤーの状態//攻撃中、移動中など//状態遷移の管理をするためのもの
	std::shared_ptr<PlayerState> m_prevState;//前の状態

	bool m_isRaven = false;//鴉状態かどうか//攻撃が変化する
	bool m_isTitleMode = false;//タイトル画面かどうか//trueの場合、移動方向をカメラ基準ではなく固定軸にする
	const int kPlayerNeckBoneIndex = 25;//首のボーンのインデックス
	int m_wingModelHandle = -1;//鴉の羽のモデルのハンドル//鴉状態の時に表示する
	std::shared_ptr<Weapon> m_weapon;//武器

	int m_attackModelHandle = -1;//攻撃のモデルのハンドル
	int m_whiteHandle;


	//エフェクトマネージャー作るからいったん表示させる
	int m_efHandle = -1;//エフェクトのハンドル
	int m_efPlayingHandle = -1;//再生中のエフェクトのハンドル
	int m_efAreaMaxHandle[static_cast<int>(WaveNumForPlayer::WaveSize)] ={ -1,-1,-1 } ;//壁エフェクトのハンドル
	int m_efAreaMaxPlayingHandle[static_cast<int>(WaveNumForPlayer::WaveSize)] = { -1,-1,-1 };//再生中

	int m_efAreaMinHandle[static_cast<int>(WaveNumForPlayer::WaveSize)] = { -1,-1,-1 };
	int m_efAreaMinPlayingHandle[static_cast<int>(WaveNumForPlayer::WaveSize)] = { -1,-1,-1 };

	const float kPlayerRockOnRange = 300.0f;//ロックオンする範囲//この範囲内に敵がいたらロックオンする

	const float kJustAvoidRadius = 500.0f;//ジャスト回避の範囲

	const int kMaxGaugeValue = 100;//スキル・必殺技ゲージの最大値
	const int kInitialHp = 100;//プレイヤーの初期HP
	const int kSkillAttackGaugeCost = 20;//スキル攻撃を行う際に消費するスキルゲージ量

	std::weak_ptr<CameraManager> m_cameraManager;//カメラマネージャ-の弱参照
	std::weak_ptr<EnemyManager> m_enemyManager;//EnemyManagerの弱参照
	std::weak_ptr<LockOnManager> m_lockOnManager;
	

	//PlayerState
	friend class PlayerState;//PlayerStateクラスから、Playerクラスのprivateメンバにアクセスできるようにする
	friend class PlayerStateIdle;
	friend class PlayerStateMove;
	friend class PlayerStateJump;
	friend class PlayerStateFall;
	friend class PlayerStateHit;
	friend class PlayerStateAttack;
	friend class PlayerStateSkillAttack;
	friend class PlayerStateUlt;
	friend class PlayerStateDie;
	friend class PlayerStateDashAttack;
	friend class PlayerStateResultMove;
	//武器
	friend class Weapon;

};



