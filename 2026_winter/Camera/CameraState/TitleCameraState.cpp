#include "TitleCameraState.h"
#include "../CameraManager.h"
#include "Player.h"

namespace
{
	constexpr float kToOriginLength = 200.0f;//原点からカメラまでの距離
	constexpr float kCameraHeight = 100.0f;//カメラの高さ

	const Vector3 kFollowCameraOffset = Vector3(-150.0f, 60.0f, 0.0f);//追従時の、プレイヤーから見たカメラの相対位置
	const Vector3 kFollowTargetOffset = Vector3(0.0f, 30.0f, 0.0f);//追従時の、プレイヤーから見た注視点の相対位置//足元を見せる

	// ズームアウト用
	constexpr float kZoomOutFrame = 200.0f; // ズームアウトにかけるフレーム数

	// 開始時(現在の足元アップと同じ)カメラオフセット
	const Vector3 kZoomOutCameraOffsetStart = kFollowCameraOffset;
	// 終了時(引いてキャラ全体が入る)カメラオフセット。距離を離し、高さも上げる
	const Vector3 kZoomOutCameraOffsetEnd = Vector3(-200.0f, 140.0f, 0.0f);

	// 開始時の注視点オフセット(足元)
	const Vector3 kZoomOutTargetOffsetStart = kFollowTargetOffset;
	// 終了時の注視点オフセット(体の中心あたりに上げる)
	const Vector3 kZoomOutTargetOffsetEnd = Vector3(0.0f, 100.0f, 0.0f);

	constexpr float kTargetMoveFrame = 60.0f;
	constexpr float kTargetMoveDisX = 1300.0f;
	constexpr float kTargetMoveDisY = 1050.0f;
	constexpr float kTargetMoveDisZ = 1000.0f;

	constexpr float kCameraViewAngle = DX_PI_F / 3.0f;//カメラの視野角
	constexpr float kCameraNear = 0.0f;//ニアクリップ
	constexpr float kCameraFar = 1500.0f;//ファークリップ

	constexpr float kFixedShotDepth = 5050.0f;//定点カット(Fixed)のZ座標の深さ
	constexpr float kOpeningLerpFactor = 0.8f;//オープニング演出のカメラ位置ラープ係数
}

TitleCameraState::TitleCameraState(std::weak_ptr<CameraManager> owner) : CameraStateBase(owner)
{
}

TitleCameraState::~TitleCameraState()
{
}

void TitleCameraState::Enter(CameraData data)
{
	m_pos = Vector3(0.0f, kCameraHeight, -kToOriginLength);
	m_target = Vector3(0.0f, 100.0f, 0.0f);//原点を見る


	//カメラの設定
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLibVector(), m_target.ToDxLibVector());
	SetupCamera_Perspective(kCameraViewAngle);
	SetCameraNearFar(kCameraNear, kCameraFar);
}

void TitleCameraState::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;
	Vector3 pos = player->GetRigidBody().GetPos();

	switch (m_shot)
	{
	case Shot::Fixed:
		//定点で映すだけなので、座標・注視点は固定で更新の必要はない
		m_pos = Vector3(-kToOriginLength, kCameraHeight, -kFixedShotDepth);
		m_target = Vector3(0.0f, 100.0f, -kFixedShotDepth);//原点を見る

	/*	m_pos = pos + kFollowCameraOffset + Vector3(0,100,-100.0f);
		m_target = pos + kFollowTargetOffset+Vector3(0,100,0);*/
		break;
	case Shot::FollowPlayer:
		//プレイヤーとの相対位置を保ったまま追従する
		m_pos = pos + kFollowCameraOffset;
		m_target = pos + kFollowTargetOffset;
		break;
	case Shot::ZoomOut:
	{
		float t = m_count_zoom / kZoomOutFrame; // 0.0〜1.0の進行度
		if (t > 1.0f) t = 1.0f;

		// カメラオフセットを補間
		Vector3 cameraOffset = kZoomOutCameraOffsetStart + (kZoomOutCameraOffsetEnd - kZoomOutCameraOffsetStart) * t;
		// 注視点オフセットを補間
		Vector3 targetOffset = kZoomOutTargetOffsetStart + (kZoomOutTargetOffsetEnd - kZoomOutTargetOffsetStart) * t;

		// プレイヤーの現在位置を基準に反映(追従しつつズームアウト)
		m_pos = pos + cameraOffset;
		m_target = pos + targetOffset;
		m_count_zoom++;
		if (m_count_zoom >= kZoomOutFrame)
		{
			//m_count = 0;
			//m_shot = Shot::FollowPlayer; // ズームアウト後、通常の追従に移行(必要に応じて変更)
		}
	}
		break;
	case Shot::Opening:
		m_pos = Vector3::Lerp(m_pos, pos + kFollowCameraOffset, kOpeningLerpFactor);

		m_target.x += kTargetMoveDisX / kTargetMoveFrame;
		m_target.y += kTargetMoveDisY / kTargetMoveFrame;
		m_target.z += -kTargetMoveDisZ / kTargetMoveFrame;
		m_count++;
		if(m_count >= kTargetMoveFrame)
					{
			m_count = 0;
			m_shot = Shot::Finish;
		}
		break;
	case Shot::Finish:
		break;
	}

	//カメラの位置と注視点を反映する
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLibVector(), m_target.ToDxLibVector());
}

void TitleCameraState::Exit()
{
}

void TitleCameraState::SkipToOpening(const Vector3& playerPos)
{
	//ZoomOutが完了した直後の状態(オフセットが終了値になった状態)を再現する
	m_pos = playerPos + kZoomOutCameraOffsetEnd;
	m_target = playerPos + kZoomOutTargetOffsetEnd;

	m_count = 0;
	m_shot = Shot::Opening;
}

void TitleCameraState::Draw()
{
}

void TitleCameraState::CameraSetting()
{
	//カメラの位置と注視点を反映する
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLibVector(), m_target.ToDxLibVector());
}
