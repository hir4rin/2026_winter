#include "Player.h"
#include "PlayerState.h"
#include "Character/Player/State/Locomotion/PlayerStateIdle.h"
#include "Character/Player/State/Locomotion/PlayerStateMove.h"
#include "PlayerWeapon.h"
#include "Collision/HitCol.h"
#include "Collision/AttackCol.h"
#include "Camera/CameraManager.h"
#include "Camera/LockOnManager.h"
#include "Battle/BattleManager.h"
#include "Battle/ResultRecorder.h"
#include "DataLoader/DataManager.h"
#include "Character/Enemy/EnemyBase.h"
#include "Render/ModelShaderManager.h"
#include "Main/System.h"
#include "Math/Matrix4x4.h"
#include "Camera/Camera.h"
#include "Collision/SphereShape.h"
#include "Collision/CapsuleShape.h"
#include "Stage/Stage.h"
#include "Main/Input.h"
#include "EffekseerForDXLib.h"
//#include "../Effect/EffectManager.h"
#include <cmath>
#include <algorithm>
#include <cassert>
#include <string>
#include <fstream>
#include <sstream>

namespace
{
	constexpr float kPlayerOffset = 40.0f;//かプセルの足のオフセット
	constexpr float kPlayerHead = 120.0f;//カプセルの頭の高さ

	constexpr int kInitialHp = 100;//プレイヤーの初期HP

	constexpr float kArea1MaxZ = 1197;//エリア1のz座標の範囲//上側
	constexpr float kArea2MaxZ = 5275;//エリア2のz座標の範囲//右側

	const Vector3 kArea1EfPos = Vector3(105.0f, 0.0f, 1197.0f);

	constexpr float kRadius = 40.0f;
	constexpr float kRadiusHit = 50.0f;

	constexpr float kCameraShakePower = 2.5f;//カメラの揺れの強さ
	constexpr float kCameraShakeTime = 5.0f;//カメラの揺れの時間

	constexpr float kAirHitKnockBackXZ = 3.0f;//空中被弾で横に押される力
	constexpr float kAirHitKnockBackY = 2.0f;//空中被弾で少しだけ浮く力

	constexpr int kSkillKessatuFrame = 60;//スキル血殺の演出の長さ(タイムスケールを掛けて減る)
	constexpr int kSkillKessatuSlowFrame = 60;//スキル血殺のスローの長さ(実フレーム)
	constexpr float kSkillKessatuTimeScaleRate = 0.1f;//スキル血殺中の時間スケール


	constexpr float kBloodWingScale = 14.0f;//必殺技状態の翼エフェクトのスケール//エフェクトは長さ10ほどで作っている
	const char* const kBloodWingFrameName = "spine_05";//翼の付け根にするボーン//背骨の一番上(肩甲骨の高さ)
	constexpr float kBloodWingBackOffset = 15.0f;//翼の付け根をボーンから背中側にずらす距離
	constexpr float kBloodWingDownOffset = 0.0f;//翼の付け根をボーンから下にずらす距離
	constexpr float kBloodWingTiltFollowRate = 0.5f;//体の傾きに翼を追従させる割合(0で向きだけ、1でボーンと同じだけ傾く)
	constexpr float kBloodWingBackLean = -15.0f * DX_PI_F / 180.0f;//翼を背中側(カメラ側)に倒す角度//マイナスで後ろに倒れる
	constexpr float kBloodWingActivateDelay = 40.0f;//発動の演出から翼を出すまで//演出の血が弾けるフレームに合わせる

	constexpr float kHealRate = 0.4f;//1回の回復量(最大HPに対する割合)
	constexpr float kHealEffectScale = 20.0f;//回復のエフェクトのスケール//1/20で作っている

	constexpr float kRotationLerpFactor = 0.1f;//モデルの向きを目標角度に近づける割合(ほぼlerp)

	constexpr float kGroundCheckCapsuleTopOffset = 100.0f;//地面判定用カプセルの上端のy軸オフセット
	constexpr float kGroundCheckCapsuleBottomOffset = -20.0f;//地面判定用カプセルの下端のy軸オフセット
	constexpr float kGroundCheckCapsuleRadius = 40.0f;//地面判定用カプセルの半径

	constexpr float kSoftTargetKeepFrame = 90.0f;//攻撃をやめてから内部ターゲットを保持するフレーム数
	constexpr float kSoftTargetRangeMult = 3.0f;//これ以上離れたら内部ターゲットを消す(ロックオン範囲に対する倍率)

	const char* const kHeadFrameName = "head";//頭のボーン//敵やカメラの方に向ける
	constexpr float kHeadLookCameraDistance = 1000.0f;//カメラの向きを見るときに、前方のどれくらい先の点を見るか

	const char* const kLookCsvPath = "data/Player/Look/PlayerLook.csv";//マテリアルごとの法線マップ・マスク画像と強さ(CharacterLook が読む)

	//足の軌跡(ジャンプの白いもや)
	const char* const kLeftCalfFrameName = "calf_l";
	const char* const kLeftFootFrameName = "foot_l";
	const char* const kLeftBallFrameName = "ball_l";
	const char* const kRightCalfFrameName = "calf_r";
	const char* const kRightFootFrameName = "foot_r";
	const char* const kRightBallFrameName = "ball_r";
	constexpr float kFootTrailBaseRate = 0.6f;//帯の根元//足首からすねへの割合
	constexpr float kFootTrailTipExtend = 0.5f;//帯の先//つま先をさらに伸ばす割合(足首→つま先)
	constexpr float kFootTrailLife = 14.0f;
	constexpr float kFootTrailMinMove = 1.0f;
	constexpr float kFootTrailHazeAlpha = 160.0f;
	constexpr float kFootTrailStreakAlpha = 80.0f;
	constexpr float kFootTrailStreakGlowAlpha = 25.0f;
	constexpr float kFootTrailEdgeAlpha = 0.0f;
	constexpr float kFootTrailFadePower = 0.8f;

	//肩から光の帯(ダッシュ中)
	const char* const kLeftClavicleFrameName = "clavicle_l";
	const char* const kLeftUpperarmFrameName = "upperarm_l";
	const char* const kRightClavicleFrameName = "clavicle_r";
	const char* const kRightUpperarmFrameName = "upperarm_r";
	constexpr float kShoulderTrailBack = 12.0f;//肩から背中側へずらす
	constexpr float kShoulderTrailDown = 4.0f;//肩から下へずらす
	constexpr float kShoulderTrailOut = 10.0f;//肩から外側へずらす
	constexpr float kShoulderTrailHalfWidth = 2.0f;//筋の幅の半分//外側に45°傾けた向きに広げる
	constexpr float kShoulderTrailSwaySide = 6.0f;//なびきの横の幅
	constexpr float kShoulderTrailSwayUp = 4.0f;//なびきの縦の幅
	constexpr float kShoulderTrailSwaySpeed = 0.3f;//なびきの速さ(1フレームに進む角度)
	constexpr float kShoulderTrailLife = 12.0f;
	constexpr float kShoulderTrailMinMove = 1.0f;
	constexpr float kShoulderTrailHazeAlpha = 34.0f;//まわりのぼんやりした光
	constexpr float kShoulderTrailStreakAlpha = 173.0f;//筋
	constexpr float kShoulderTrailStreakGlowAlpha = 120.0f;//筋を加算で重ねる
	constexpr float kShoulderTrailEdgeAlpha = 0.0f;
	constexpr float kShoulderTrailFadePower = 0.7f;
	constexpr int kShoulderTrailTintR = 150;//もや・加算の線の色(淡い水色)
	constexpr int kShoulderTrailTintG = 215;
	constexpr int kShoulderTrailTintB = 255;
	//出直すたびの場所のばらつき(x 外側 / y 上 / z 後ろ)
	constexpr float kShoulderStrandOutMin = -4.0f;
	constexpr float kShoulderStrandOutMax = 14.0f;
	constexpr float kShoulderStrandUpMin = -6.0f;
	constexpr float kShoulderStrandUpMax = 10.0f;
	constexpr float kShoulderStrandBackMin = 0.0f;
	constexpr float kShoulderStrandBackMax = 10.0f;
	constexpr float kShoulderStrandOnMin = 8.0f;//出ている長さ
	constexpr float kShoulderStrandOnMax = 16.0f;
	constexpr float kShoulderStrandOffMin = 2.0f;//消えている長さ
	constexpr float kShoulderStrandOffMax = 7.0f;
	const char* const kShoulderTrailTexturePath = "data/Effect/Move/Texture/MW_WindStrand_a.png";
	const char* const kShoulderTrailGlowTexturePath = "data/Effect/Move/Texture/MW_RibbonGlow_a.png";
}


Player::Player()
{
	m_hp = kInitialHp;
	m_comboInfo.SkillGauge = kMaxGaugeValue;
	m_comboInfo.UltGauge = kMaxGaugeValue;
	//m_modelHandle = MV1LoadModel("data/Player/Player.mv1");
	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerModel));
	m_waistFrame = MV1SearchFrame(m_modelHandle, "pelvis");//腰のボーン
	m_headFrame = MV1SearchFrame(m_modelHandle, kHeadFrameName);//頭のボーン
	//m_modelHandle = MV1LoadModel("data/Player/Player_true.mv1");
	//m_modelHandle = MV1LoadModel("data/Player/1danme.mv1");

	m_whiteHandle = LoadGraph("data/UI/white.png");

	m_rb.m_pos = Vector3(0, 0, 0);

	//モデルの初期位置を設定する//前を向いているようにする
	Matrix4x4 rotY = Matrix4x4::MakeRotationY(DX_PI_F);
	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);
	Matrix4x4 mtx = trans * rotY;
	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));

	//コンボチェーンの初期化
	InitializeComboChain();
	//アニメーションの名前のマップの初期化
	const auto& animData = DataManager::GetInstance().GetPlayerAnimData();
	m_animNames = animData.animNames;

	//必殺技状態の翼の付け根のボーン
	m_bloodWingFrame = MV1SearchFrame(m_modelHandle, kBloodWingFrameName);

	//足の軌跡
	m_calfFrame[0] = MV1SearchFrame(m_modelHandle, kLeftCalfFrameName);
	m_footFrame[0] = MV1SearchFrame(m_modelHandle, kLeftFootFrameName);
	m_ballFrame[0] = MV1SearchFrame(m_modelHandle, kLeftBallFrameName);
	m_calfFrame[1] = MV1SearchFrame(m_modelHandle, kRightCalfFrameName);
	m_footFrame[1] = MV1SearchFrame(m_modelHandle, kRightFootFrameName);
	m_ballFrame[1] = MV1SearchFrame(m_modelHandle, kRightBallFrameName);
	const PlayerSwordTrail::Style footStyle = { kFootTrailLife, kFootTrailMinMove, kFootTrailHazeAlpha,
		kFootTrailStreakAlpha, kFootTrailStreakGlowAlpha, kFootTrailEdgeAlpha, kFootTrailFadePower };
	for (auto& trail : m_footTrail) trail.SetStyle(footStyle);

	//肩の光の帯
	m_clavicleFrame[0] = MV1SearchFrame(m_modelHandle, kLeftClavicleFrameName);
	m_upperarmFrame[0] = MV1SearchFrame(m_modelHandle, kLeftUpperarmFrameName);
	m_clavicleFrame[1] = MV1SearchFrame(m_modelHandle, kRightClavicleFrameName);
	m_upperarmFrame[1] = MV1SearchFrame(m_modelHandle, kRightUpperarmFrameName);
	const PlayerSwordTrail::Style shoulderStyle = { kShoulderTrailLife, kShoulderTrailMinMove, kShoulderTrailHazeAlpha,
		kShoulderTrailStreakAlpha, kShoulderTrailStreakGlowAlpha, kShoulderTrailEdgeAlpha, kShoulderTrailFadePower,
		kShoulderTrailTintR, kShoulderTrailTintG, kShoulderTrailTintB };
	for (auto& strands : m_shoulderStrand)
	{
		for (int i = 0; i < kShoulderStrandNum; ++i)
		{
			auto& strand = strands[i];
			strand.trail.SetStyle(shoulderStyle);
			strand.trail.SetTextures(kShoulderTrailTexturePath, kShoulderTrailTexturePath, kShoulderTrailGlowTexturePath);
			//最初から全部そろって出ないようにずらす
			strand.timer = static_cast<float>(GetRand(static_cast<int>(kShoulderStrandOnMax)));
			strand.phase = static_cast<float>(GetRand(628)) * 0.01f;
		}
	}

	//法線マップ・遮蔽・光沢の画像と強さ(マテリアルの名前で当てる)
	m_look.Load(m_modelHandle, kLookCsvPath);
}

Player::~Player()
{
	if (m_currentState)
	{
		m_currentState->Exit();//状態を抜ける
	}
	if (m_bloodWingPlayingHandle != -1)StopEffekseer3DEffect(m_bloodWingPlayingHandle);
	if (m_bloodWingActivateHandle != -1)StopEffekseer3DEffect(m_bloodWingActivateHandle);
	if (m_healEffectHandle != -1)StopEffekseer3DEffect(m_healEffectHandle);
	MV1DeleteModel(m_modelHandle);
	DeleteGraph(m_whiteHandle);
}

void Player::Init()
{
	//向いている方向
	m_targetVec = Vector3(0, 0, 1);//最初は前を向いているようにする
	//初期状態をIdleにする//アニメーションの初期化
	m_anim.Init(m_modelHandle, GetAnimName("Idle"), true);
	//weak_from_this()は、shared_ptrを作成,
	//Playerクラスのインスタンスから、Playerクラスのshared_ptrを取得できるようになる
	m_currentState = std::make_shared<PlayerStateIdle>(GetWeakPtr());

	//IDの取得
	SetID();

	//当たり判定の初期化
	//中心点、オフセット、半径、当たり判定のタイプ、タグ、当たり判定が有効かどうか
	ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, kPlayerOffset, 0),
		.shape = std::make_unique<CapsuleShape>(Vector3(0,kPlayerHead,0),kRadius),
		.tag = {Collider::Faction::Player, Collider::ColRole::None},
		.isActive = true
		});

	//地面への吸着を行う
	SetUseGroundSnap(true);

	//やられ判定の初期化
	InitHitCol(GetWeakPtr());
	m_hitCol->ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, 100.0f, 0),
		.shape = std::make_unique<SphereShape>(kRadiusHit),
		.tag = {Collider::Faction::Player, Collider::ColRole::Hit},
		.isActive = true,
		.isTrigger = true
		});

	CharacterBase::ApplyPos();//座標の更新//モデルの座標を更新する
	ChangeState(m_currentState);//初期化

	//武器の生成
	m_weapon = std::make_shared<PlayerWeapon>(GetWeakPtr());//武器の生成//Playerクラスのインスタンスから、Playerクラスのshared_ptrを取得できるようになる
	//effectの生成
	m_efHandle = System::GetInstance().GetHandle(AsyncData::PlayerEffectSkill);
	for (auto& handle : m_efAreaMaxHandle)
	{
		handle = System::GetInstance().GetHandle(AsyncData::AreaWallEffect);
	}
	for (auto& handle : m_efAreaMinHandle)
	{
		handle = System::GetInstance().GetHandle(AsyncData::AreaWallEffect);
	}
	m_lockOnManager = std::make_shared<LockOnManager>();
	m_lockOnManager->Init(GetWeakPtr(), m_cameraManager, m_enemyManager);
}

int g_trailCapture = 0;//[TEMP_TRAIL]
void Player::Update(Camera& camera)
{
	//ライト(雑実装)
	auto& input = Input::GetInstance();

	//[TEMP_TRAIL] 確認用
	{
		static int s_trailFrame = 0;
		++s_trailFrame;
		if (strstr(GetCommandLineA(), "-trailtest") && s_trailFrame >= 100 && g_trailCapture == 0 && IsFloor())
		{
			ChangeState(std::make_shared<PlayerStateJump>(GetWeakPtr()));
			g_trailCapture = 1;
		}
		if (strstr(GetCommandLineA(), "-dashtest"))
		{
			if (s_trailFrame == 100) input.StartRecord({ { 10, { "Up" } }, { 1, { "Up", "L3" } }, { 80, { "Up" } }, { 40, {} } });
			if (g_trailCapture == 0 && (std::dynamic_pointer_cast<PlayerStateFastRun>(m_currentState) || s_trailFrame == 200)) g_trailCapture = 1;
			if (s_trailFrame % 10 == 0) { FILE* fp = nullptr; fopen_s(&fp, "Log/dashtest.txt", "a"); if (fp) { fprintf(fp, "f%d %s up=%d floor=%d y=%.1f\n", s_trailFrame, typeid(*m_currentState).name(), input.IsPressed("Up") ? 1 : 0, IsFloor() ? 1 : 0, m_rb.m_pos.y); fclose(fp); } }
		}
	}

	//被ダメ後の無敵時間の更新
	if (m_damageInfo.damageTimer > 0.0f)
	{
		m_damageInfo.damageTimer -= 1.0f * System::GetInstance().GetTimeScale();//被ダメ後の無敵時間を減らす
	}

#ifdef _DEBUG
	{
		if (input.IsTriggered("Start"))
		{
			AddSkillGauge(20);
			AddUltGauge(20);
		}
		//デバック用
		if (input.IsTriggered("RT") && m_isUltimating)
		{
			m_isUltimating = false;
		}

		/*if (input.IsTriggered("A"))
		{
			if (!std::dynamic_pointer_cast<PlayerStateStartMovie>(m_currentState))
			{
				ChangeState(std::make_shared< PlayerStateStartMovie>(GetWeakPtr()));
				return;
			}
			else
			{
				ChangeState(std::make_shared< PlayerStateIdle>(GetWeakPtr()));
				return;
			}
		}*/

	}
#endif

	//デバッグ用//F8を押した瞬間だけComboChain.csvを読み込み直してコンボノードを更新する
	bool isReloadPressed = CheckHitKey(KEY_INPUT_F8) != 0;
	if (isReloadPressed && !m_wasReloadPressed)
	{
		DataManager::GetInstance().ReloadComboRawData();
		InitializeComboChain();
	}
	m_wasReloadPressed = isReloadPressed;


	//LTを押したとき
	if (input.IsTriggered("LT"))
	{
		//必殺技状態中にもう一度押したら専用必殺技
		if (m_isUltimating)
		{
			if (CanUltFinish())ChangeState(std::make_shared<PlayerStateUlt>(GetWeakPtr()));
		}
		//必殺技状態に移行
		else
		{
			m_isUltimating = true;
			StartBloodWingActivate();
		}
	}

	//RBで回復
	if (input.IsTriggered("RB"))
	{
		Heal();
	}
	HealEffectUpdate();

	//押し戻しの処理が続かないように消す//縦の速度(重力)は空中のステート(Jump,Fall,Attack)が自分で作る
	m_rb.m_vel = Vector3(0, 0, 0);

	//落下死したら止める(リスポーンまで)
	if (m_deathCause == DeathCause::Fall || CheckFallDeath())return;

	//ロックオンと内部ターゲットの更新
	m_lockOnManager->Update();
	UpdateSoftTarget();

	UpdateGuardBreakChance();//ブレイクのチャンス

	//ボスのトドメのムービー//やられていたら入らない
	if (!m_bossFinisherTarget.expired())
	{
		if (std::dynamic_pointer_cast<PlayerStateDie>(m_currentState))m_bossFinisherTarget.reset();
		else ChangeState(std::make_shared<PlayerStateBossFinisher>(GetWeakPtr()));
	}

	m_swordTrail.SetEmitting(false);//攻撃のStateがCSVの区間の間だけtrueにする
	SetFootTrailEmitting(false);//ジャンプの間だけtrueにする
	SetShoulderTrailEmitting(false);//ダッシュの間だけtrueにする
	m_shoulderSwayFrame += System::GetInstance().GetTimeScale() * m_ownTimeScale;
	UpdateShoulderStrands();
	if (m_currentState)
	{
		m_currentState->Update();//状態の更新
	}
	//軌跡に刀の位置を足すのは、押し戻しの後の位置で描くときに行う
	m_isTrailSamplePending = true;
	m_trailTimeScale = System::GetInstance().GetTimeScale() * m_ownTimeScale;
	//頭を敵やカメラの方に向ける//アニメーションの更新(各Stateの中)の後に呼ぶ
	UpdatePlayerHeadLook();


	//演出で血を吸い込んでいる間は翼を出さない//BloodWingUpdate は1フレームに2回呼ばれるのでここで減らす
	if (m_bloodWingDelay > 0.0f)m_bloodWingDelay -= System::GetInstance().GetTimeScale();

	//これらは押し戻しの時に呼ばれないのでずれる→そこでも呼ぶ必要あり
	BloodWingUpdate();
	m_weapon->Update();//武器の更新

	//回転処理//座標も行列で更新
	//UpdateAngle();
	//UpdateAngleAndPos();

	//-----------------------変わらなかった；；-----------------------------------------
	////ルートモーションONの場合、
	////アニメーションが適用された後のモデルの行列を取得
	//MATRIX modelMat = MV1GetMatrix(m_modelHandle);

	////モデルの行列から、移動量を取得する
	//m_rb.m_pos.x = modelMat.m[3][0];
	//m_rb.m_pos.y = modelMat.m[3][1];
	//m_rb.m_pos.z = modelMat.m[3][2];
	////Y軸回転角度を合わせる
	//m_rotAngleY = atan2f(modelMat.m[0][2], modelMat.m[2][2]);//モデルの行列から、Y軸の回転角度を取得する

	////方向ベクトルを同期
	//m_targetVec.x = sinf(m_rotAngleY + DX_PI_F);
	//m_targetVec.z = cosf(m_rotAngleY + DX_PI_F);
	//m_targetVec.y = 0.0f;
	//m_targetVec = m_targetVec.Normalize();
	//----------------------------------------------------------------------------------------
}
void Player::Draw()
{
	//スキル攻撃中はモデルを描画しない(代わりにスキルのエフェクトが出ている)//演出で隠しているときも描かない
	if (!m_isSkillInvisible && !m_isHidden)
	{
		//モデルの描画
		//ステージの明かりの光も足す
		m_look.Draw(m_modelHandle);//読めていなければ今まで通りの描き方になる

		m_weapon->Draw(m_swordTrail.IsEmitting());//武器//軌跡を出している間は刀身を光らせる
	}
	//刀の軌跡//画面の刀と同じ位置になるよう、描く直前の刀の位置を足す
	if (m_isTrailSamplePending)
	{
		Vector3 base, tip;
		m_weapon->GetBladePos(base, tip);
		m_swordTrail.Sample(base, tip, m_trailTimeScale);
		for (int i = 0; i < 2; ++i)
		{
			GetFootTrailPos(i, base, tip);
			m_footTrail[i].Sample(base, tip, m_trailTimeScale);
			for (int j = 0; j < kShoulderStrandNum; ++j)
			{
				auto& strand = m_shoulderStrand[i][j];
				strand.trail.SetEmitting(m_isShoulderTrailEmitting && strand.isOn);
				GetShoulderTrailPos(i, j, base, tip);
				strand.trail.Sample(base, tip, m_trailTimeScale);
			}
		}
		m_isTrailSamplePending = false;
	}
	m_swordTrail.Draw();
	for (auto& trail : m_footTrail) trail.Draw();
	for (auto& strands : m_shoulderStrand)
	{
		for (auto& strand : strands) strand.trail.Draw();
	}
	//コンボチェーンの描画
	for (int i = 0; i < m_comboChain.size(); ++i)
	{
		//std::string text = std::to_string(i) + ": " + m_comboChain[i].animName;
	}
#ifdef _DEBUG
	if (m_currentState)
	{
		m_currentState->DebugDraw();//デバッグ描画
		m_currentState->DebugDrawWallCheck();//壁判定のレイ
	}
	//DrawSphere3D(m_rb.m_pos.ToDxLibVector(), kPlayerRockOnRange * 5, 16, GetColor(255, 0, 0), GetColor(255, 0, 0), false);
#endif

}

void Player::EffectDraw()
{
	//Vector3 offset = m_targetVec * 50.0f;
	// 再生中のエフェクトを移動する。

	
}

void Player::OnCollision(Collider& other)
{
}

void Player::OnDamage(Collider& other, AttackData& data)
{
#ifdef _DEBUG
	//return;
#endif
	//ダメージを受けたときの処理
	//敵の攻撃データをもらい、ダメージを減らし、体力を減らす、場合によってはプレイヤーを吹き飛ばす
	//DrawFormatString(0, 0, GetColor(255, 0, 0), "Player: OnDamage");
	//状態による無敵(ジャスト回避の受付中など)はダメージを受けない
	if (m_currentState && m_currentState->IsInvincible())return;
	//ダメージを受けたときの処理
	if (m_damageInfo.damageTimer <= 0.0f)//無敵時間が終わっている場合のみダメージを受ける
	{
		//体力を減らす
		m_hp -= static_cast<int>(data.attackPower);
		//ダメージが入った瞬間にSEとエフェクトを出す
		//System::GetInstance().GetSoundManager().PlaySE("PlayerHit");
		//EffectManager::GetInstance().Play(AsyncData::PlayerDamageEffect, m_rb.m_pos + Vector3(0, kPlayerCenter, 0));
		//無敵時間を設定する
		m_damageInfo.damageTimer = m_damageInfo.kDamageTime;
		//リザルト集計用//被弾回数を加算する
		m_damageTakenCount++;
		//コンボを切る
		if (auto recorder = System::GetInstance().GetBattleMgr()->GetResultRecorder().lock()) recorder->OnPlayerDamaged();
		if (m_hp <= 0)
		{
			//死亡ステートにする
			m_deathCause = DeathCause::Hp;
			ChangeState(std::make_shared<PlayerStateDie>(GetWeakPtr(), data));
			return;
		}
	}
	//無敵時間中はダメージを受けない
	else
	{
		return;
	}
	//被ダメdataを更新する
	m_attackData = data;

	//スキル攻撃中、必殺技中は被弾アニメーションに遷移しない(ダメージ自体は通常通り受ける)
	bool isSkillAttacking = std::dynamic_pointer_cast<PlayerStateSkillAttack>(m_currentState) != nullptr;//打ち上げ(スキル0)も含む
	bool isUltAttacking = std::dynamic_pointer_cast<PlayerStateUlt>(m_currentState) != nullptr;
	if (isSkillAttacking || isUltAttacking)return;


	//Kirimomi攻撃なら吹き飛ばせる
	if (data.isKirimomi)
	{
		//ヒット情報の作成
		HitInfo hitinfo = {
			.knockBackVel = CalcKnockBackDir(other) * data.knockBackPower.x + Vector3(0.0f, data.knockBackPower.y, 0.0f),
			.isKirimomi = data.isKirimomi,
		};
		//吹き飛ばしステートにする
		ChangeState(std::make_shared<PlayerHitKirimomiState>(GetWeakPtr(), hitinfo));
		return;
	}

	//空中で被弾したら小さく吹っ飛んで落ちる//OnDamage中はIsFloorが使えないので前フレームの接地を見る
	if (!WasFloor())
	{
		//吹き飛ぶ力は攻撃ではなくプレイヤー側の値
		HitInfo hitinfo = {
			.knockBackVel = CalcKnockBackDir(other) * kAirHitKnockBackXZ + Vector3(0.0f, kAirHitKnockBackY, 0.0f),
		};
		ChangeState(std::make_shared<PlayerHitAirState>(GetWeakPtr(), hitinfo));
		return;
	}

	//stateをhitStateにする
	ChangeState(std::make_shared<PlayerStateHit>(GetWeakPtr(), data));
}

Vector3 Player::CalcKnockBackDir(Collider& other)
{
	//攻撃判定は持ち主の前に出ているので、持ち主の位置を基準にする
	Vector3 attackerPos = other.GetRigidBody().GetPos();
	auto attackCol = dynamic_cast<AttackCol*>(&other);
	if (attackCol)
	{
		auto attacker = attackCol->GetOwner().lock();
		if (attacker) attackerPos = attacker->GetRigidBody().GetPos();
	}

	Vector3 dir = m_rb.m_pos - attackerPos;
	dir.y = 0.0f;
	//重なっていたら後ろへ
	if (dir.Magnitude() <= 0.0f) dir = m_targetVec * -1.0f;
	return dir.Normalize();
}

bool Player::CheckFallDeath()
{
	//HPで死んでいる最中は見ない
	if (m_deathCause != DeathCause::None)return false;
	if (IsEndingJumping())return false;//列車から飛び降りて落ちている間は死なない
	auto stage = m_stage.lock();
	if (!stage)return false;

	bool isFall = m_rb.m_pos.y < stage->GetKillY() ||
		stage->IsInWallZone(Collider::ColRole::DeathZone, *this);
	if (!isFall)return false;

	m_deathCause = DeathCause::Fall;
	m_isDead = true;
	m_rb.m_moveVel = Vector3(0, 0, 0);
	return true;
}

void Player::OnParried()
{
	//死亡中は何もしない
	if (std::dynamic_pointer_cast<PlayerStateDie>(m_currentState))return;
	if (std::dynamic_pointer_cast<PlayerAttackParried>(m_currentState))return;
	
	//空中ならはじかせない
	if (!m_wasFloor)return;

	//敵パリィに遷移
	ChangeState(std::make_shared<PlayerAttackParried>(GetWeakPtr()));
}

void Player::OnJustDodge(Collider& other, AttackData& data)
{
	//現在の状態に通知する//回避状態以外は何もしない
	if (!m_currentState)return;
	m_currentState->OnJustDodge(other, data);
}

float Player::GetAttackFrame()
{
	//ステートがないときは今のアニメのフレームを返す//保険
	const float animFrame = m_anim.GetNowAnimFrame();

	//窓口
	return m_currentState ? m_currentState->GetAttackFrame(animFrame) : animFrame;
}

void Player::StartSkillKessatsu(const Vector3& pos)
{
	auto battleMgr = System::GetInstance().GetBattleMgr();
	battleMgr->SetUltStart(kSkillKessatuFrame, pos);
	if (auto recorder = battleMgr->GetResultRecorder().lock()) recorder->AddTechnique(ResultData::Technique::SkillKessatsu);
	if (battleMgr->GetIsLastHitEventPlaying())return;
	System::GetInstance().SetTimeScaleForFrames(kSkillKessatuTimeScaleRate, kSkillKessatuSlowFrame);
	if (auto cameraManager = m_cameraManager.lock())
	{
		cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::SkillKessatuCamera);
	}
}

void Player::ForceIdleState()
{
	//強制的に止めるので、移動の慣性も消す(Idleで滑らないように)
	m_rb.m_moveVel = Vector3(0, 0, 0);
	ChangeState(std::make_shared<PlayerStateIdle>(GetWeakPtr()));


	//IsHitStateに変える//後でする



}

void Player::OnAttackHit(int otherId)
{

	auto cameraManager = GetCameraManager().lock();

	////ownerに当たったことを連絡->AttackMoveを止める
	//bool isLockOn = mainCamera->GetIsLockOn();
	//if (isLockOn)
	//{
	//	//ロックオンしている敵がいて、そいつに当たったら攻撃の移動を止める
	//	auto playerTarget = GetTargetEnemy();
	//	auto targetEnemy = playerTarget.lock();
	//	if (!targetEnemy)
	//	{
	//		assert(false && "PlayerAttackOnCollision:ターゲットしている敵がいません");
	//	}
	//	if (otherId == targetEnemy->GetId())
	//	{
	//		//攻撃の移動を止める
	//		auto& comboInfo = GetComboInfo();
	//		comboInfo.isHit = true;//攻撃が当たったことを通知する//これで、攻撃の移動を止める
	//	}
	//}
	////ロックオンしていない場合
	//else
	//{
	//	//内部ターゲットにセットする
	//	lockOnManager->SetTargetEnemy(otherId);
	//}
}

void Player::SetResultUp()
{

	int texNum = MV1GetTextureNum(m_modelHandle);
	for (int i = 0; i < texNum; i++) {
		MV1SetTextureGraphHandle(m_modelHandle, i, m_whiteHandle, FALSE);
	}

	int matNum = MV1GetMaterialNum(m_modelHandle);
	const COLOR_F kResultMatColorOff = GetColorF(0.0f, 0.0f, 0.0f, 1.0f);//反射・環境光を消す色
	const COLOR_F kResultEmissiveColor = GetColorF(1.0f, 0.0f, 0.0f, 1.0f);//発光色(赤)
	for (int i = 0; i < matNum; i++) {
		MV1SetMaterialDifColor(m_modelHandle, i, kResultMatColorOff); // 拡散反射は消す
		MV1SetMaterialSpcColor(m_modelHandle, i, kResultMatColorOff); // 鏡面ハイライトも消す
		MV1SetMaterialAmbColor(m_modelHandle, i, kResultMatColorOff); // 環境光も消す
		MV1SetMaterialEmiColor(m_modelHandle, i, kResultEmissiveColor); // 発光色だけ赤に
	}
}

void Player::ChangeResultMove()
{
	//現在の状態から抜ける
	if (m_currentState)
	{
		m_currentState->Exit();
	}

	//newStateに更新
	m_prevState = m_currentState;
	std::shared_ptr<PlayerState> state = std::make_shared<PlayerStateResultMove>(GetWeakPtr());
	m_currentState = state;
	//newStateの初期化
	if (m_currentState)
	{
		m_currentState->Enter();
	}

}

void Player::ChangeState(std::shared_ptr<PlayerState> newState)
{
	//現在の状態から抜ける
	if (m_currentState)
	{
		m_currentState->Exit();
	}
	//newStateに更新
	m_prevState = m_currentState;
	m_currentState = newState;
	//歩く・走る・止まる以外のステートに移ったら、移動の慣性を消す
	//(攻撃や回避が終わってIdleに戻ったとき、前に走っていた向きに滑らないように)
	bool isLocomotion = std::dynamic_pointer_cast<PlayerStateIdle>(newState) ||
		std::dynamic_pointer_cast<PlayerStateMove>(newState) ||
		std::dynamic_pointer_cast<PlayerStateFastRun>(newState);
	if (!isLocomotion)
	{
		m_rb.m_moveVel = Vector3(0, 0, 0);
	}
	//newStateの初期化
	if (m_currentState)
	{
		m_currentState->Enter();
	}
}

void Player::AddSkillGauge(int value)
{
	return;//一旦100のまま
	m_comboInfo.SkillGauge += value;
	if (m_comboInfo.SkillGauge > kMaxGaugeValue)
	{
		m_comboInfo.SkillGauge = kMaxGaugeValue;
	}
}

void Player::AddUltGauge(int value)
{
	m_comboInfo.UltGauge += value;
	if (m_comboInfo.UltGauge > kMaxGaugeValue)
	{
		m_comboInfo.UltGauge = kMaxGaugeValue;
	}
}

int Player::GetMaxHp()const
{
	return kInitialHp;
}

bool Player::IsLockOn()const
{
	//trueでロックオン中
	return m_lockOnManager && m_lockOnManager->IsLockOn();
}

void Player::SetSoftTarget(std::shared_ptr<EnemyBase> target)
{
	m_softTarget = target;
	//制限時間をセット
	m_softTargetKeepTimer = kSoftTargetKeepFrame;
}

std::shared_ptr<EnemyBase> Player::GetSoftTarget() const
{
	auto target = m_softTarget.lock();
	if (!target || target->GetIsLifeZero())return nullptr;
	return target;
}

Vector3 Player::GetCameraFocusPos() const
{
	//☆注意事項
	//これはスキル0->1の時のカメラをカッコいい角度にするようなので、汎用性はない

	Vector3 pos = m_rb.m_pos;
	if (!m_isCameraFocusOverride)return pos;
	//打ち上げた敵が死んでいたら通常の注視点
	auto enemy = m_cameraFocusEnemy.lock();
	if (!enemy || enemy->GetIsLifeZero())return pos;
	//Yだけ敵に合わせる//プレイヤーの方が高いときはプレイヤー
	pos.y = (std::max)(pos.y, enemy->GetRigidBody().GetPos().y);
	return pos;
}

std::shared_ptr<EnemyBase> Player::GetAttackTarget() const
{
	//ロックオンしているならロックオンの敵を返す

	if (IsLockOn())
	{
		return m_lockOnManager->GetLockTarget();
	}
	//違うなら内部ターゲットを返す
	return GetSoftTarget();
}

bool Player::IsMoveState() const
{
	return std::dynamic_pointer_cast<PlayerStateMove>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateFastRun>(m_currentState);
}

void Player::SetFootTrailEmitting(bool isEmitting)
{
	for (auto& trail : m_footTrail) trail.SetEmitting(isEmitting);
}

void Player::SetShoulderTrailEmitting(bool isEmitting)
{
	m_isShoulderTrailEmitting = isEmitting;
}

void Player::UpdateShoulderStrands()
{
	auto randRange = [](float min, float max) { return min + (max - min) * static_cast<float>(GetRand(1000)) * 0.001f; };
	const float speed = System::GetInstance().GetTimeScale() * m_ownTimeScale;
	for (auto& strands : m_shoulderStrand)
	{
		for (auto& strand : strands)
		{
			strand.timer -= speed;
			if (strand.timer > 0.0f) continue;
			strand.isOn = !strand.isOn;
			if (strand.isOn)
			{
				//出直すたびに別の場所から
				strand.timer = randRange(kShoulderStrandOnMin, kShoulderStrandOnMax);
				strand.offset = Vector3(randRange(kShoulderStrandOutMin, kShoulderStrandOutMax),
					randRange(kShoulderStrandUpMin, kShoulderStrandUpMax),
					randRange(kShoulderStrandBackMin, kShoulderStrandBackMax));
				strand.phase = randRange(0.0f, DX_TWO_PI_F);
			}
			else
			{
				strand.timer = randRange(kShoulderStrandOffMin, kShoulderStrandOffMax);
			}
		}
	}
}

void Player::GetShoulderTrailPos(int side, int index, Vector3& base, Vector3& tip) const
{
	if (m_clavicleFrame[side] < 0 || m_upperarmFrame[side] < 0)
	{
		base = tip = m_rb.m_pos;
		return;
	}
	const Vector3 clavicle = Vector3(MV1GetFramePosition(m_modelHandle, m_clavicleFrame[side]));
	const Vector3 upperarm = Vector3(MV1GetFramePosition(m_modelHandle, m_upperarmFrame[side]));
	Vector3 forward = Vector3(m_targetVec.x, 0.0f, m_targetVec.z);
	if (forward.Magnitude() > 0.0f) forward = forward.Normalize();
	//体の中心から肩への横の向き(外側)
	Vector3 out = upperarm - clavicle;
	out.y = 0.0f;
	if (out.Magnitude() > 0.0f) out = out.Normalize();
	//筋ごとにずらしてなびかせる
	const ShoulderStrand& strand = m_shoulderStrand[side][index];
	const float phase = m_shoulderSwayFrame * kShoulderTrailSwaySpeed + strand.phase;
	Vector3 center = upperarm - forward * (kShoulderTrailBack + strand.offset.z) + out * (kShoulderTrailOut + strand.offset.x);
	center = center + out * (sinf(phase) * kShoulderTrailSwaySide)
		+ Vector3(0.0f, sinf(phase * 1.3f) * kShoulderTrailSwayUp - kShoulderTrailDown + strand.offset.y, 0.0f);
	//筋の面を外側へ45°傾ける(真後ろから見ても幅が出るように)
	const Vector3 widthDir = (out + Vector3(0.0f, 1.0f, 0.0f)).Normalize();
	base = center - widthDir * kShoulderTrailHalfWidth;
	tip = center + widthDir * kShoulderTrailHalfWidth;
}

void Player::GetFootTrailPos(int side, Vector3& base, Vector3& tip) const
{
	if (m_calfFrame[side] < 0 || m_footFrame[side] < 0 || m_ballFrame[side] < 0)
	{
		base = tip = m_rb.m_pos;
		return;
	}
	const Vector3 calf = Vector3(MV1GetFramePosition(m_modelHandle, m_calfFrame[side]));
	const Vector3 foot = Vector3(MV1GetFramePosition(m_modelHandle, m_footFrame[side]));
	const Vector3 ball = Vector3(MV1GetFramePosition(m_modelHandle, m_ballFrame[side]));
	base = foot + (calf - foot) * kFootTrailBaseRate;
	tip = ball + (ball - foot) * kFootTrailTipExtend;
}

bool Player::CanInputAssasinState() const
{
	return std::dynamic_pointer_cast<PlayerStateIdle>(m_currentState) ||
		IsMoveState() ||
		std::dynamic_pointer_cast<PlayerStateJump>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateFall>(m_currentState);
}

std::shared_ptr<EnemyBase> Player::GetAssasinTarget() const
{
	return m_assasinTarget.lock();
}

const bool Player::CanAssasin() const
{
	auto assasinTarget = GetAssasinTarget();

	return assasinTarget ? true : false;
}

std::shared_ptr<EnemyBase> Player::GetPartBrokenTarget() const
{
	return m_partBrokenTarget.lock();
}

const bool Player::CanPartBrokenFinish() const
{
	auto partBrokenTarget = GetPartBrokenTarget();

	return partBrokenTarget ? true : false;
}

bool Player::CanInputPartBrokenState() const
{
	return std::dynamic_pointer_cast<PlayerStateIdle>(m_currentState) ||
		IsMoveState() ||
		std::dynamic_pointer_cast<PlayerStateAttack>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateAttackLanding>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateSkillAttack>(m_currentState);
}




void Player::InitializeComboChain()
{
	//コンボチェーンの初期化
	//音の出すタイミングは、攻撃の当たり判定を有効にするタイミングと同じにする予定なので、コンボノードの中に入れる

	//CSVファイルを読み込む
	const auto& rawData = DataManager::GetInstance().GetComboRawData();

	m_comboChain.clear();//読み込み直しのときに前のデータが残らないように消す

	for (const auto& tokens : rawData)
	{
		//列数チェック//tokensはvector<string>で、1行分のデータが入っている//横
		if (tokens.size() < ComboNodeType::Size)//vectorのサイズなので、push_backした行の数
		{
			assert(false && "ComboChain.csvの列数が不足しています");
			continue;
		}
		ComboNode node;
		node.animName = tokens[ComboNodeType::AnimName];
		//アニメーションの再生速度//空欄なら1.0(等速)
		node.animTimeScale = tokens[ComboNodeType::AnimTimeScale].empty() ? 1.0f : std::stof(tokens[ComboNodeType::AnimTimeScale]);
		node.index = std::stoi(tokens[ComboNodeType::Index]);
		//攻撃の種類//空欄ならNone
		node.attackType = tokens[ComboNodeType::AttackTypeCol].empty() ? AttackType::None : static_cast<AttackType>(std::stoi(tokens[ComboNodeType::AttackTypeCol]));
		node.attackPower = std::stof(tokens[ComboNodeType::AttackPower]);
		node.brokenRate = std::stof(tokens[ComboNodeType::BrokenRate]);
		//突進フレーム//開始が空欄なら0(最初から)、終了が空欄なら-1(進行率のデフォルト値を使う)
		node.moveStartFrame = tokens[ComboNodeType::MoveTimeStart].empty() ? 0.0f : std::stof(tokens[ComboNodeType::MoveTimeStart]);
		node.moveEndFrame = tokens[ComboNodeType::MoveTimeEnd].empty() ? -1.0f : std::stof(tokens[ComboNodeType::MoveTimeEnd]);
		node.moveSpeedX = std::stof(tokens[ComboNodeType::MoveSpeedX]);
		node.moveSpeedY = std::stof(tokens[ComboNodeType::MoveSpeedY]);
		//nextWeakAttack(空なら空vector)
		if (!tokens[ComboNodeType::NextLightAttack].empty())
		{
			std::istringstream weakSS(tokens[ComboNodeType::NextLightAttack]);
			std::string idx;
			while (std::getline(weakSS, idx, ';'))
			{
				node.nextWeakAttack.push_back(std::stoi(idx));
			}
		}

		//nextHeavyAttack(空なら空vector)
		if (!tokens[ComboNodeType::NextHeavyAttack].empty())
		{
			std::istringstream heavySS(tokens[ComboNodeType::NextHeavyAttack]);
			std::string idx;
			while (std::getline(heavySS, idx, ';'))
			{
				node.nextHeavyAttack.push_back(std::stoi(idx));
			}
		}
		node.knockBackXZ = std::stof(tokens[ComboNodeType::knockBackXZ]);
		node.knockBackY = std::stof(tokens[ComboNodeType::knockBackY]);
		node.isKirimomi = tokens[ComboNodeType::IsKirimomi] == "1" ? true : false;//CSVの値が1ならtrue、0ならfalse
		node.hitStopTime = tokens[ComboNodeType::HitStopTime].empty() ? 0.0f : std::stof(tokens[ComboNodeType::HitStopTime]);
		node.hitStopFrame = tokens[ComboNodeType::HitStopFrame].empty() ? 0.0f : std::stof(tokens[ComboNodeType::HitStopFrame]);
		node.ownHitStop = tokens[ComboNodeType::OwnHitStop].empty() ? 0.0f : std::stof(tokens[ComboNodeType::OwnHitStop]);
		node.ownHitStopFrame = tokens[ComboNodeType::OwnHitStopFrame].empty() ? 0.0f : std::stof(tokens[ComboNodeType::OwnHitStopFrame]);
		node.allTimeScaleFlag = tokens[ComboNodeType::AllTimeScaleFlag] == "1";
		node.seFrameRate = std::stof(tokens[ComboNodeType::SeFrameRate]);
		node.seName = tokens[ComboNodeType::SeName];
		//当たり判定のフレーム//;で区切って複数書いたら多段ヒット
		{
			std::istringstream startSS(tokens[ComboNodeType::AttackColStartFrame]);
			std::string frame;
			while (std::getline(startSS, frame, ';'))
			{
				node.attackColStartFrames.push_back(std::stof(frame));
			}
			std::istringstream endSS(tokens[ComboNodeType::AttackColEndFrame]);
			while (std::getline(endSS, frame, ';'))
			{
				node.attackColEndFrames.push_back(std::stof(frame));
			}
			//StartとEndの数がずれてたらCSVの書き間違い
			assert(node.attackColStartFrames.size() == node.attackColEndFrames.size() && "attackColStartとattackColEndの数が合ってない");
		}
		//空欄なら-1(総フレーム数を使う)
		node.endFrame = tokens[ComboNodeType::EndFrame].empty() ? -1.0f : std::stof(tokens[ComboNodeType::EndFrame]);
		node.blendFrame = tokens[ComboNodeType::BlendFrame].empty() ? -1.0f : std::stof(tokens[ComboNodeType::BlendFrame]);
		//遷移フレーム//空欄なら-1(進行率のデフォルト値を使う)
		node.comboInputStartFrame = tokens[ComboNodeType::ComboInputStartFrame].empty() ? -1.0f : std::stof(tokens[ComboNodeType::ComboInputStartFrame]);
		node.comboInputEndFrame = tokens[ComboNodeType::ComboInputEndFrame].empty() ? -1.0f : std::stof(tokens[ComboNodeType::ComboInputEndFrame]);
		node.cancelFrame = tokens[ComboNodeType::CancelFrame].empty() ? -1.0f : std::stof(tokens[ComboNodeType::CancelFrame]);
		node.actionCancelFrame = tokens[ComboNodeType::ActionCancelFrame].empty() ? -1.0f : std::stof(tokens[ComboNodeType::ActionCancelFrame]);
		//刀の軌跡を伸ばす区間//開始と終了を別の列に;区切りで(当たり判定の列と同じ)//列が無い・空欄なら出さない
		{
			auto readFrames = [&tokens](int col)
				{
					std::vector<float> frames;
					if (col >= static_cast<int>(tokens.size())) return frames;
					std::string text = tokens[col];
					if (!text.empty() && text.back() == '\r') text.pop_back();
					std::istringstream ss(text);
					std::string frame;
					while (std::getline(ss, frame, ';'))
					{
						if (!frame.empty()) frames.push_back(std::stof(frame));
					}
					return frames;
				};
			const std::vector<float> starts = readFrames(ComboNodeType::SlashStart);
			const std::vector<float> ends = readFrames(ComboNodeType::SlashEnd);
			assert(starts.size() == ends.size() && "slashStartとslashEndの数が合ってない");
			for (size_t i = 0; i < starts.size() && i < ends.size(); ++i)
			{
				node.slashWindows.push_back({ starts[i], ends[i] });
			}
		}
		m_comboChain.push_back(node);
	}

}

void Player::UpdateAngle()
{
	//float targetAngle = 0.0f;//目標の角度
	//if (m_targetVec.Magnitude() > 0.0f)//最初の入力されないとき以外、ここを通り、モデルの向きを変える
	//{
	//	//プレイヤーの移動方向にモデルの方向を近づける
	//	targetAngle = atan2f(m_targetVec.x, m_targetVec.z);//移動ベクトルのx成分とz成分から、プレイヤーの向きたい方向の角度を求める
	//	// Y軸回転行列を作成する//この工程は毎フレーム、原点からプレイヤーの位置に移動してから、回転する行列を作成している
	//	//180度ずれてたので、回転角度を180度ずらす
	//	/* m_rotAngle = targetAngle - DX_PI_F;*/

	//	 //角度の差分を計算//回転角度を-90から90にするため(最短経路を選択)
	//	float difference = targetAngle - m_rotAngleY - DX_PI_F;
	//	while (difference > DX_PI_F) difference -= 2.0f * DX_PI_F;
	//	while (difference < -DX_PI_F) difference += 2.0f * DX_PI_F;
	//	//targetAngle + DX_PI_F
	//	m_rotAngleY += difference * 0.1f;//回転角度を少しずつ目標の角度に近づける//ほぼlerp
	//}	
	//	Matrix4x4 rotY = Matrix4x4::MakeRotationY(m_rotAngleY);
	//	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	//	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);

	//	Matrix4x4 mtx = trans * rotY;
	//	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));
}

void Player::BloodWingUpdate()
{
	//必殺技状態の間だけ出す//スキル攻撃でモデルを消している間は翼も消す
	bool isShow = m_isUltimating && !m_isSkillInvisible;
	if (!isShow)
	{
		if (m_bloodWingPlayingHandle != -1)
		{
			StopEffekseer3DEffect(m_bloodWingPlayingHandle);
			m_bloodWingPlayingHandle = -1;
		}
		if (m_bloodWingActivateHandle != -1)
		{
			StopEffekseer3DEffect(m_bloodWingActivateHandle);
			m_bloodWingActivateHandle = -1;
		}
		m_bloodWingDelay = 0.0f;
		return;
	}
	//発動の演出が終わったらハンドルを捨てる
	if (m_bloodWingActivateHandle != -1 && IsEffekseer3DEffectPlaying(m_bloodWingActivateHandle) != 0)
	{
		m_bloodWingActivateHandle = -1;
	}
	//出始めは翼が広がるアニメーションから再生する//エフェクト側で消えずにループし続ける
	if (m_bloodWingPlayingHandle == -1 && m_bloodWingDelay <= 0.0f)
	{
		m_bloodWingPlayingHandle = PlayEffekseer3DEffect(System::GetInstance().GetHandle(AsyncData::BloodWingEffect));
		SetScalePlayingEffekseer3DEffect(m_bloodWingPlayingHandle, kBloodWingScale, kBloodWingScale, kBloodWingScale);
	}

	//背中(肩甲骨の高さ)のボーンに付ける//アニメーションで体が動いても背中についてくる
	int modelHandle = m_modelHandle;
	int frame = m_bloodWingFrame;
	//ボーンが見つからなかったときはカプセルの頭の高さにする
	Vector3 rootPos = (frame >= 0) ? Vector3(MV1GetFramePosition(modelHandle, frame)) : m_rb.m_pos + Vector3(0.0f, kPlayerHead, 0.0f);

	//モデルの向き//モデルは180度ずれているのでπ足す(スキル3の分身と同じ)
	float yaw = m_rotAngleY + DX_PI_F;
	//エフェクトの回転//エフェクトの+Zを前に向ける//翼はエフェクトの-X側に広がり、-Z側(背中側)に反る
	MATRIX effectRot = MGetRotY(yaw);
	if (frame >= 0)
	{
		//ボーンが初期姿勢からどれだけ傾いたかを、モデルの向きの回転に足す//前かがみやのけぞりにも翼がついてくる
		//tilt = 初期姿勢のボーンの回転の逆 * 今のボーンの回転 * モデルの回転の逆//モデル空間での傾き(初期姿勢なら単位行列)
		MATRIX baseBoneRot = GetRotationOnly(GetFrameBaseModelMatrix(modelHandle, frame));
		MATRIX boneRot = GetRotationOnly(MV1GetFrameLocalWorldMatrix(modelHandle, frame));
		MATRIX modelRot = GetRotationOnly(MV1GetMatrix(modelHandle));
		MATRIX tilt = MMult(MMult(MTranspose(baseBoneRot), boneRot), MTranspose(modelRot));
		tilt = ScaleRotation(tilt, kBloodWingTiltFollowRate);//傾きすぎないように弱める
		//effectRot = 向き * モデルの回転の逆 * 傾き * モデルの回転
		effectRot = MMult(MMult(MMult(effectRot, MTranspose(modelRot)), tilt), modelRot);
	}

	//付け根をボーンから背中側・下にずらす//体の傾きに合わせてずらす向きも回す
	Vector3 offset = VTransformSR(VGet(0.0f, -kBloodWingDownOffset, -kBloodWingBackOffset), effectRot);
	Vector3 pos = rootPos + offset;

	//翼を背中側(カメラ側)に倒す//エフェクトのローカルのX軸回り
	effectRot = MMult(MGetRotX(kBloodWingBackLean), effectRot);

	//Effekseerの回転はZ→X→Yの順に掛かるので、その順のオイラー角に分解して渡す
	float rotX = asinf(std::clamp(-effectRot.m[2][1], -1.0f, 1.0f));
	float rotY = atan2f(effectRot.m[2][0], effectRot.m[2][2]);
	float rotZ = atan2f(effectRot.m[0][1], effectRot.m[1][1]);

	//翼と発動の演出は同じ付け根・向きにする
	for (int handle : { m_bloodWingPlayingHandle, m_bloodWingActivateHandle })
	{
		if (handle == -1)continue;
		SetPosPlayingEffekseer3DEffect(handle, pos.x, pos.y, pos.z);
		SetRotationPlayingEffekseer3DEffect(handle, rotX, rotY, rotZ);
	}
}

float Player::GetUltBladeRate() const
{
	if (!m_weapon)return 1.0f;
	return m_weapon->GetUltBladeRate();
}

void Player::StartBloodWingActivate()
{
	//前の演出・翼が残っていたら消して出し直す
	if (m_bloodWingActivateHandle != -1)StopEffekseer3DEffect(m_bloodWingActivateHandle);
	if (m_bloodWingPlayingHandle != -1)
	{
		StopEffekseer3DEffect(m_bloodWingPlayingHandle);
		m_bloodWingPlayingHandle = -1;
	}
	m_bloodWingActivateHandle = PlayEffekseer3DEffect(System::GetInstance().GetHandle(AsyncData::BloodWingActivateEffect));
	SetScalePlayingEffekseer3DEffect(m_bloodWingActivateHandle, kBloodWingScale, kBloodWingScale, kBloodWingScale);
	m_bloodWingDelay = kBloodWingActivateDelay;
	BloodWingUpdate();//出た瞬間から背中に合わせる
}

void Player::Heal()
{
	//死んでいる・満タンなら使わない
	int maxHp = GetMaxHp();
	if (m_hp <= 0 || m_hp >= maxHp)return;

	m_hp = (std::min)(maxHp, m_hp + static_cast<int>(maxHp * kHealRate));

	//前のエフェクトが残っていたら消して出し直す
	if (m_healEffectHandle != -1)StopEffekseer3DEffect(m_healEffectHandle);
	m_healEffectHandle = PlayEffekseer3DEffect(System::GetInstance().GetHandle(AsyncData::HealEffect));
	SetScalePlayingEffekseer3DEffect(m_healEffectHandle, kHealEffectScale, kHealEffectScale, kHealEffectScale);
	HealEffectUpdate();
}

void Player::HealEffectUpdate()
{
	if (m_healEffectHandle == -1)return;
	//終わったらハンドルを捨てる
	if (IsEffekseer3DEffectPlaying(m_healEffectHandle) != 0)
	{
		m_healEffectHandle = -1;
		return;
	}
	//足元についていく
	SetPosPlayingEffekseer3DEffect(m_healEffectHandle, m_rb.m_pos.x, m_rb.m_pos.y, m_rb.m_pos.z);
}

//回転行列の回転量だけを割合で弱める(回転軸はそのまま)
MATRIX Player::ScaleRotation(const MATRIX& rot, float rate)
{
	float cosAngle = std::clamp((rot.m[0][0] + rot.m[1][1] + rot.m[2][2] - 1.0f) * 0.5f, -1.0f, 1.0f);
	float angle = acosf(cosAngle);
	VECTOR axis = VGet(rot.m[1][2] - rot.m[2][1], rot.m[2][0] - rot.m[0][2], rot.m[0][1] - rot.m[1][0]);
	if (angle < 0.0001f || VSize(axis) < 0.0001f) return MGetIdent();//ほぼ回転していない
	return MGetRotAxis(VNorm(axis), angle * rate);
}

//行列から回転だけを取り出す(拡大縮小と平行移動を除く)
MATRIX Player::GetRotationOnly(const MATRIX& mat)
{
	MATRIX rot = MGetIdent();
	for (int i = 0; i < 3; ++i)
	{
		VECTOR axis = VNorm(VGet(mat.m[i][0], mat.m[i][1], mat.m[i][2]));
		rot.m[i][0] = axis.x;
		rot.m[i][1] = axis.y;
		rot.m[i][2] = axis.z;
	}
	return rot;
}

//アニメーションが付いていない初期姿勢での、フレームのモデル空間の行列
MATRIX Player::GetFrameBaseModelMatrix(int modelHandle, int frameIndex)
{
	MATRIX mat = MGetIdent();
	for (int frame = frameIndex; frame >= 0; frame = MV1GetFrameParent(modelHandle, frame))
	{
		mat = MMult(mat, MV1GetFrameBaseLocalMatrix(modelHandle, frame));//子→親の順に掛ける
	}
	return mat;
}

void Player::ApplyPos()
{
	//モデルの座標を更新する
	CharacterBase::ApplyPos();
	//移動の向きへ回す
	UpdateRotAngle();
	////移動制限
	//for (int i = 0; i < static_cast<int>(WaveNumForPlayer::WaveSize); ++i)
	//{
	//	//最期のiならbreakする//最後のiは、ウェーブがないエリアなので、制限しない
	//	if (i == static_cast<int>(WaveNumForPlayer::WaveSize) - 1)break;
	//	switch (i)
	//	{
	//		case static_cast<int>(WaveNumForPlayer::Wave1):
	//			if (m_isWaveArea[i])
	//			{
	//				if (m_rb.m_pos.z >= kArea1MaxZ)
	//				{
	//					{
	//						m_rb.m_pos.z = kArea1MaxZ;
	//					}
	//				}
	//				//エリア制限のエフェクトを出す
	//				if (m_efAreaMaxPlayingHandle[i] == -1)
	//				{
	//					m_efAreaMaxPlayingHandle[i] = PlayEffekseer3DEffect(m_efAreaMaxHandle[i]);
	//					SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1EfPos.z);
	//				}
	//				else
	//				{
	//					SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1EfPos.z);
	//				}

	//			}
	//			//違うならエフェクトを止める
	//			else
	//			{
	//				if (m_efAreaMinPlayingHandle[i] != -1)
	//				{
	//					StopEffekseer3DEffect(m_efAreaMinPlayingHandle[i]);
	//					m_efAreaMinPlayingHandle[i] = -1;
	//				}
	//				if (m_efAreaMaxPlayingHandle[i] != -1)
	//				{
	//					StopEffekseer3DEffect(m_efAreaMaxPlayingHandle[i]);
	//					m_efAreaMaxPlayingHandle[i] = -1;
	//				}
	//			}
	//			break;
	//			case static_cast<int>(WaveNumForPlayer::Wave2):
	//				if (m_isWaveArea[i])
	//				{
	//					if (m_rb.m_pos.z <= kArea1MaxZ)
	//					{
	//						m_rb.m_pos.z = kArea1MaxZ;
	//					}
	//					if (m_rb.m_pos.z >= kArea2MaxZ)
	//					{
	//						m_rb.m_pos.z = kArea2MaxZ;
	//					}
	//					//エリア制限のエフェクトを出す
	//					//下限
	//					if (m_efAreaMinPlayingHandle[i] == -1)
	//					{
	//						m_efAreaMinPlayingHandle[i] = PlayEffekseer3DEffect(m_efAreaMinHandle[i]);
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMinPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1MaxZ);
	//					}
	//					else
	//					{
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMinPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1EfPos.z);
	//					}
	//					//上限
	//					if (m_efAreaMaxPlayingHandle[i] == -1)
	//					{
	//						m_efAreaMaxPlayingHandle[i] = PlayEffekseer3DEffect(m_efAreaMaxHandle[i]);
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea1MaxZ);
	//					}
	//					else
	//					{
	//						SetPosPlayingEffekseer3DEffect(m_efAreaMaxPlayingHandle[i], kArea1EfPos.x, kArea1EfPos.y, kArea2MaxZ);
	//					}
	//				}	
	//				//違うならエフェクトを止める
	//				else
	//				{
	//					if (m_efAreaMinPlayingHandle[i] != -1)
	//					{
	//						StopEffekseer3DEffect(m_efAreaMinPlayingHandle[i]);
	//						m_efAreaMinPlayingHandle[i] = -1;
	//					}
	//					if (m_efAreaMaxPlayingHandle[i] != -1)
	//					{
	//						StopEffekseer3DEffect(m_efAreaMaxPlayingHandle[i]);
	//						m_efAreaMaxPlayingHandle[i] = -1;
	//					}
	//				}
	//				break;
	//			default:
	//				for (auto& playingHandle : m_efAreaMinPlayingHandle)
	//				{
	//					if (playingHandle != -1)
	//					{
	//						StopEffekseer3DEffect(playingHandle);
	//						playingHandle = -1;
	//					}
	//				}
	//				for (auto& playingHandle : m_efAreaMaxPlayingHandle)
	//				{
	//					if (playingHandle != -1)
	//					{
	//						StopEffekseer3DEffect(playingHandle);
	//						playingHandle = -1;
	//					}
	//				}
	//				break;
	//	}
	//}

	//下方向とのレイキャストで、地面から空中に遷移したかを判定する
	Vector3 causuleTop = m_rb.m_pos + Vector3(0, kGroundCheckCapsuleTopOffset, 0);//カプセルの上端の座標//レイキャストの始点
	Vector3 causuleBottom = m_rb.m_pos + Vector3(0, kGroundCheckCapsuleBottomOffset, 0);//カプセルの下端の座標//レイキャストの終点

	Vector3 rayCastSphere = m_rb.m_pos;
	//auto stage = m_stage.lock();
	//if (stage)
	//{
	//	auto hitDim = MV1CollCheck_Capsule(stage->GetStageModelHandle(), -1, causuleTop.ToDxLibVector(), causuleBottom.ToDxLibVector(), kGroundCheckCapsuleRadius);
	//	//地面にいる判定はCollisionMangerの押し戻し処理の際にしているのでしない
	//	if (hitDim.HitNum > 0)
	//	{
	//	}
	//	//当たっていなかったら、地面にいないと判定する
	//	else
	//	{
	//		//地面にいたら、地面から離れたときの処理をする
	//		if (IsFloor())
	//		{
	//			ChangeState(std::make_shared<PlayerStateFall>(GetWeakPtr()));//落下ステートにする
	//		}

	//		SetIsFloor(false);
	//		m_isGround = false;

	//	}
	//	MV1CollResultPolyDimTerminate(hitDim);
	//}

		//歩いて地面から離れたら落下ステートにする
	if (IsLeftFloor() && !GetBossFinisherState())//トドメのムービーは動きが決まっているので落ちない
	{
		//歩いて地面から離れたときだけコヨーテタイムのジャンプを許可する
		bool canCoyoteJump = std::dynamic_pointer_cast<PlayerStateMove>(m_currentState) != nullptr;
		ChangeState(std::make_shared<PlayerStateFall>(GetWeakPtr(), canCoyoteJump));
	}



	//プレイヤーが地上にいたら空中攻撃をリセット
	if (IsFloor())
	{
		m_comboInfo.isAirAttack = false;
		m_comboInfo.isAirSkillAttack = false;
	}

	BloodWingUpdate();
	m_weapon->Update();//武器の更新
}

void Player::UpdateRotAngle()
{
	float targetAngle = 0.0f;//目標の角度
	if (m_targetVec.Magnitude() > 0.0f)//最初の入力されないとき以外、ここを通り、モデルの向きを変える
	{
		//モデルの移動方向にモデルの方向を近づける
		targetAngle = atan2f(m_targetVec.x, m_targetVec.z);//移動ベクトルのx成分とz成分から、プレイヤーの向きたい方向の角度を求める
		// Y軸回転行列を作成する//この工程は毎フレーム、原点からモデルの位置に移動してから、回転する行列を作成している
		//180度ずれてたので、回転角度を180度ずらす
		/* m_rotAngle = targetAngle - DX_PI_F;*/
		 //角度の差分を計算//回転角度を-90から90にするため(最短経路を選択)
		float difference = targetAngle - m_rotAngleY - DX_PI_F;
		while (difference > DX_PI_F) difference -= 2.0f * DX_PI_F;
		while (difference < -DX_PI_F) difference += 2.0f * DX_PI_F;
		//targetAngle + DX_PI_F
		m_rotAngleY += difference * kRotationLerpFactor;//回転角度を少しずつ目標の角度に近づける//ほぼlerp
	}
}

bool Player::CanSkillAttack(bool changeGauge)
{
	return true;//一旦true

	bool canSkill = m_comboInfo.SkillGauge >= kSkillAttackGaugeCost;

	if (canSkill)
	{
		if (changeGauge)m_comboInfo.SkillGauge -= kSkillAttackGaugeCost;
		return true;
	}

	return false;
}

bool Player::CanUltAttack()
{
	bool canUlt = m_comboInfo.UltGauge >= kMaxGaugeValue;

	if (canUlt)
	{
		m_comboInfo.UltGauge -= kMaxGaugeValue;
		return true;
	}

	return false;
}

bool Player::CanUltFinish()const
{
	//地上のみ
	if (!IsFloor())return false;
	//キャンセルして出せるステート//被弾・死亡・演出中などからは出せない
	return std::dynamic_pointer_cast<PlayerStateIdle>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateMove>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateAttack>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateSkillAttack>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateDashAttack>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateAttackLanding>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateDodge>(m_currentState);
}

void Player::UpdatePlayerHeadLook()
{
	//Idle,Moveのときだけ向ける
	bool isLook = std::dynamic_pointer_cast<PlayerStateIdle>(m_currentState) ||
		std::dynamic_pointer_cast<PlayerStateMove>(m_currentState)           ||
		std::dynamic_pointer_cast<PlayerStateFastRun>(m_currentState);

	//見る先を決める//ロックオンの敵→暗殺対象->内部ターゲットの敵→カメラの向きの順
	Vector3 targetPos;
	auto lockTarget = m_lockOnManager->GetLockTarget();
	auto assasinTarget = GetAssasinTarget();
	auto softTarget = GetSoftTarget();

	//Y軸も変えたいと思ったけど実装敵に後回し
	Vector3 offsetY = Vector3(0.0f, 150.0f, 0.0f);
	//Vector3 offsetY = m_offset;

	if (lockTarget)
	{
		targetPos = lockTarget->GetRigidBody().GetPos() + offsetY;
	}
	else if (assasinTarget)
	{
		targetPos = assasinTarget->GetRigidBody().GetPos() + offsetY;
	}
	else if (softTarget)
	{
		targetPos = softTarget->GetRigidBody().GetPos() + offsetY;
	}
	else
	{
		//見る相手がいないときは正面に戻す//targetPosを(0,0,0)にすると原点の方を向いてしまう
		isLook = false;
	}

	//もし敵との距離が遠かったら頭は回さない(正面に戻す)
	float distance = (m_rb.m_pos - targetPos).Magnitude();
	if (distance > 500.0f)
	{
		isLook = false;
	}


	//頭のボーンを回すのは通常のモデルだけ//Idle,Moveは通常のモデルを使っている
	UpdateHeadLook(m_headFrame, isLook, targetPos);
}

void Player::UpdateGuardBreakChance()
{
	if (m_guardBreakChanceTarget.expired())return;

	//地上の通常攻撃中だけ
	bool canChance = std::dynamic_pointer_cast<PlayerStateAttack>(m_currentState) && IsFloor();
	if (canChance)
	{
		ChangeState(std::make_shared<PlayerStateGuardBreakChance>(GetWeakPtr()));
	}
	m_guardBreakChanceTarget.reset();//持ち越さない
}

void Player::UpdateSoftTarget()
{
	auto target = m_softTarget.lock();
	if (!target)return;

	//死んだら消す
	if (target->GetIsLifeZero())
	{
		ClearSoftTarget();
		return;
	}
	//離れすぎたら消す
	float distance = (target->GetRigidBody().GetPos() - m_rb.m_pos).Magnitude();
	if (distance > kPlayerRockOnRange * kSoftTargetRangeMult)
	{
		ClearSoftTarget();
		return;
	}
	//攻撃中はタイマーを進めない//おそらく空振りの攻撃でもタイマーがとまってしまうため、バグるかも
	if (m_comboInfo.currentComboIndex != ComboIndex::None)
	{
		m_softTargetKeepTimer = kSoftTargetKeepFrame;
		return;
	}
	m_softTargetKeepTimer -= System::GetInstance().GetTimeScale();
	//TImerが過ぎていたら内部ターゲットを解放する
	if (m_softTargetKeepTimer <= 0.0f)
	{

		ClearSoftTarget();
	}

}

