#include "ScreenBloodUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	const char* const kSplatPathFormat = "data/UI/ScreenBlood/splat_";
	const char* const kSprayPathFormat = "data/UI/ScreenBlood/spray_";
	const char* const kVignettePath = "data/UI/bloodUI/ink_vignette.png";

	//血の色(画像は白)
	constexpr int kBloodR = 128;
	constexpr int kBloodG = 2;
	constexpr int kBloodB = 12;
	constexpr int kStainAlpha = 240;

	//画像の中心から根元までの距離(画像の幅に対する割合。MakeScreenBlood.ps1 の形に合わせる)
	constexpr float kSplatRootRate = 0.45f * 0.49f;
	constexpr float kSprayRootRate = 0.82f * 0.49f;

	//大きな飛沫:四隅に塊を置き、画面の中心へ飛ばす(設計解像度)
	constexpr float kCornerMargin = 70.0f;//塊の中心の、画面の角からの距離
	constexpr float kCornerJitter = 40.0f;
	constexpr float kSplatAngleRange = 0.35f;//中心へ向けた向きからずらす最大(ラジアン)
	constexpr float kSplatScaleMin = 0.55f;//1024pxの画像に掛ける
	constexpr float kSplatScaleMax = 0.72f;

	//粒の帯:上か下のふちから、ふちに沿って少し内側へ
	constexpr float kSprayEdgeMargin = 30.0f;
	constexpr float kSprayRootMinX = 260.0f;
	constexpr float kSprayRootMaxX = 1020.0f;
	constexpr float kSprayTiltMin = 0.15f;//ふちから内側へ傾ける角度(ラジアン)
	constexpr float kSprayTiltMax = 0.45f;
	constexpr float kSprayScaleMin = 0.45f;
	constexpr float kSprayScaleMax = 0.6f;
	constexpr int kSprayDelayMax = 3;

	//時間(実フレーム)
	constexpr int kGrowFrame = 3;//広がる
	constexpr float kGrowStartRate = 0.75f;//出始めの大きさ
	constexpr int kLife = 100;//出てから消えるまで
	constexpr int kFadeFrame = 45;//最後のこのフレームで消える

	//ふちの墨
	constexpr int kVignetteAlpha = 110;
	constexpr int kVignetteInFrame = 4;
	constexpr int kVignetteLife = 90;

	constexpr size_t kStainMax = 12;//これを超えたら古いものから消す
}

ScreenBloodUI::~ScreenBloodUI()
{
	for (int handle : m_splatHandles) DeleteGraph(handle);
	for (int handle : m_sprayHandles) DeleteGraph(handle);
	DeleteGraph(m_vignetteHandle);
}

void ScreenBloodUI::Init()
{
	//縮めたときに縁がにじまないように乗算済みアルファで読む
	SetUsePremulAlphaConvertLoad(TRUE);
	for (int i = 0; i < kSplatNum; ++i)
	{
		m_splatHandles[i] = LoadGraph((kSplatPathFormat + std::to_string(i) + ".png").c_str());
	}
	for (int i = 0; i < kSprayNum; ++i)
	{
		m_sprayHandles[i] = LoadGraph((kSprayPathFormat + std::to_string(i) + ".png").c_str());
	}
	m_vignetteHandle = LoadGraph(kVignettePath);
	SetUsePremulAlphaConvertLoad(FALSE);
	m_isVisible = true;
}

void ScreenBloodUI::Update()
{
	auto battleMgr = System::GetInstance().GetBattleMgr();
	if (battleMgr && battleMgr->TakeScreenBloodRequest()) Spawn();

	for (auto& stain : m_stains) ++stain.frame;
	std::erase_if(m_stains, [](const Stain& stain) { return stain.frame >= stain.delay + kLife; });

	if (m_vignetteFrame >= 0 && ++m_vignetteFrame >= kVignetteLife) m_vignetteFrame = -1;
}

void ScreenBloodUI::Draw()
{
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawVignette();
	DrawStains();
	SetDrawBright(255, 255, 255);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

float ScreenBloodUI::RandRange(float min, float max)
{
	return min + (max - min) * GetRand(10000) / 10000.0f;
}

void ScreenBloodUI::Spawn()
{
	constexpr float kW = static_cast<float>(Game::kScreenWidth);
	constexpr float kH = static_cast<float>(Game::kScreenHeight);

	//大きな飛沫:左右に1つずつ(上下はランダム。対角になりやすいように右は左の逆が多め)
	const bool isLeftTop = GetRand(1) == 0;
	const bool isRightTop = (GetRand(3) == 0) ? isLeftTop : !isLeftTop;
	const float corners[2][2] = {
		{ kCornerMargin, isLeftTop ? kCornerMargin : kH - kCornerMargin },
		{ kW - kCornerMargin, isRightTop ? kCornerMargin : kH - kCornerMargin },
	};
	for (const auto& corner : corners)
	{
		const float x = corner[0] + RandRange(-kCornerJitter, kCornerJitter);
		const float y = corner[1] + RandRange(-kCornerJitter, kCornerJitter);
		const float toCenter = std::atan2(kH * 0.5f - y, kW * 0.5f - x);
		AddStain(m_splatHandles[GetRand(kSplatNum - 1)], kSplatRootRate, x, y,
			toCenter + RandRange(-kSplatAngleRange, kSplatAngleRange), RandRange(kSplatScaleMin, kSplatScaleMax), 0);
	}

	//粒の帯:上か下のふちに1本
	const bool isTop = GetRand(1) == 0;
	const bool isToRight = GetRand(1) == 0;
	const float rootX = RandRange(kSprayRootMinX, kSprayRootMaxX);
	const float rootY = isTop ? kSprayEdgeMargin : kH - kSprayEdgeMargin;
	//ふちに沿って、内側へ傾ける
	const float tilt = RandRange(kSprayTiltMin, kSprayTiltMax);
	const float inward = isTop ? 1.0f : -1.0f;
	const float angle = isToRight ? tilt * inward : DX_PI_F - tilt * inward;
	AddStain(m_sprayHandles[GetRand(kSprayNum - 1)], kSprayRootRate, rootX, rootY,
		angle, RandRange(kSprayScaleMin, kSprayScaleMax), 1 + GetRand(kSprayDelayMax - 1));

	//多すぎたら古いものから消す
	if (m_stains.size() > kStainMax)
	{
		m_stains.erase(m_stains.begin(), m_stains.begin() + (m_stains.size() - kStainMax));
	}

	m_vignetteFrame = 0;
}

void ScreenBloodUI::AddStain(int handle, float rootOffset, float rootX, float rootY, float angle, float scale, int delay)
{
	//画像の根元は中心から-Xにあるので、中心を飛ぶ向きへずらす
	int width = 0;
	int height = 0;
	GetGraphSize(handle, &width, &height);
	const float offset = rootOffset * width * scale;
	Stain stain;
	stain.handle = handle;
	stain.x = rootX + std::cos(angle) * offset;
	stain.y = rootY + std::sin(angle) * offset;
	stain.scale = scale;
	stain.angle = angle;
	stain.rootDistance = offset;
	stain.delay = delay;
	m_stains.push_back(stain);
}

void ScreenBloodUI::DrawVignette()
{
	if (m_vignetteFrame < 0)return;

	const float in = (std::min)(1.0f, static_cast<float>(m_vignetteFrame + 1) / kVignetteInFrame);
	const float out = 1.0f - static_cast<float>(m_vignetteFrame) / kVignetteLife;
	SetDrawBright(255, 255, 255);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(kVignetteAlpha * in * out));
	DrawExtendGraphF(0.0f, 0.0f, static_cast<float>(Game::GetScreenWidth()), static_cast<float>(Game::GetScreenHeight()), m_vignetteHandle, TRUE);
}

void ScreenBloodUI::DrawStains()
{
	SetDrawBright(kBloodR, kBloodG, kBloodB);
	const float screenScale = Game::GetScale();
	for (const auto& stain : m_stains)
	{
		const int t = stain.frame - stain.delay;
		if (t < 0)continue;

		//広がる
		const float grow = (std::min)(1.0f, static_cast<float>(t + 1) / kGrowFrame);
		const float growRate = kGrowStartRate + (1.0f - kGrowStartRate) * (1.0f - (1.0f - grow) * (1.0f - grow));
		//消える
		const int fadeStart = kLife - kFadeFrame;
		const float fade = (t < fadeStart) ? 1.0f : 1.0f - static_cast<float>(t - fadeStart) / kFadeFrame;

		//広がるときは根元を中心に広がるように、中心を根元へ寄せる
		const float back = stain.rootDistance * (1.0f - growRate);
		const float x = stain.x - std::cos(stain.angle) * back;
		const float y = stain.y - std::sin(stain.angle) * back;

		SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(kStainAlpha * fade));
		DrawRotaGraphF(Game::ScaleX(x), Game::ScaleY(y), stain.scale * growRate * screenScale, stain.angle, stain.handle, TRUE);
	}
}
