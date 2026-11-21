#include "TrainOutside.h"
#include "SkyBox.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <string>

namespace
{
	const char* const kSkyDir = "data/Stage/CyberStage/Sky";//ステージ1の夜の空(遠くの街)
	const char* const kLampPath = "data/Stage/TrainTrackside/TrackLamp.mv1";//Unity の TrackPropTool.Export で書き出し

	//線路脇の灯り(CP_Street_Lamp_03。原点は柱の根元、腕は -Z 向き)
	constexpr float kLampScale = 2.0f;
	constexpr float kLampX = 1500.0f;//線路の真ん中から柱まで
	constexpr float kLampStart = 600.0f;//出口から最初の灯りまで
	constexpr float kLampSpacing = 1300.0f;//左右交互に置く間隔
	constexpr float kLampHeadY = 495.0f;//灯りの頭の高さ(モデルの単位)
	constexpr float kLampHeadReach = 261.0f;//柱から灯りの頭まで(モデルの単位)
	constexpr float kLampColorScale = 0.8f;
	constexpr float kLampDrawRange = 5000.0f;//カメラから前後この距離まで(カメラのファー)

	//灯りの頭の光の粒
	constexpr int kLampGlowR = 255;
	constexpr int kLampGlowG = 180;
	constexpr int kLampGlowB = 110;
	constexpr float kLampGlowSize = 260.0f;
	constexpr float kLampGlowAlpha = 0.85f;

	//遠くの街の光の粒(空の絵の窓に重ねる)
	constexpr int kCityRowTop = 220;//窓を探す絵の行(1024の中)
	constexpr int kCityRowBottom = 640;
	constexpr int kCityContrast = 50;//周り(3ピクセル離れた4点)よりこれだけ明るい点を窓にする
	constexpr int kCityGlowPerFace = 140;
	constexpr unsigned int kCitySeed = 11;
	constexpr float kCityDistance = 4300.0f;//カメラからの距離(ファーより手前)
	constexpr float kCitySizeMin = 22.0f;
	constexpr float kCitySizeMax = 55.0f;
	constexpr float kCityAlpha = 0.9f;
	constexpr float kTwinkleSpeed = 0.0012f;//1ミリ秒あたり
	constexpr float kTwinkleRate = 0.45f;//瞬きで暗くなる割合

	constexpr int kGlowGraphSize = 64;
}

TrainOutside::TrainOutside()
{
}

TrainOutside::~TrainOutside()
{
	if (m_lampHandle != -1) MV1DeleteModel(m_lampHandle);
	if (m_glowGraph != -1) DeleteGraph(m_glowGraph);
}

void TrainOutside::Init(const Vector3& stagePos, float baseY, float forwardZ)
{
	m_stagePos = stagePos;
	m_baseY = baseY;
	m_forwardZ = forwardZ;

	m_skyBox = std::make_unique<SkyBox>();
	m_skyBox->Init(kSkyDir);

	m_lampHandle = MV1LoadModel(kLampPath);
	MV1SetScale(m_lampHandle, VGet(kLampScale, kLampScale, kLampScale));
	const COLOR_F scale = GetColorF(kLampColorScale, kLampColorScale, kLampColorScale, 1.0f);
	MV1SetAmbColorScale(m_lampHandle, scale);
	MV1SetDifColorScale(m_lampHandle, scale);

	LoadCityGlows();
	MakeGlowGraph();
}

void TrainOutside::CollectLamps(float exitZ, float centerZ, float range, std::vector<Lamp>& out)const
{
	out.clear();
	//出口から進む向きに測った距離で、左右交互に並べる(出口と一緒に流れる)
	const float center = (centerZ - exitZ) * m_forwardZ;
	const int first = (std::max)(0, static_cast<int>(std::ceil((center - range - kLampStart) / kLampSpacing)));
	const int last = static_cast<int>(std::floor((center + range - kLampStart) / kLampSpacing));
	for (int i = first; i <= last; ++i)
	{
		const float side = (i % 2 == 0) ? 1.0f : -1.0f;
		const float z = exitZ + m_forwardZ * (kLampStart + i * kLampSpacing);
		Lamp lamp;
		lamp.base = Vector3(m_stagePos.x + side * kLampX, m_stagePos.y + m_baseY, z);
		lamp.head = lamp.base + Vector3(-side * kLampHeadReach * kLampScale, kLampHeadY * kLampScale, 0.0f);
		lamp.rotY = side * DX_PI_F * 0.5f;//腕(-Z)を線路の方へ向ける
		out.push_back(lamp);
	}
}

void TrainOutside::GetLampLights(float exitZ, float centerZ, float range, std::vector<Vector3>& out)const
{
	CollectLamps(exitZ, centerZ, range, m_lampBuffer);
	out.clear();
	for (const auto& lamp : m_lampBuffer) out.push_back(lamp.head);
}

void TrainOutside::DrawSky()const
{
	if (m_rate <= 0.0f) return;
	SetFogEnable(FALSE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * m_rate));
	m_skyBox->Draw();
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetFogEnable(TRUE);
}

void TrainOutside::DrawLamps(float exitZ)const
{
	if (m_lampHandle == -1) return;
	CollectLamps(exitZ, GetCameraPosition().z, kLampDrawRange, m_lampBuffer);
	for (const auto& lamp : m_lampBuffer)
	{
		MV1SetPosition(m_lampHandle, lamp.base.ToDxLibVector());
		MV1SetRotationXYZ(m_lampHandle, VGet(0.0f, lamp.rotY, 0.0f));
		ModelShaderManager::GetInstance().DrawModel(m_lampHandle);
	}
}

void TrainOutside::DrawGlows(float exitZ)const
{
	//灯りの柱の陰では隠れるように深さは見るが、書かない
	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(FALSE);
	SetUseLighting(FALSE);
	SetDrawMode(DX_DRAWMODE_BILINEAR);

	//灯りの頭(柱は抜ける前から見えるので、いつも描く)
	CollectLamps(exitZ, GetCameraPosition().z, kLampDrawRange, m_lampBuffer);
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * kLampGlowAlpha));
	SetDrawBright(kLampGlowR, kLampGlowG, kLampGlowB);
	for (const auto& lamp : m_lampBuffer)
	{
		DrawBillboard3D(lamp.head.ToDxLibVector(), 0.5f, 0.5f, kLampGlowSize, 0.0f, m_glowGraph, TRUE);
	}

	//遠くの街(空と一緒に出す)
	if (m_rate > 0.0f)
	{
		const VECTOR cam = GetCameraPosition();
		const float time = static_cast<float>(GetNowCount());
		for (const auto& glow : m_cityGlows)
		{
			const float twinkle = 1.0f - kTwinkleRate * (0.5f + 0.5f * std::sin(time * kTwinkleSpeed + glow.phase));
			SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * glow.alpha * twinkle * m_rate));
			SetDrawBright(glow.r, glow.g, glow.b);
			DrawBillboard3D(VAdd(cam, VScale(glow.dir, kCityDistance)), 0.5f, 0.5f, glow.size, 0.0f, m_glowGraph, TRUE);
		}
	}

	SetDrawBright(255, 255, 255);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetUseLighting(TRUE);
	SetWriteZBuffer3D(TRUE);
}

void TrainOutside::LoadCityGlows()
{
	//SkyBox::Draw の面の置き方と同じ(絵の左上→右上の向きと、上→下の向き)
	struct Face
	{
		const char* file;
		VECTOR center;//面の真ん中の向き
		VECTOR right;//絵の右
	};
	const Face faces[] = {
		{ "front", VGet(0, 0, 1), VGet(1, 0, 0) },
		{ "back", VGet(0, 0, -1), VGet(-1, 0, 0) },
		{ "right", VGet(1, 0, 0), VGet(0, 0, -1) },
		{ "left", VGet(-1, 0, 0), VGet(0, 0, 1) },
	};
	std::mt19937 rand(kCitySeed);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	m_cityGlows.clear();
	for (const auto& face : faces)
	{
		const std::string path = std::string(kSkyDir) + "/" + face.file + ".png";
		const int soft = LoadSoftImage(path.c_str());
		if (soft == -1) continue;
		int w = 0, h = 0;
		GetSoftImageSize(soft, &w, &h);
		auto lum = [&](int x, int y)
			{
				int r, g, b, a;
				GetPixelSoftImage(soft, x, y, &r, &g, &b, &a);
				return (r + g * 2 + b) / 4;
			};
		//周りより明るい点(窓)を集める
		struct Window { int x, y; };
		std::vector<Window> windows;
		const int top = kCityRowTop * h / 1024;
		const int bottom = kCityRowBottom * h / 1024;
		for (int y = (std::max)(top, 3); y < (std::min)(bottom, h - 3); ++y)
		{
			for (int x = 3; x < w - 3; ++x)
			{
				const int around = (lum(x - 3, y) + lum(x + 3, y) + lum(x, y - 3) + lum(x, y + 3)) / 4;
				if (lum(x, y) - around > kCityContrast) windows.push_back({ x, y });
			}
		}
		std::shuffle(windows.begin(), windows.end(), rand);
		const int num = (std::min)(kCityGlowPerFace, static_cast<int>(windows.size()));
		for (int i = 0; i < num; ++i)
		{
			const auto& win = windows[i];
			int r, g, b, a;
			GetPixelSoftImage(soft, win.x, win.y, &r, &g, &b, &a);
			//色は窓の色を明るくして使う
			const int maxColor = (std::max)({ r, g, b, 1 });
			const float boost = 255.0f / maxColor;
			const float u = (win.x + 0.5f) / w * 2.0f - 1.0f;
			const float v = 1.0f - (win.y + 0.5f) / h * 2.0f;
			const VECTOR dir = VNorm(VAdd(face.center, VAdd(VScale(face.right, u), VGet(0.0f, v, 0.0f))));
			const float size = kCitySizeMin + (kCitySizeMax - kCitySizeMin) * unit(rand) * unit(rand);
			m_cityGlows.push_back({ dir, static_cast<int>(r * boost), static_cast<int>(g * boost), static_cast<int>(b * boost),
				size, kCityAlpha * (0.4f + 0.6f * unit(rand)), unit(rand) * DX_TWO_PI_F });
		}
		DeleteSoftImage(soft);
	}
}

void TrainOutside::MakeGlowGraph()
{
	//真ん中が白く、ふちへなめらかに消える丸(TitleCity と同じ)
	const int soft = MakeARGB8ColorSoftImage(kGlowGraphSize, kGlowGraphSize);
	const float half = kGlowGraphSize * 0.5f;
	for (int y = 0; y < kGlowGraphSize; ++y)
	{
		for (int x = 0; x < kGlowGraphSize; ++x)
		{
			const float d = std::sqrt((x + 0.5f - half) * (x + 0.5f - half) + (y + 0.5f - half) * (y + 0.5f - half)) / half;
			const float a = (d >= 1.0f) ? 0.0f : (1.0f - d) * (1.0f - d);
			DrawPixelSoftImage(soft, x, y, 255, 255, 255, static_cast<int>(255 * a));
		}
	}
	m_glowGraph = CreateGraphFromSoftImage(soft);
	DeleteSoftImage(soft);
}
