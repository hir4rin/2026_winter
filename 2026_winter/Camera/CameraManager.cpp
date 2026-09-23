#include "CameraManager.h"
#include "Camera.h"
#include "LockOnManager.h"
#include "CameraState/CameraStateBase.h"
#include "CameraState/PlayerFollowCamera.h"
#include "CameraState/LockOnCameraState.h"
#include "CameraState/UltCameraState.h"
#include "CameraState/TitleCameraState.h"
#include "CameraState/FinishingFirstCamera.h"
#include "CameraState/FinishingSecondCamera.h"
#include "CameraState/ResultCameraState.h"
#include "../Managers/CollisionManager.h"
#include "../System.h"
#include "../BattleManager.h"
#include "../Character/Enemy/EnemyBase.h"
#include "EffekseerForDXLib.h"
#include "../Input.h"
#include "../Math/Matrix4x4.h"
//#include "../imguiApp.h"
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
	SetWeakRef(player);

	////PlayerCameraでスタート
	ChangeState(std::make_shared<PlayerFollowCamera>(shared_from_this()));
}

void CameraManager::Update(Vector3 pos, Vector3 pos2)
{


	// DXライブラリのカメラとEffekseerのカメラを同期する。
	Effekseer_Sync3DSetting();

	m_currentState->Update();
	//カメラに反映
	Vector3 renderPos = m_currentState->GetPos();
	renderPos += CameraShakeUpdate();
	m_renderPos = renderPos;

	SetCameraPositionAndTarget_UpVecY(m_renderPos.ToDxLibVector(), m_currentState->GetTarget().ToDxLibVector());
	return;
	
}
void CameraManager::Draw()
{
}
void CameraManager::ApplyCameraSettings()
{
	//ImGuiのカメラアニメーターで"Execute"を押して再生中なら、保存したキーフレーム間を補間した座標を
	//そのままDxLibに渡す(Override Cameraよりもこちらを優先する)
	//(Debug/Release問わず判定する。Release版ではF2キーでウィンドウを開かない限りこれらのフラグはtrueにならない)
	//imguiApp.hが存在しないため一旦コメントアウト
	//if (imguiApp::GetInstance().IsCameraAnimatorPlaying())
	//{
	//	SetCameraPositionAndTarget_UpVecY(
	//		imguiApp::GetInstance().GetAnimatorCameraPos().ToDxLibVector(),
	//		imguiApp::GetInstance().GetAnimatorCameraTarget().ToDxLibVector());
	//	Effekseer_Sync3DSetting();
	//	return;
	//}

	//ImGuiのカメラデバッグウィンドウで"Override Camera"がONになっているなら、
	//下にある通常のカメラ計算(フォトモード判定やStateの計算結果)をすべて無視して、
	//ImGui側で編集した座標・注視点をそのままDxLibに渡す
	//if (imguiApp::GetInstance().IsCameraOverrideEnabled())
	//{
	//	SetCameraPositionAndTarget_UpVecY(
	//		imguiApp::GetInstance().GetOverrideCameraPos().ToDxLibVector(),
	//		imguiApp::GetInstance().GetOverrideCameraTarget().ToDxLibVector());
	//	//DxLib側のカメラ情報が変わったので、Effekseer(エフェクト)側のカメラ情報も合わせて更新する
	//	//(これをしないとエフェクトの見た目・位置がカメラとズレる)
	//	Effekseer_Sync3DSetting();
	//	return;//ここで抜けるので、この下の通常カメラ処理(フォトモード判定など)は実行されない
	//}

	//フォトモード中は、Stateが計算した値ではなく、フリーカメラの座標をそのままDxLibに渡す
	if (System::GetInstance().GetBattleMgr()->GetPhotoMode())
	{
		SetCameraPositionAndTarget_UpVecY(m_photoCamPos.ToDxLibVector(), m_photoCamTarget.ToDxLibVector());
		Effekseer_Sync3DSetting();
		return;
	}

	//m_mainCamera->CameraSetting();
	SetCameraPositionAndTarget_UpVecY(m_renderPos.ToDxLibVector(), m_currentState->GetTarget().ToDxLibVector());
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


void CameraManager::SetWeakRef(std::weak_ptr<Player> m_player, std::weak_ptr<EnemyBase> m_enemy)
{
	//カメラコンテキストにセット
	m_context->m_player = m_player;
	//m_context->m_targetEnemy = m_enemy;
}

std::shared_ptr<EnemyBase> CameraManager::GetTargetEnemy()const
{
	auto lockOnManager = m_lockOnManager.lock();
	if (!lockOnManager)return nullptr;
	return lockOnManager->GetTarget().lock();
}
void CameraManager::StartCameraShake(float power, float time)
{
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
	m_shakeTimer -= 1.0;//

	float progress = m_shakeTimer / m_shakeTimerMax;//揺れの進行度合いを0から1の範囲で表す
	float currentPower = m_shakePower * progress;//現在の揺れの強さを計算する

	float magX = (GetRand(kShakeRandMax) / kShakeRandNormalize - 1.0f) * currentPower;
	float magY = (GetRand(kShakeRandMax) / kShakeRandNormalize - 1.0f) * currentPower;
	Vector3 mag = Vector3(magX, magY, 0.0f);

	return mag;


}

void CameraManager::ChangeState(std::shared_ptr<CameraStateBase> newState)
{
	//同じStateなら遷移しない
	if (m_currentState&&
		m_currentState->GetCameraType() == newState->GetCameraType())
	{
		return;
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
		data.overrideSetting = m_currentState->GetOverrideBlendSetting();
	}

	m_currentState = newState;

	if (m_currentState)
	{
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
	case CameraStateName::TitleCamera:
		newState = std::make_shared<TitleCameraState>(shared_from_this());
		break;
	case CameraStateName::FinishingFirstCamera:
		newState = std::make_shared<FinishingFirstCamera>(shared_from_this());
		break;
	case CameraStateName::FinishingSecondCamera:
		newState = std::make_shared<FinishingSecondCamera>(shared_from_this());
		break;
	case CameraStateName::ResultCamera:
		newState = std::make_shared<ResultCameraState>(shared_from_this());
		break;
	default:
		return;

	}

	ChangeState(newState);
}


