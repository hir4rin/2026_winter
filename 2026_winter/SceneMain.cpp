#include "SceneMain.h"
#include "DxLib.h"

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
}

SceneMain::SceneMain() :
	m_frameCount(0)
{
}

SceneMain::~SceneMain()
{
}

void SceneMain::Init()
{
	//3Dカメラの位置と注視点を設定する
	SetCameraPositionAndTarget_UpVecY(VGet(500.0f, 500.0f, -500.0f), VGet(0.0f, 0.0f, 0.0f));
}

void SceneMain::Update()
{
	m_frameCount++;
}

void SceneMain::Draw()
{
	DrawGrid();
	DrawString(0, 0, "SceneMain", GetColor(255, 255, 255));
	DrawFormatString(0, 16, GetColor(255, 255, 255), "FRAME:%d", m_frameCount);
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
