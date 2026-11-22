#include "PlayerStateStartMovie.h"
#include "Character/Player/Player.h"
#include "Camera/CameraManager.h"
#include "Battle/BattleManager.h"
#include "Main/System.h"
#include "EffekseerForDXLib.h"
#include <algorithm>
#include <cmath>

namespace
{
	using F = PlayerStartMovieFrame;
	constexpr float kLiftOffFrame = F::kJumpStartFrame + 8.0f;//JumpUpで体が浮くフレーム
	constexpr float kAttackBeforeApex = 10.0f;//真上に着く何フレーム前に空中強攻撃を振りかぶるか

	constexpr float kLeapEasingPower = 2.0f;
	constexpr float kPlungeEasingPower = 2.5f;

	//落ちてくる間の煙の尾・火の粉・風の筋(列車から落ちるときと同じ)
	const char* const kFallTrailEffectPath = "data/Effect/Train/TrainFallTrail.efk";
	constexpr float kFallTrailScale = 20.0f;

	//着地の衝撃(閃光・火花・ひび・衝撃の輪・土煙・破片)//1/20で作ったエフェクト
	const char* const kLandEffectPath = "data/Effect/StartMovie/StartLand.efk";
	constexpr float kLandEffectScale = 20.0f;

	constexpr float kBlendFrame = 5.0f;
	constexpr float kLandAnimSpeed = 0.3f;//着地のポーズを長く見せる
	constexpr float kLandAnimEndFrame = 19.0f;//PlayerStateAttackLandingと同じ
	constexpr float kStandBlendFrame = 30.0f;

	constexpr float kShakePower = 12.0f;
	constexpr float kShakeTime = 24.0f;
	constexpr float kFovPunchAngle = 8.0f;
	constexpr float kFovPunchTime = 24.0f;
	constexpr float kFovPunchInFrame = 2.0f;
	constexpr float kFovPunchOutPower = 2.0f;
}

PlayerStateStartMovie::PlayerStateStartMovie(std::weak_ptr<Player> player, const PlayerStartMovieFrame& frame) :
	PlayerState(player),
	m_frame(frame)
{
}

PlayerStateStartMovie::~PlayerStateStartMovie()
{
}

void PlayerStateStartMovie::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	System::GetInstance().GetBattleMgr()->SetIsEventPlaying(true);

	//屋上・空中は当たり判定が無く、通りの見えない壁も抜けるので着地まで切る
	player->SetIsActive(false);
	player->m_rb.m_vel = Vector3(0, 0, 0);
	player->m_targetVec = m_frame.front;
	player->SetRotY(atan2f(m_frame.front.x, m_frame.front.z) - DX_PI_F);
	m_frameCount = 0.0f;
	m_isAttackAnim = false;
	m_landEffect = LoadEffekseerEffect(kLandEffectPath);

	if (m_frame.IsFall())
	{
		//空から落ちてくる//最初から空中強攻撃の構え
		SetPos(*player, m_frame.FallStartPos());
		const ComboNode& node = player->GetComboNode(ComboIndex::AirHeavyAttack1);
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame, 0.0f, 0.0f);
		m_isAttackAnim = true;
		m_fallTrailEffect = LoadEffekseerEffect(kFallTrailEffectPath);
		m_fallTrailPlaying = PlayEffekseer3DEffect(m_fallTrailEffect);
		SetScalePlayingEffekseer3DEffect(m_fallTrailPlaying, kFallTrailScale, kFallTrailScale, kFallTrailScale);
		UpdateFallTrail(*player);
		m_phase = Phase::Fall;
		return;
	}

	SetPos(*player, m_frame.RoofPos());
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Idle"), true, 1.0f, -1.0f, 0.0f, 0.0f);
	m_phase = Phase::Roof;
}

void PlayerStateStartMovie::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	//実フレームで進める(カメラと合わせる)
	m_frameCount += 1.0f;
	player->m_rb.m_vel = Vector3(0, 0, 0);

	switch (m_phase)
	{
	case Phase::Roof:
		if (m_frameCount >= F::kJumpStartFrame)
		{
			player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JumpUp"), false, 1.0f, -1.0f, 0.0f, kBlendFrame);
			m_phase = Phase::Leap;
		}
		break;
	case Phase::Leap:
		UpdateLeap(*player);
		break;
	case Phase::Fall:
		UpdateFall(*player);
		break;
	case Phase::Plunge:
		UpdatePlunge(*player);
		break;
	case Phase::Land:
		if (player->m_anim.GetAnimEndFlag())
		{
			player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Idle"), true, 1.0f, -1.0f, 0.0f, kStandBlendFrame);
			m_phase = Phase::Stand;
		}
		break;
	case Phase::Stand:
		break;
	}

	UpdateFallTrail(*player);

	if (m_frameCount >= m_frame.TotalFrame())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
		return;
	}

	player->m_anim.Update();
}

void PlayerStateStartMovie::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->SetIsActive(true);
	System::GetInstance().GetBattleMgr()->SetIsEventPlaying(false);
	StopFallTrail();
	if (m_fallTrailEffect != -1) DeleteEffekseerEffect(m_fallTrailEffect);
	m_fallTrailEffect = -1;
	if (m_landEffect != -1) DeleteEffekseerEffect(m_landEffect);
	m_landEffect = -1;
}

void PlayerStateStartMovie::DebugDraw()
{
}

void PlayerStateStartMovie::UpdateLeap(Player& player)
{
	StartAttackAnim(player);

	if (m_frameCount < kLiftOffFrame)return;

	//着地点の真上へ、上がりながら速度を落とす
	const float s = std::clamp((m_frameCount - kLiftOffFrame) / (m_frame.ApexFrame() - kLiftOffFrame), 0.0f, 1.0f);
	const float eased = Easing::Apply(EasingMode::EaseOut, s, kLeapEasingPower);
	SetPos(player, Vector3::Lerp(m_frame.RoofPos(), m_frame.ApexPos(), eased));

	if (s >= 1.0f)m_phase = Phase::Plunge;
}

void PlayerStateStartMovie::UpdateFall(Player& player)
{
	StartAttackAnim(player);

	//斜め上から着地点の真上へまっすぐ落ちる
	const float s = std::clamp(m_frameCount / m_frame.ApexFrame(), 0.0f, 1.0f);
	SetPos(player, Vector3::Lerp(m_frame.FallStartPos(), m_frame.ApexPos(), s));

	if (s >= 1.0f)m_phase = Phase::Plunge;
}

void PlayerStateStartMovie::StartAttackAnim(Player& player)
{
	//振りかぶり//空中強攻撃と同じアニメ・速さ・止めるフレーム
	if (m_isAttackAnim || m_frameCount < m_frame.ApexFrame() - kAttackBeforeApex)return;
	const ComboNode& node = player.GetComboNode(ComboIndex::AirHeavyAttack1);
	player.m_anim.ChangeAnimWithModelHandle(player.m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame, 0.0f, kBlendFrame);
	m_isAttackAnim = true;
}

void PlayerStateStartMovie::UpdatePlunge(Player& player)
{
	//真下へ加速しながら落ちる
	const float apexFrame = m_frame.ApexFrame();
	const float s = std::clamp((m_frameCount - apexFrame) / (m_frame.LandFrame() - apexFrame), 0.0f, 1.0f);
	const float eased = Easing::Apply(EasingMode::EaseIn, s, kPlungeEasingPower);
	SetPos(player, Vector3::Lerp(m_frame.ApexPos(), m_frame.origin, eased));

	if (s >= 1.0f)Land(player);
}

void PlayerStateStartMovie::Land(Player& player)
{
	SetPos(player, m_frame.origin);
	//煙の尾は新しく出すのだけ止める(出た煙は残って、着地の煙と一緒に消える)
	if (m_fallTrailPlaying != -1) GetEffekseer3DManager()->StopRoot(m_fallTrailPlaying);
	PlayLandEffect();
	player.SetIsActive(true);
	player.SetIsFloor(true);
	player.m_isGround = true;

	player.m_anim.ChangeAnimWithModelHandle(player.m_modelHandle, player.GetAnimName("HeavyAttackLanding"), false, kLandAnimSpeed, kLandAnimEndFrame, 0.0f, kBlendFrame);

	if (auto camera = player.GetCameraManager().lock())
	{
		camera->StartCameraShake(kShakePower, kShakeTime);
		camera->StartFovPunch(kFovPunchAngle, kFovPunchTime, kFovPunchInFrame, kFovPunchOutPower);
	}
	m_phase = Phase::Land;
}

void PlayerStateStartMovie::SetPos(Player& player, const Vector3& pos)
{
	//当たり判定を切っている間はApplyPosが呼ばれないので、モデルもここで動かす
	player.m_rb.m_pos = pos;
	player.UpdateAngleAndPos();
}

void PlayerStateStartMovie::UpdateFallTrail(Player& player)
{
	//足元についていく(煙と火の粉はその場に残って尾になる)
	if (m_fallTrailPlaying == -1)return;
	const Vector3 foot = player.GetRigidBody().GetPos();
	SetPosPlayingEffekseer3DEffect(m_fallTrailPlaying, foot.x, foot.y, foot.z);
}

void PlayerStateStartMovie::StopFallTrail()
{
	if (m_fallTrailPlaying != -1) StopEffekseer3DEffect(m_fallTrailPlaying);
	m_fallTrailPlaying = -1;
}

void PlayerStateStartMovie::PlayLandEffect()
{
	if (m_landEffect == -1)return;
	const int playing = PlayEffekseer3DEffect(m_landEffect);
	SetScalePlayingEffekseer3DEffect(playing, kLandEffectScale, kLandEffectScale, kLandEffectScale);
	SetPosPlayingEffekseer3DEffect(playing, m_frame.origin.x, m_frame.origin.y, m_frame.origin.z);
}
