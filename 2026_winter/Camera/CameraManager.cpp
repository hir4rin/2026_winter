#include "CameraManager.h"
#include "Camera.h"
#include "LockOnManager.h"
#include "Character/Player/Player.h"
#include "CameraState/CameraStateBase.h"
#include "CameraState/PlayerFollowCamera.h"
#include "CameraState/LockOnCameraState.h"
#include "CameraState/UltCameraState.h"
#include "CameraState/SkillKessatuCamera.h"
#include "CameraState/FinishingSecondCamera.h"
#include "CameraState/Assasin/AssasinCamera.h"
#include "CameraState/Assasin/AssasinCameraStart.h"
#include "CameraState/Assasin/AirAssasinCamera.h"
#include "CameraState/Assasin/AirAssasinCameraStart.h"
#include "CameraState/PartBroken/PartBrokenACamera.h"
#include "CameraState/PartBroken/PartBrokenACameraStart.h"
#include "CameraState/PartBroken/PartBrokenACameraEnd.h"
#include "CameraState/PartBroken/PartBrokenBCamera.h"
#include "CameraState/PartBroken/PartBrokenBCameraStart.h"
#include "CameraState/BattleSmoothCamera.h"
#include "Collision/CollisionManager.h"
#include "Stage/Stage.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "../Character/Enemy/EnemyBase.h"
#include "EffekseerForDXLib.h"
#include "Main/Input.h"
#include "../Math/Matrix4x4.h"
#include "Main/imguiApp.h"
#include "Main/FrameProfiler.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kPhotoCamSpeed = 20.0f;//フォトモード中のカメラ移動速度
	constexpr float kPhotoAngleSpeed = 0.03f;//フォトモード中のカメラ回転速度
	//constexpr float kPhotoLookDistance = 500.0f;//注視点までの距離
	constexpr float kPhotoAngleVClamp = 0.49f;//フォトモード中の垂直角度の可動範囲(DX_PI_Fに対する倍率)

	constexpr int kShakeRandMax = 200;//カメラ揺れのランダム値の最大
	constexpr float kShakeRandNormalize = 100.0f;//ランダム値を-1.0~1.0の範囲に正規化するための除数

	constexpr float kFovPunchCoolTime = 80.0f;//パンチのクールタイム
	constexpr float kFovOffsetWeightFrame = 10.0f;//演出カメラの出入りでオフセットを出し入れするフレーム数
	constexpr float kFovMin = 10.0f * DX_PI_F / 180.0f;//視野角の下限
	constexpr float kFovMax = 120.0f * DX_PI_F / 180.0f;//視野角の上限

	constexpr float kCollisionMargin = 20.0f;//当たった位置から注視点側に離す距離
	constexpr float kCollisionMinDist = 30.0f;//注視点に近づける限界
	constexpr float kCollisionReturnRate = 0.1f;//離れるときの戻る速さ

	//カクつきの記録
	constexpr float kSnapLogDist = 5.0f;//当たり判定でこれ以上一瞬で寄ったら記録する
	constexpr float kJumpMoveChange = 20.0f;//カメラの1フレームの移動が前のフレームからこれ以上変わったら記録する
	constexpr float kJumpTurnChange = 2.0f;//カメラの向きの1フレームの回転(度)が前のフレームからこれ以上変わったら記録する
}

CameraManager::CameraManager()
{
	m_context = std::make_shared<CameraContext>();

}

CameraManager::~CameraManager()
{
}


void CameraManager::Init(std::weak_ptr<Player> player,std::weak_ptr<Stage> stage)
{
	//プレイヤーカメラの初期化
	m_context->m_player = player;
	m_stage = stage;

	////PlayerCameraでスタート
	ChangeState(std::make_shared<PlayerFollowCamera>(shared_from_this()));
}

void CameraManager::Update()
{


	// DXライブラリのカメラとEffekseerのカメラを同期する。
	Effekseer_Sync3DSetting();

	//ロックオン状態だったらロックオンカメラへ移行させる
	SyncLockOnState();

	m_currentState->Update();
	//カメラに反映//stageとの当たり判定もする
	Vector3 renderPos = FixStageCollision(m_currentState->GetPos(), m_currentState->GetTarget());
	renderPos += CameraShakeUpdate();
	m_renderPos = renderPos;

	m_renderFov = m_currentState->GetFov() + FovOffsetUpdate() + FovPunchUpdate();
	m_renderFov = std::clamp(m_renderFov, kFovMin, kFovMax);
	CheckCameraJump(m_currentState->GetTarget());

	SetCameraPositionAndTarget_UpVecY(m_renderPos.ToDxLibVector(), m_currentState->GetTarget().ToDxLibVector());
	SetupCamera_Perspective(m_renderFov);
	return;

}

Vector3 CameraManager::FixStageCollision(const Vector3& pos, const Vector3& target)
{
	m_collisionSnap = 0.0f;
	auto stage = m_stage.lock();
	if (!stage || !IsStageCollisionCamera() || !Switch().isStageCollision)
	{
		m_collisionDist = -1.0f;
		return pos;
	}

	Vector3 toCamera = pos - target;
	float rawDist = toCamera.Magnitude();
	if (rawDist <= kCollisionMinDist) return pos;
	Vector3 dir = toCamera.Normalize();

	//注視点からカメラへ線分を引く
	float goalDist = rawDist;
	auto hit = MV1CollCheck_Line(stage->GetStageModelHandle(), -1, target.ToDxLibVector(), pos.ToDxLibVector());
	if (hit.HitFlag)
	{
		float hitDist = (Vector3::FromDxLibVector(hit.HitPosition) - target).Magnitude();
		goalDist = (std::max)(hitDist - kCollisionMargin, kCollisionMinDist);
	}

	//寄るときはすぐで、離れるときはゆっくり
	//今の判定よりステージとのほうが短いときは寄る
	if (m_collisionDist < 0.0f || goalDist < m_collisionDist)
	{
		const float prevDist = m_collisionDist < 0.0f ? rawDist : m_collisionDist;
		if (hit.HitFlag && prevDist - goalDist >= kSnapLogDist)
		{
			m_collisionSnap = prevDist - goalDist;
			if (Switch().isLog) FrameProfiler::GetInstance().LogEvent("collision snap %.0f -> %.0f (raw %.0f)", prevDist, goalDist, rawDist);
		}
		m_collisionDist = goalDist;
	}
	else
	{
		
		m_collisionDist += (goalDist - m_collisionDist) * kCollisionReturnRate;
	}
	m_collisionDist = (std::min)(m_collisionDist, rawDist);

	return target + dir * m_collisionDist;
}

bool CameraManager::IsStageCollisionCamera() const
{
	//ここはよう悩み//一旦全部の判定をする
	return true;


	auto type = m_currentState->GetCameraType();
	return type == CameraStateBase::Type::PlayerCamera ||
		type == CameraStateBase::Type::LockOnCamera ||
		type == CameraStateBase::Type::BattleSmoothCamera;
}
bool CameraManager::IsFovOffsetCamera() const
{
	auto type = m_currentState->GetCameraType();
	return type == CameraStateBase::Type::PlayerCamera ||
		type == CameraStateBase::Type::LockOnCamera ||
		type == CameraStateBase::Type::BattleSmoothCamera;
}
CameraManager::CheckSwitch& CameraManager::Switch()
{
	static CheckSwitch checkSwitch;
	return checkSwitch;
}

void CameraManager::CheckCameraJump(const Vector3& target)
{
	const Vector3 toTarget = target - m_renderPos;
	if (toTarget.Magnitude() <= 0.0f)return;
	const Vector3 dir = toTarget.Normalize();
	if (!m_hasLastCamera)
	{
		m_hasLastCamera = true;
		m_lastRenderPos = m_renderPos;
		m_lastDir = dir;
		return;
	}

	//1フレームの移動と回転が、前のフレームから急に変わったら記録する(なめらかに動いているなら差は小さい)
	const Vector3 move = m_renderPos - m_lastRenderPos;
	const float turn = std::acos(std::clamp(dir.Dot(m_lastDir), -1.0f, 1.0f)) * 180.0f / DX_PI_F;
	const float moveChange = (move - m_lastMove).Magnitude();
	const float turnChange = std::abs(turn - m_lastTurn);
	if (Switch().isLog && (moveChange >= kJumpMoveChange || turnChange >= kJumpTurnChange))
	{
		++m_jumpCount;
		FrameProfiler::GetInstance().LogEvent("JUMP move %.0f (prev %.0f, change %.0f) turn %.1f (prev %.1f) | %s snap %.0f shake %.1f punch %.1f timescale %.2f",
			move.Magnitude(), m_lastMove.Magnitude(), moveChange, turn, m_lastTurn,
			GetCameraTypeName(static_cast<int>(m_currentState->GetCameraType())), m_collisionSnap,
			m_shakeTimer > 0.0f ? m_shakePower : 0.0f, m_fovPunchOffset * 180.0f / DX_PI_F, System::GetInstance().GetTimeScale());
	}
	m_lastRenderPos = m_renderPos;
	m_lastMove = move;
	m_lastDir = dir;
	m_lastTurn = turn;
}

const char* CameraManager::GetCameraTypeName(int type)
{
	switch (static_cast<CameraStateBase::Type>(type))
	{
	case CameraStateBase::Type::PlayerCamera: return "PlayerCamera";
	case CameraStateBase::Type::UltCamera: return "UltCamera";
	case CameraStateBase::Type::LockOnCamera: return "LockOnCamera";
	case CameraStateBase::Type::FinishingSecondCamera: return "FinishingSecondCamera";
	case CameraStateBase::Type::AssasinCamera: return "AssasinCamera";
	case CameraStateBase::Type::AssasinCameraStart: return "AssasinCameraStart";
	case CameraStateBase::Type::PartBrokenCameraA: return "PartBrokenCameraA";
	case CameraStateBase::Type::PartBrokenCameraAStart: return "PartBrokenCameraAStart";
	case CameraStateBase::Type::PartBrokenCameraAEnd: return "PartBrokenCameraAEnd";
	case CameraStateBase::Type::PartBrokenCameraB: return "PartBrokenCameraB";
	case CameraStateBase::Type::PartBrokenCameraBStart: return "PartBrokenCameraBStart";
	case CameraStateBase::Type::SkillKessatuCamera: return "SkillKessatuCamera";
	case CameraStateBase::Type::AirAssasinCamera: return "AirAssasinCamera";
	case CameraStateBase::Type::AirAssasinCameraStart: return "AirAssasinCameraStart";
	case CameraStateBase::Type::BattleSmoothCamera: return "BattleSmoothCamera";
	default: return "Other";
	}
}

void CameraManager::DrawDebugWindow()
{
	CheckSwitch& s = Switch();
	ImGui::Begin("Camera Check");
	ImGui::Checkbox("Stage Collision", &s.isStageCollision);
	ImGui::Checkbox("Shake", &s.isShake);
	ImGui::Checkbox("FOV Punch", &s.isFovPunch);
	ImGui::Checkbox("Log (Log/CameraEvents.txt)", &s.isLog);
	ImGui::Text("Jump %d / Frame %d", m_jumpCount, FrameProfiler::GetInstance().GetFrameCount());
	ImGui::Text("State %s", GetCameraTypeName(static_cast<int>(m_currentState->GetCameraType())));
	ImGui::Text("Collision dist %.0f", m_collisionDist);
	ImGui::End();
}

void CameraManager::Draw()
{
}
void CameraManager::ApplyCameraSettings()
{
	//ImGuiのカメラアニメーターで"Execute"を押して再生中なら、保存したキーフレーム間を補間した座標を
	//そのままDxLibに渡す(Override Cameraよりもこちらを優先する)
	//(Debug/Release問わず判定する。Release版ではF2キーでウィンドウを開かない限りこれらのフラグはtrueにならない)
	if (imguiApp::GetInstance().IsCameraAnimatorPlaying())
	{
		SetCameraPositionAndTarget_UpVecY(
			imguiApp::GetInstance().GetAnimatorCameraPos().ToDxLibVector(),
			imguiApp::GetInstance().GetAnimatorCameraTarget().ToDxLibVector());
		SetupCamera_Perspective(CameraStateBase::kDefaultFov);
		Effekseer_Sync3DSetting();
		return;
	}

	//ImGuiのカメラデバッグウィンドウで"Override Camera"がONになっているなら、
	//下にある通常のカメラ計算(フォトモード判定やStateの計算結果)をすべて無視して、
	//ImGui側で編集した座標・注視点をそのままDxLibに渡す
	if (imguiApp::GetInstance().IsCameraOverrideEnabled())
	{
		SetCameraPositionAndTarget_UpVecY(
			imguiApp::GetInstance().GetOverrideCameraPos().ToDxLibVector(),
			imguiApp::GetInstance().GetOverrideCameraTarget().ToDxLibVector());
		SetupCamera_Perspective(imguiApp::GetInstance().GetOverrideCameraFov());
		//DxLib側のカメラ情報が変わったので、Effekseer(エフェクト)側のカメラ情報も合わせて更新する
		//(これをしないとエフェクトの見た目・位置がカメラとズレる)
		Effekseer_Sync3DSetting();
		return;//ここで抜けるので、この下の通常カメラ処理(フォトモード判定など)は実行されない
	}

	//フォトモード・ステージ編集中は、Stateが計算した値ではなく、フリーカメラの座標をそのままDxLibに渡す
	auto battleMgr = System::GetInstance().GetBattleMgr();
	if (battleMgr->GetPhotoMode() || battleMgr->GetStageEditMode())
	{
		SetCameraPositionAndTarget_UpVecY(m_photoCamPos.ToDxLibVector(), m_photoCamTarget.ToDxLibVector());
		SetupCamera_Perspective(imguiApp::GetInstance().GetOverrideCameraFov());
		Effekseer_Sync3DSetting();
		return;
	}

	//m_mainCamera->CameraSetting();
	SetCameraPositionAndTarget_UpVecY(m_renderPos.ToDxLibVector(), m_currentState->GetTarget().ToDxLibVector());
	SetupCamera_Perspective(m_renderFov);
	//カメラ変更を反映させる
	Effekseer_Sync3DSetting();
}

void CameraManager::UpdatePhotoCamera()
{
	auto& input = Input::GetInstance();

	//右スティックで向きを変える
	auto rightStick = input.GetRightStickInput();
	m_photoAngleH += rightStick.x * kPhotoAngleSpeed;
	m_photoAngleV -= rightStick.y * kPhotoAngleSpeed;
	m_photoAngleV = std::clamp(m_photoAngleV, -DX_PI_F * kPhotoAngleVClamp, DX_PI_F * kPhotoAngleVClamp);

	//向きから前方向・右方向のベクトルを作る(atan2f(x,z)の対応の逆)
	Vector3 forward = Vector3(sinf(m_photoAngleH), 0.0f, cosf(m_photoAngleH));
	Vector3 right = Vector3(cosf(m_photoAngleH), 0.0f, -sinf(m_photoAngleH));

	//十字キーで水平移動する
	Vector3 moveDir = Vector3();
	if (input.IsPressed("Up"))    moveDir = moveDir + forward;
	if (input.IsPressed("Down"))  moveDir = moveDir + forward * -1.0f;
	if (input.IsPressed("Right")) moveDir = moveDir + right;
	if (input.IsPressed("Left"))  moveDir = moveDir + right * -1.0f;
	if (moveDir.Magnitude() > 0.0f)
	{
		m_photoCamPos = m_photoCamPos + moveDir.Normalize() * kPhotoCamSpeed;
	}

	//RB/LBで上下移動する
	if (input.IsPressed("RB")) m_photoCamPos.y += kPhotoCamSpeed;
	if (input.IsPressed("LB")) m_photoCamPos.y -= kPhotoCamSpeed;

	//注視点は、固定の前方ベクトルをm_photoAngleH,m_photoAngleVで回転させる
	Vector3 baseDir = Vector3(0.0f, 0.0f, m_kToTargetDistance);//常に同じ、前フレームの結果を使わない
	auto rotY = Matrix4x4::MakeRotationY(m_photoAngleH);
	auto rotX = Matrix4x4::MakeRotationX(m_photoAngleV);

	auto baseDirDx = baseDir.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);
	auto rotXMat = Matrix4x4::ToDxLibMatrix(rotX);

	auto rotated = VTransform(baseDirDx, rotXMat);
	rotated = VTransform(rotated, rotYMat);

	auto pos = VAdd(rotated, m_photoCamPos.ToDxLibVector());
	m_photoCamTarget = Vector3::FromDxLibVector(pos);



	//Vector3 lookDir = Vector3(
	//	sinf(m_photoAngleH) * cosf(m_photoAngleV),
	//	sinf(m_photoAngleV),
	//	cosf(m_photoAngleH) * cosf(m_photoAngleV));
	//m_photoCamTarget = m_photoCamPos + lookDir * kPhotoLookDistance;
}

void CameraManager::SetPhotoCamera()
{
	//angleや座標を保存
	m_photoCamPos = GetActiveCamera()->GetPos();
	m_photoCamTarget = GetActiveCamera()->GetTarget();

	m_photoAngleH = GetActiveCamera()->GetCameraAngleH();
	m_photoAngleV = GetActiveCamera()->GetCameraAngleV();

	//注視点までの距離を計算
	m_kToTargetDistance = (m_photoCamPos - m_photoCamTarget).Magnitude();

}




void CameraManager::StartCameraShake(float power, float time)
{
	if (!Switch().isShake)return;
	if (Switch().isLog) FrameProfiler::GetInstance().LogEvent("shake power %.1f time %.0f", power, time);
	m_shakePower = power;
	m_shakeTimer = time;
	m_shakeTimerMax = time;
	m_isShaking = true;
}

Vector3 CameraManager::CameraShakeUpdate()
{
	if (m_shakeTimer <= 0.0f)
	{
		m_isShaking = false;
		return Vector3();
	}
	m_shakeTimer -= 1.0f * System::GetInstance().GetTimeScale();//

	float progress = m_shakeTimer / m_shakeTimerMax;//揺れの進行度合いを0から1の範囲で表す
	float currentPower = m_shakePower * progress;//現在の揺れの強さを計算する

	float magX = (GetRand(kShakeRandMax) / kShakeRandNormalize - 1.0f) * currentPower;
	float magY = (GetRand(kShakeRandMax) / kShakeRandNormalize - 1.0f) * currentPower;
	Vector3 mag = Vector3(magX, magY, 0.0f);

	return mag;


}

void CameraManager::StartFovPunch(float angleDeg, float time, float inFrame, float outPower)
{
	if (!Switch().isFovPunch)return;
	float angle = angleDeg * DX_PI_F / 180.0f;
	//クールタイム中は無視
	if (m_fovPunchCoolTimer > 0.0f)return;
	//今より弱いなら無視
	if (std::abs(angle) < std::abs(m_fovPunchOffset))return;
	if (Switch().isLog) FrameProfiler::GetInstance().LogEvent("fov punch %.1f deg time %.0f in %.0f", angleDeg, time, inFrame);

	m_fovPunchAngle = angle;
	m_fovPunchTimer = 0.0f;
	m_fovPunchMaxTime = time;
	m_fovPunchInFrame = inFrame;
	m_fovPunchOutPower = outPower;
	m_fovPunchCoolTimer = kFovPunchCoolTime;
}

float CameraManager::FovPunchUpdate()
{
	//クールタイムはパンチが終わっていても減らす
	m_fovPunchCoolTimer = (std::max)(m_fovPunchCoolTimer - 1.0f, 0.0f);

	//最大時間を超えたら終わる
	if (m_fovPunchTimer >= m_fovPunchMaxTime)
	{
		m_fovPunchOffset = 0.0f;
		return 0.0f;
	}
	//タイムスケールとかをするならここにかける(いまはしない設計
	m_fovPunchTimer += 1.0f;

	float rate = 0.0f;
	if (m_fovPunchTimer < m_fovPunchInFrame)
	{
		//入り:一瞬で最大へ
		rate = m_fovPunchTimer / m_fovPunchInFrame;
	}
	else
	{
		//戻り:ゆっくり0へ
		float outFrame = (std::max)(m_fovPunchMaxTime - m_fovPunchInFrame, 1.0f);
		float t = std::clamp((m_fovPunchTimer - m_fovPunchInFrame) / outFrame, 0.0f, 1.0f);
		rate = 1.0f - Easing::Apply(EasingMode::EaseOut, t, m_fovPunchOutPower);
	}

	m_fovPunchOffset = m_fovPunchAngle * rate;
	return m_fovPunchOffset;
}

void CameraManager::SetFovOffset(float angleDeg, float blendFrame)
{
	//今の値から変え始める//途中で変えてもカクつかない
	m_fovOffsetStart = m_fovOffset;
	m_fovOffsetGoal = angleDeg * DX_PI_F / 180.0f;
	m_fovOffsetTimer = 0.0f;
	m_fovOffsetBlendFrame = blendFrame;
}

void CameraManager::ResetFovOffset(float blendFrame)
{
	SetFovOffset(0.0f, blendFrame);
}

float CameraManager::FovOffsetUpdate()
{
	//実フレームで進める
	if (m_fovOffsetTimer < m_fovOffsetBlendFrame)
	{
		m_fovOffsetTimer += 1.0f;
		float t = std::clamp(m_fovOffsetTimer / m_fovOffsetBlendFrame, 0.0f, 1.0f);
		m_fovOffset = std::lerp(m_fovOffsetStart, m_fovOffsetGoal, Easing::Apply(EasingMode::EaseOut, t, 2.0f));
	}
	else
	{
		m_fovOffset = m_fovOffsetGoal;
	}

	//通常のカメラなら効き具合を1へ、演出カメラなら0へ
	if (IsFovOffsetCamera())
	{
		m_fovOffsetWeight += 1.0f / kFovOffsetWeightFrame;
	}
	else
	{
		//違うならだんだん0にする
		m_fovOffsetWeight -= 1.0f / kFovOffsetWeightFrame;
	}
	m_fovOffsetWeight = std::clamp(m_fovOffsetWeight, 0.0f, 1.0f);

	return m_fovOffset * m_fovOffsetWeight;
}

bool CameraManager::IsLockOn()const
{
	auto player = m_context->m_player.lock();
	return player && player->IsLockOn();
}

std::shared_ptr<EnemyBase> CameraManager::GetLockTarget() const
{
	auto player = m_context->m_player.lock();
	if (!player)return nullptr;
	auto lockOn = player->GetLockOnManager();
	if (lockOn)
	{
		return lockOn->GetLockTarget();
	}
	return nullptr;
}

std::shared_ptr<EnemyBase> CameraManager::GetAttackTarget() const
{
	auto player = m_context->m_player.lock();
	if (!player)return nullptr;
	return player->GetAttackTarget();
}


void CameraManager::ChangeState(std::shared_ptr<CameraStateBase> newState)
{
	//同じStateなら遷移しない
	if (m_currentState&&
		m_currentState->GetCameraType() == newState->GetCameraType())
	{
		return;
	}

	if (Switch().isLog)
	{
		const int prevType = m_currentState ? static_cast<int>(m_currentState->GetCameraType()) : -1;
		FrameProfiler::GetInstance().LogEvent("state %s -> %s", GetCameraTypeName(prevType), GetCameraTypeName(static_cast<int>(newState->GetCameraType())));
	}

	// 保持
	//prevcamera
	if (m_currentState)
	{
		m_currentState->Exit();
	}

	CameraStateBase::CameraData data = { };


	if (m_currentState)
	{
		data.pos = m_currentState->GetPos();
		data.target = m_currentState->GetTarget();
		data.angleH = m_currentState->GetCameraAngleH();
		data.angleV = m_currentState->GetCameraAngleV();
		data.fov = m_currentState->GetFov();
		data.overrideSetting = m_currentState->GetOverrideBlendSetting();
	}

	m_currentState = newState;

	if (m_currentState)
	{
		m_currentState->SetStartFov(data.fov);
		m_currentState->Enter(data);
	}

}

void CameraManager::InitState(std::shared_ptr<CameraStateBase> newState)
{
	m_currentState = newState;

	CameraStateBase::CameraData data;

	if (m_currentState)
	{
		m_currentState->Enter(data);
	}
}

void CameraManager::ChangeStateFromScene(CameraStateName stateName)
{
	std::shared_ptr<CameraStateBase> newState;
	switch (stateName)
	{
	case CameraStateName::PlayerCaemra:
		newState = std::make_shared<PlayerFollowCamera>(shared_from_this());
		break;
	case CameraStateName::LockOnCamera:
		newState = std::make_shared<LockOnCameraState>(shared_from_this());
		break;
	case CameraStateName::UltCamera:
		newState = std::make_shared<UltCameraState>(shared_from_this());
		break;
	case CameraStateName::SkillKessatuCamera:
		newState = std::make_shared<SkillKessatuCamera>(shared_from_this());
		break;
	case CameraStateName::FinishingSecondCamera:
		newState = std::make_shared<FinishingSecondCamera>(shared_from_this());
		break;
	case CameraStateName::AssasinCamera:
		newState = std::make_shared<AssasinCameraState>(shared_from_this());
		break;
	case CameraStateName::AssasinCameraStart:
		newState = std::make_shared<AssasinCameraStart>(shared_from_this());
		break;
	case CameraStateName::AirAssasinCamera:
		newState = std::make_shared<AirAssasinCameraState>(shared_from_this());
		break;
	case CameraStateName::AirAssasinCameraStart:
		newState = std::make_shared<AirAssasinCameraStart>(shared_from_this());
		break;
	case CameraStateName::PartBrokenACamera:
		newState = std::make_shared<PartBrokenACamera>(shared_from_this());
		break;
	case CameraStateName::PartBrokenACameraStart:
		newState = std::make_shared<PartBrokenACameraStart>(shared_from_this());
		break;
	case CameraStateName::PartBrokenACameraEnd:
		newState = std::make_shared<PartBrokenACameraEnd>(shared_from_this());
		break;
	case CameraStateName::PartBrokenBCamera:
		newState = std::make_shared<PartBrokenBCamera>(shared_from_this());
		break;
	case CameraStateName::PartBrokenBCameraStart:
		newState = std::make_shared<PartBrokenBCameraStart>(shared_from_this());
		break;
	case CameraStateName::BattleSmoothCamera:
		newState = std::make_shared<BattleSmoothCamera>(shared_from_this());
		break;
	default:
		return;

	}

	ChangeState(newState);
}

void CameraManager::OnPlayerAttackHit()
{
	if (!m_currentState)return;
	//PlayerCamera以外(LockOnCamera、UltCamera、演出カメラなど)の時は遷移しない
	if (m_currentState->GetCameraType() != CameraStateBase::Type::PlayerCamera)return;

	ChangeStateFromScene(CameraStateName::BattleSmoothCamera);
}

void CameraManager::SyncLockOnState()
{	
	if (!m_currentState)return;
	auto type = m_currentState->GetCameraType();
	bool isLockOn = IsLockOn();

	//追従カメラとロックオンカメラの間だけ切り替える(必殺技・演出カメラには触らない)
	if (isLockOn && (type == CameraStateBase::Type::PlayerCamera || type == CameraStateBase::Type::BattleSmoothCamera))
	{
		ChangeStateFromScene(CameraStateName::LockOnCamera);
	}
	else if (!isLockOn && type == CameraStateBase::Type::LockOnCamera)
	{
		ChangeStateFromScene(CameraStateName::PlayerCaemra);
	}
}


