#include "TitleCity.h"
#include "SkyBox.h"
#include "Main/Game.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
	const char* const kModelPath = "data/Stage/CyberStage/CyberStage.mv1";
	const char* const kColModelPath = "data/Stage/CyberStage/CyberStageCol.mv1";
	const char* const kSkyDir = "data/Stage/CyberStage/Sky";
	const char* const kLightCsvPath = "data/Stage/Lights/Stage005.csv";
	constexpr int kLightNumber = 5;//明かりの CSV の番号(ステージ1)

	//床の高さをそろえる(Stage::GameInit と同じ)
	constexpr float kFloorCheckStartY = 800.0f;
	constexpr float kFloorCheckEndY = -100000.0f;
	constexpr float kDefaultPosY = -800.0f;

	//見下ろすカメラ(ステージの座標)//左手前の高い所から、明かりの多い広場を見る
	const VECTOR kCameraPos = { -6000.0f, 2500.0f, 22000.0f };
	const VECTOR kCameraTarget = { 3000.0f, 0.0f, 29000.0f };
	constexpr float kFov = 45.0f * DX_PI_F / 180.0f;
	constexpr float kNear = 50.0f;
	constexpr float kFar = 30000.0f;

	//夜のもや(本編のステージ1と同じ色)
	constexpr int kFogR = 22, kFogG = 12, kFogB = 32;
	constexpr float kFogStart = 3000.0f;
	constexpr float kFogEnd = 14000.0f;
	constexpr float kFogOff = 1.0e8f;
	constexpr float kFogOffEnd = 2.0e8f;//始まりと同じにすると0で割ってフォグが全面にかかり、フォグを使わないステージ(トレーニング)に残る

	//街の光(DxLib の標準ライト)
	const COLOR_F kLightColor = { 0.25f, 0.25f, 0.35f, 1.0f };
	const VECTOR kLightDir = { 0.3f, -1.0f, 0.5f };

	//光の玉
	constexpr int kGlowGraphSize = 64;
	constexpr float kGlowSizeRate = 1.0f;//明かりの届く距離に対する玉の大きさ
	constexpr float kGlowAlpha = 0.55f;
	constexpr float kTwinkleSpeed = 0.02f;
	constexpr float kTwinkleRate = 0.25f;//瞬きで暗くなる割合

	//窓の明かり(ビルの壁にランダムに置く)
	constexpr int kWindowNum = 2500;
	constexpr unsigned int kWindowSeed = 7;
	constexpr float kWallNormalY = 0.3f;//面の向きの上下がこれ以下なら壁
	constexpr float kWindowMinY = 150.0f;//地面すれすれには置かない
	constexpr float kWindowOffset = 15.0f;//壁から少し浮かせる
	constexpr float kWindowSizeMin = 220.0f;
	constexpr float kWindowSizeMax = 560.0f;
	constexpr float kWindowAlpha = 0.9f;

	//ぼかし・にじみ
	constexpr int kScreenDiv = 2;//画面の 1/2 で描く
	constexpr int kBloomDiv = 4;//にじみは 1/4
	constexpr int kBlurPixel = 16;
	constexpr int kBlurParam = 350;
	constexpr int kBloomThreshold = 60;//これより暗い所はにじませない
	constexpr int kBloomBlurParam = 1400;
	constexpr int kBloomAlpha = 220;
}

TitleCity::TitleCity()
{
}

TitleCity::~TitleCity()
{
	MV1DeleteModel(m_modelHandle);
	DeleteGraph(m_screen);
	DeleteGraph(m_bloom);
	DeleteGraph(m_glowGraph);
	//明かりをタイトルのキャラに残さない
	ModelShaderManager::GetInstance().SetStageLights({}, kFogOff, kFogOffEnd, 0.0f);
}

void TitleCity::Init()
{
	m_modelHandle = MV1LoadModel(kModelPath);

	//床が y=0 に来るようにずらす(明かりの CSV もこの位置が前提)
	float floorY = kDefaultPosY;
	const int col = MV1LoadModel(kColModelPath);
	MV1SetupCollInfo(col, -1);
	const MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(col, -1, VGet(0.0f, kFloorCheckStartY, 0.0f), VGet(0.0f, kFloorCheckEndY, 0.0f));
	if (hit.HitFlag)floorY = -hit.HitPosition.y;
	MV1DeleteModel(col);
	MV1SetPosition(m_modelHandle, VGet(0.0f, floorY, 0.0f));

	m_lights.Load(kLightNumber, Vector3(0.0f, floorY, 0.0f));
	m_lights.SetFogRange(kFogStart, kFogEnd);
	LoadGlows(floorY);
	AddWindowGlows();
	MakeGlowGraph();

	m_skyBox = std::make_unique<SkyBox>();
	m_skyBox->Init(kSkyDir);

	m_screen = MakeScreen(Game::GetScreenWidth() / kScreenDiv, Game::GetScreenHeight() / kScreenDiv, FALSE);
	m_bloom = MakeScreen(Game::GetScreenWidth() / kBloomDiv, Game::GetScreenHeight() / kBloomDiv, FALSE);
}

void TitleCity::LoadGlows(float offsetY)
{
	//kind,posX,posY,posZ,colorR,colorG,colorB,range,intensity
	std::ifstream file(kLightCsvPath);
	std::string line;
	int index = 0;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#')continue;
		std::stringstream ss(line);
		std::string kind;
		std::getline(ss, kind, ',');
		float v[8] = {};
		char comma = 0;
		for (int i = 0; i < 8; ++i)
		{
			ss >> v[i];
			ss >> comma;
		}
		const float bright = (std::min)(v[7], 2.0f) * 0.5f;
		m_glows.push_back({ VGet(v[0], v[1] + offsetY, v[2]),
			static_cast<int>(255 * v[3]), static_cast<int>(255 * v[4]), static_cast<int>(255 * v[5]),
			v[6] * kGlowSizeRate, kGlowAlpha * (0.5f + bright), index * 1.7f });
		index++;
	}
}

void TitleCity::AddWindowGlows()
{
	MV1SetupReferenceMesh(m_modelHandle, -1, TRUE);
	const MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(m_modelHandle, -1, TRUE);

	//壁(横を向いた面)だけ、面積に比例して選ぶ
	std::vector<int> walls;
	std::vector<float> areaSum;
	float total = 0.0f;
	for (int i = 0; i < ref.PolygonNum; ++i)
	{
		const auto& poly = ref.Polygons[i];
		const VECTOR a = ref.Vertexs[poly.VIndex[0]].Position;
		const VECTOR b = ref.Vertexs[poly.VIndex[1]].Position;
		const VECTOR c = ref.Vertexs[poly.VIndex[2]].Position;
		const VECTOR cross = VCross(VSub(b, a), VSub(c, a));
		const float size = VSize(cross);
		if (size <= 0.0f || std::fabs(cross.y / size) > kWallNormalY)continue;
		if ((std::min)({ a.y, b.y, c.y }) < kWindowMinY)continue;
		total += size * 0.5f;
		walls.push_back(i);
		areaSum.push_back(total);
	}

	std::mt19937 rand(kWindowSeed);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	for (int n = 0; n < kWindowNum && !walls.empty(); ++n)
	{
		const float pick = unit(rand) * total;
		const int index = static_cast<int>(std::lower_bound(areaSum.begin(), areaSum.end(), pick) - areaSum.begin());
		const auto& poly = ref.Polygons[walls[(std::min)(index, static_cast<int>(walls.size()) - 1)]];
		const VECTOR a = ref.Vertexs[poly.VIndex[0]].Position;
		const VECTOR b = ref.Vertexs[poly.VIndex[1]].Position;
		const VECTOR c = ref.Vertexs[poly.VIndex[2]].Position;
		//三角形の中の点
		float u = unit(rand), v = unit(rand);
		if (u + v > 1.0f) { u = 1.0f - u; v = 1.0f - v; }
		const VECTOR normal = VNorm(VCross(VSub(b, a), VSub(c, a)));
		const VECTOR pos = VAdd(VAdd(a, VAdd(VScale(VSub(b, a), u), VScale(VSub(c, a), v))), VScale(normal, kWindowOffset));

		//色は暖色が多め、時々赤・青白
		const float color = unit(rand);
		int r = 255, g = 170, bl = 90;
		if (color > 0.85f) { r = 170; g = 215; bl = 255; }
		else if (color > 0.6f) { r = 255; g = 70; bl = 55; }
		const float size = kWindowSizeMin + (kWindowSizeMax - kWindowSizeMin) * unit(rand);
		m_glows.push_back({ pos, r, g, bl, size, kWindowAlpha * (0.4f + 0.6f * unit(rand)), unit(rand) * DX_TWO_PI_F });
	}
	MV1TerminateReferenceMesh(m_modelHandle, -1, TRUE);
}

void TitleCity::MakeGlowGraph()
{
	//真ん中が白く、ふちへなめらかに消える丸
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

void TitleCity::Draw()
{
	m_frame++;
	//今のカメラを覚えておく(SetDrawScreen で戻るので)
	const CameraState saved = { GetCameraPosition(), GetCameraTarget(), GetCameraFov(), GetCameraNear(), GetCameraFar() };
	const VECTOR savedLightDir = GetLightDirection();
	const COLOR_F savedLightColor = GetLightDifColor();

	SetDrawScreen(m_screen);
	ClearDrawScreen();
	SetCameraNearFar(kNear, kFar);
	SetupCamera_Perspective(kFov);
	SetCameraPositionAndTarget_UpVecY(kCameraPos, kCameraTarget);
	SetLightDirection(VNorm(kLightDir));
	SetLightDifColor(kLightColor);
	DrawCity(kCameraTarget);

	//明るい所だけ取り出して大きくにじませる。全体もぼかす
	GraphFilterBlt(m_screen, m_bloom, DX_GRAPH_FILTER_BRIGHT_CLIP, DX_CMP_LESS, kBloomThreshold, TRUE, GetColor(0, 0, 0), 255);
	GraphFilter(m_bloom, DX_GRAPH_FILTER_GAUSS, kBlurPixel, kBloomBlurParam);
	GraphFilter(m_screen, DX_GRAPH_FILTER_GAUSS, kBlurPixel, kBlurParam);

	SetDrawScreen(DX_SCREEN_BACK);
	SetCameraNearFar(saved.nearZ, saved.farZ);
	SetupCamera_Perspective(saved.fov);
	SetCameraPositionAndTarget_UpVecY(saved.pos, saved.target);
	SetLightDirection(savedLightDir);
	SetLightDifColor(savedLightColor);

	const int w = Game::GetScreenWidth();
	const int h = Game::GetScreenHeight();
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawExtendGraph(0, 0, w, h, m_screen, FALSE);
	SetDrawBlendMode(DX_BLENDMODE_ADD, kBloomAlpha);
	DrawExtendGraph(0, 0, w, h, m_bloom, FALSE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

void TitleCity::DrawCity(const VECTOR& target)
{
	m_skyBox->Draw();

	SetFogEnable(TRUE);
	SetFogMode(DX_FOGMODE_LINEAR);
	SetFogColor(kFogR, kFogG, kFogB);
	SetFogStartEnd(kFogStart, kFogEnd);

	//見ている所の近くの明かりを使う
	m_lights.Update(Vector3(target.x, target.y, target.z));
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);
	//明かりをタイトルのキャラに残さない
	ModelShaderManager::GetInstance().SetStageLights({}, kFogOff, kFogOffEnd, 0.0f);

	//自作の頂点シェーダーはフォグを切っても距離の値を読むので、遠くへ送ってから切る(Stage::Draw と同じ)
	SetFogStartEnd(kFogOff, kFogOffEnd);
	SetFogEnable(FALSE);

	DrawGlows();
}

void TitleCity::DrawGlows()
{
	//建物の陰では隠れるように深さは見るが、書かない
	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(FALSE);
	SetUseLighting(FALSE);
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	for (const auto& glow : m_glows)
	{
		const float twinkle = 1.0f - kTwinkleRate * (0.5f + 0.5f * std::sin(m_frame * kTwinkleSpeed + glow.phase));
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * glow.alpha * twinkle));
		SetDrawBright(glow.r, glow.g, glow.b);
		DrawBillboard3D(glow.pos, 0.5f, 0.5f, glow.size, 0.0f, m_glowGraph, TRUE);
	}
	SetDrawBright(255, 255, 255);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetUseLighting(TRUE);
	SetWriteZBuffer3D(TRUE);
}
