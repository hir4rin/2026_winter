#include "SceneMain.h"
#include "DxLib.h"
#include <cmath>
#include "Character/CharacterBase.h"

namespace
{
	//グリッドの範囲と間隔
	constexpr float kGridRange = 500.0f;
	constexpr float kGridSpan = 100.0f;

	//軸線の長さ
	constexpr float kAxisLength = 700.0f;

	//色
	constexpr unsigned int kGridLineColor = 0x808080;//グレー
	constexpr unsigned int kAxisColorX = 0xff0000;//赤
	constexpr unsigned int kAxisColorY = 0x00ff00;//緑
	constexpr unsigned int kAxisColorZ = 0x0000ff;//青

	//テスト用モデル
	const char* const kModelPath = "data/Character.mv1";

	//アニメーション名(data/PlayerAnim.xlsx の animName 列と同じ)
	const char* const kAnimIdle = "Ninja_Idle_Chudan";//Idle
	const char* const kAnimWalk = "Ninja_QuickDash_Forward_Loop";//Walk
	const char* const kAnimFrontRoll = "Ninja_Acrobatic_Frontflip";//前転
	const char* const kAnimBackRoll = "Ninja_Acrobatic_Backflip";//後転

	constexpr float kRollMoveSpeed = 4.0f;//転がっている間の1フレームの移動量(水平の移動量はアニメから除去しているため)

	//カメラ設定(2026_summerのCamera.cppに合わせる)
	constexpr float kCameraViewAngle = DX_PI_F / 3.0f;
	constexpr float kCameraNear = 100.0f;
	constexpr float kCameraFar = 1500.0f;
	const VECTOR kCameraOffset = { 500.0f, 500.0f, -500.0f };//モデルからカメラへのオフセット

	constexpr float kMoveSpeed = 5.0f;//1フレームの移動量
	constexpr float kRotateLerpRate = 0.2f;//向きを目標に近づける割合
	constexpr float kAnimSpeed = 0.5f;//アニメーションの進む速さ(30fpsのクリップを60fpsで再生)
	constexpr float kBlendFrames = 10.0f;//アニメーションを切り替えるフレーム数
}

SceneMain::SceneMain() :
	m_frameCount(0)
{
}

SceneMain::~SceneMain()
{
	MV1DeleteModel(m_modelHandle);
	if (m_lightHandle != -1)
	{
		DeleteLightHandle(m_lightHandle);
		m_lightHandle = -1;
	}
}

void SceneMain::Init()
{
	//3Dカメラの視野角・クリップ範囲を設定する(2026_summerのCamera.cppと同じ設定)
	SetupCamera_Perspective(kCameraViewAngle);
	SetCameraNearFar(kCameraNear, kCameraFar);
	UpdateCamera();

	//ディレクショナルライトをカメラ→注視点の方向に向ける(2026_summerのCamera.cppと同じ設定)
	m_lightHandle = CreateDirLightHandle(VNorm(VScale(kCameraOffset, -1.0f)));

	//テスト用モデルの読み込み
	m_modelHandle = MV1LoadModel(kModelPath);
	MV1SetScale(m_modelHandle, VGet(1.0f, 1.0f, 1.0f));

	ChangeAnim(kAnimIdle);
}

VECTOR SceneMain::GetMoveInput() const
{
	VECTOR dir = VGet(0.0f, 0.0f, 0.0f);
	if (CheckHitKey(KEY_INPUT_UP))    dir.z += 1.0f;//奥
	if (CheckHitKey(KEY_INPUT_DOWN))  dir.z -= 1.0f;//手前
	if (CheckHitKey(KEY_INPUT_RIGHT)) dir.x += 1.0f;
	if (CheckHitKey(KEY_INPUT_LEFT))  dir.x -= 1.0f;

	if (VSquareSize(dir) > 0.0f) dir = VNorm(dir);
	return dir;
}

void SceneMain::UpdateRollInput()
{
	bool isZPressed = CheckHitKey(KEY_INPUT_Z) != 0;
	bool isTriggered = isZPressed && !m_wasZPressed;
	m_wasZPressed = isZPressed;

	if (!isTriggered || m_isRolling) return;

	//Z=前転、Z+↓キー=後転
	m_isRollBack = CheckHitKey(KEY_INPUT_DOWN) != 0;
	m_isRolling = true;
	ChangeAnim(m_isRollBack ? kAnimBackRoll : kAnimFrontRoll, false);
}

void SceneMain::UpdateMove()
{
	UpdateRollInput();

	if (m_isRolling)
	{
		//向いている方向(-Zが正面)に前転は進み、後転は下がる
		VECTOR front = VGet(-sinf(m_rotY), 0.0f, -cosf(m_rotY));
		float speed = m_isRollBack ? -kRollMoveSpeed : kRollMoveSpeed;
		m_pos = VAdd(m_pos, VScale(front, speed));

		MV1SetPosition(m_modelHandle, m_pos);
		MV1SetRotationXYZ(m_modelHandle, VGet(0.0f, m_rotY, 0.0f));
		return;
	}

	VECTOR dir = GetMoveInput();
	bool isMoving = VSquareSize(dir) > 0.0f;

	if (isMoving)
	{
		m_pos = VAdd(m_pos, VScale(dir, kMoveSpeed));

		//移動方向にモデルを向ける(2026_summerのCharacterBaseと同じく、モデルは-Zが正面)
		float targetAngle = atan2f(dir.x, dir.z) + DX_PI_F;
		float diff = targetAngle - m_rotY;
		while (diff > DX_PI_F) diff -= 2.0f * DX_PI_F;
		while (diff < -DX_PI_F) diff += 2.0f * DX_PI_F;
		m_rotY += diff * kRotateLerpRate;
	}

	ChangeAnim(isMoving ? kAnimWalk : kAnimIdle);

	MV1SetPosition(m_modelHandle, m_pos);
	MV1SetRotationXYZ(m_modelHandle, VGet(0.0f, m_rotY, 0.0f));
}

void SceneMain::ChangeAnim(const char* animName, bool isLoop)
{
	//ループするアニメーションは、すでに再生中なら何もしない(1回きりの転がりは毎回頭から再生する)
	if (isLoop && m_currentAnimName == animName) return;

	int animIndex = MV1GetAnimIndex(m_modelHandle, animName);
	if (animIndex == -1)
	{
		//名前が見つからないときは今のアニメーションのまま。画面に表示して気づけるようにする
		m_isAnimMissing = true;
		m_currentAnimName = animName;
		if (!isLoop) m_isRolling = false;//転がりが終わらないまま固まらないようにする
		return;
	}
	m_isAnimMissing = false;
	m_isAnimLoop = isLoop;

	//ブレンド中に切り替わったときは、古い方を捨てる
	if (m_prevAnimHandle != -1)
	{
		MV1DetachAnim(m_modelHandle, m_prevAnimHandle);
	}
	m_prevAnimHandle = m_animHandle;
	m_prevAnimCount = m_animCount;
	m_prevAnimTotal = m_animTotal;

	m_animHandle = MV1AttachAnim(m_modelHandle, animIndex, -1, -1);
	m_animCount = 0.0f;
	m_animTotal = MV1GetAttachAnimTotalTime(m_modelHandle, m_animHandle);
	m_blendRate = (m_prevAnimHandle == -1) ? 1.0f : 0.0f;
	m_currentAnimName = animName;
}

void SceneMain::UpdateAnim()
{
	if (m_animHandle == -1) return;

	m_animCount += kAnimSpeed;
	if (m_animCount >= m_animTotal)
	{
		if (m_isAnimLoop)
		{
			m_animCount -= m_animTotal;
		}
		else
		{
			//1回きりのアニメーション(転がり)は最後で止めて、入力を受け付けるように戻す
			m_animCount = m_animTotal;
			m_isRolling = false;
		}
	}
	MV1SetAttachAnimTime(m_modelHandle, m_animHandle, m_animCount);

	if (m_prevAnimHandle != -1)
	{
		m_prevAnimCount += kAnimSpeed;
		if (m_prevAnimCount >= m_prevAnimTotal) m_prevAnimCount -= m_prevAnimTotal;
		MV1SetAttachAnimTime(m_modelHandle, m_prevAnimHandle, m_prevAnimCount);

		m_blendRate += 1.0f / kBlendFrames;
		if (m_blendRate >= 1.0f)
		{
			m_blendRate = 1.0f;
			MV1DetachAnim(m_modelHandle, m_prevAnimHandle);
			m_prevAnimHandle = -1;
		}
		else
		{
			MV1SetAttachAnimBlendRate(m_modelHandle, m_prevAnimHandle, 1.0f - m_blendRate);
		}
	}
	MV1SetAttachAnimBlendRate(m_modelHandle, m_animHandle, m_blendRate);
}

void SceneMain::UpdateCamera()
{
	SetCameraPositionAndTarget_UpVecY(VAdd(m_pos, kCameraOffset), m_pos);
}

void SceneMain::Update()
{
	m_frameCount++;

	UpdateMove();
	UpdateAnim();
	UpdateCamera();
}

void SceneMain::Draw()
{
	DrawGrid();

	MV1DrawModel(m_modelHandle);

	DrawString(0, 0, "SceneMain  [ARROW] move  [Z] front roll  [Z+DOWN] back roll", GetColor(255, 255, 255));
	DrawFormatString(0, 16, GetColor(255, 255, 255), "FRAME:%d", m_frameCount);
	DrawModelDebugInfo();
}


void SceneMain::DrawModelDebugInfo()
{
	DrawFormatString(0, 32, GetColor(255, 255, 255), "anim=%s  pos=(%.0f, %.0f)",
		m_currentAnimName ? m_currentAnimName : "-", m_pos.x, m_pos.z);
	if (m_isAnimMissing)
	{
		DrawFormatString(0, 48, GetColor(255, 80, 80), "ANIM NOT FOUND in Character.mv1: %s", m_currentAnimName);
	}

	//mv1に入っているアニメーション名の一覧(名前の確認用)
	int animNum = MV1GetAnimNum(m_modelHandle);
	DrawFormatString(0, 64, GetColor(255, 255, 255), "animNum=%d", animNum);
	for (int i = 0; i < animNum && i < 10; ++i)
	{
		DrawFormatString(0, 80 + i * 16, GetColor(200, 200, 200), " [%d] %s", i, MV1GetAnimName(m_modelHandle, i));
	}
}

void SceneMain::DrawGrid()
{
	//直線の始点と終点
	VECTOR startPos;
	VECTOR endPos;

	//XZ平面のグリッド線を描画する
	for (float z = -kGridRange; z <= kGridRange; z += kGridSpan)
	{
		startPos = VGet(-kGridRange, 0.0f, z);
		endPos = VGet(kGridRange, 0.0f, z);
		DrawLine3D(startPos, endPos, kGridLineColor);
	}
	for (float x = -kGridRange; x <= kGridRange; x += kGridSpan)
	{
		startPos = VGet(x, 0.0f, -kGridRange);
		endPos = VGet(x, 0.0f, kGridRange);
		DrawLine3D(startPos, endPos, kGridLineColor);
	}

	//X軸(赤)
	startPos = VGet(-kAxisLength, 0.0f, 0.0f);
	endPos = VGet(kAxisLength, 0.0f, 0.0f);
	DrawLine3D(startPos, endPos, kAxisColorX);

	//Y軸(緑)
	startPos = VGet(0.0f, -kAxisLength, 0.0f);
	endPos = VGet(0.0f, kAxisLength, 0.0f);
	DrawLine3D(startPos, endPos, kAxisColorY);

	//Z軸(青)
	startPos = VGet(0.0f, 0.0f, -kAxisLength);
	endPos = VGet(0.0f, 0.0f, kAxisLength);
	DrawLine3D(startPos, endPos, kAxisColorZ);
}
