#include "PlayerSwordTrail.h"
#include <algorithm>
#include "DxLib.h"

namespace
{
	const char* const kStreakPath = "data/Effect/Slash/Texture/ST_Streaks_d.png";
	const char* const kEdgePath = "data/Effect/Slash/Texture/ST_Edge_e.png";
	const char* const kHazePath = "data/Effect/Slash/Texture/ST_Haze_e.png";

	constexpr float kLife = 8.0f;//点が消えるまでのフレーム数(軌跡の長さになる)
	constexpr int kSubdivision = 5;//点と点の間を何分割してなめらかにするか
	constexpr float kMinMove = 1.0f;//刀の先がこれより動いていなければ点を足さない(止まっている間)
	constexpr float kNewestU = 0.95f;//一番新しい点の画像のu(画像は外側ほど先まで伸びる三日月の形)
	constexpr float kStreakAlpha = 255.0f;//細い線の束の濃さ
	constexpr float kStreakGlowAlpha = 140.0f;//細い線の束を加算で重ねる濃さ(光って見えるように)
	constexpr float kHazeAlpha = 150.0f;//線の間のもやの濃さ
	constexpr float kEdgeAlpha = 230.0f;//先のふちの光の濃さ(加算)
	constexpr float kFadePower = 0.6f;//古くなるときの薄くなり方(1で一定の速さ、小さいほど最後まで残る)
}

PlayerSwordTrail::PlayerSwordTrail()
{
	m_style = { kLife, kMinMove, kHazeAlpha, kStreakAlpha, kStreakGlowAlpha, kEdgeAlpha, kFadePower };
	m_streakGraph = LoadGraph(kStreakPath);
	m_edgeGraph = LoadGraph(kEdgePath);
	m_hazeGraph = LoadGraph(kHazePath);
}

PlayerSwordTrail::~PlayerSwordTrail()
{
	DeleteGraph(m_streakGraph);
	DeleteGraph(m_edgeGraph);
	DeleteGraph(m_hazeGraph);
}

void PlayerSwordTrail::Sample(const Vector3& base, const Vector3& tip, float timeScale)
{
	for (auto& point : m_points) point.age += timeScale;
	const float life = m_style.life;
	std::erase_if(m_points, [life](const Point& point) { return point.age >= life; });

	if (m_isEmitting)
	{
		if (!m_wasEmitting) ++m_segment;
		const bool isMoved = m_points.empty() || m_points.back().segment != m_segment
			|| (m_points.back().tip - tip).Magnitude() >= m_style.minMove;
		if (isMoved) m_points.push_back({ base, tip, 0.0f, m_segment });
	}
	m_wasEmitting = m_isEmitting;
}

Vector3 PlayerSwordTrail::CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t)
{
	const float t2 = t * t;
	const float t3 = t2 * t;
	return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
}

void PlayerSwordTrail::BuildVertices(const std::vector<Point>& points, int begin, int end, std::vector<Vertex>& out)
{
	out.clear();
	for (int i = begin; i < end - 1; ++i)
	{
		const Point& p0 = points[(std::max)(i - 1, begin)];
		const Point& p1 = points[i];
		const Point& p2 = points[i + 1];
		const Point& p3 = points[(std::min)(i + 2, end - 1)];
		for (int s = 0; s < kSubdivision; ++s)
		{
			const float t = static_cast<float>(s) / kSubdivision;
			out.push_back({ CatmullRom(p0.base, p1.base, p2.base, p3.base, t),
				CatmullRom(p0.tip, p1.tip, p2.tip, p3.tip, t),
				p1.age + (p2.age - p1.age) * t });
		}
	}
	out.push_back({ points[end - 1].base, points[end - 1].tip, points[end - 1].age });
}

void PlayerSwordTrail::DrawStrip(const std::vector<Vertex>& vertices, int graph, int blendMode, float alpha)
{
	const int count = static_cast<int>(vertices.size());
	std::vector<VERTEX3D> polygons;
	polygons.reserve((count - 1) * 6);
	auto makeVertex = [&](const Vector3& pos, float u, float v, float age)
		{
			VERTEX3D vertex = {};
			vertex.pos = pos.ToDxLibVector();
			vertex.norm = VGet(0.0f, 1.0f, 0.0f);
			const float fade = powf(std::clamp(1.0f - age / m_style.life, 0.0f, 1.0f), m_style.fadePower);
			vertex.dif = GetColorU8(255, 255, 255, static_cast<int>(alpha * fade));
			vertex.spc = GetColorU8(0, 0, 0, 0);
			vertex.u = u;
			vertex.v = v;
			return vertex;
		};
	for (int i = 0; i < count - 1; ++i)
	{
		//uは古い側が0、新しい側がkNewestU//vは根元が0、先が1
		const float u0 = kNewestU * i / (count - 1);
		const float u1 = kNewestU * (i + 1) / (count - 1);
		const VERTEX3D base0 = makeVertex(vertices[i].base, u0, 0.0f, vertices[i].age);
		const VERTEX3D tip0 = makeVertex(vertices[i].tip, u0, 1.0f, vertices[i].age);
		const VERTEX3D base1 = makeVertex(vertices[i + 1].base, u1, 0.0f, vertices[i + 1].age);
		const VERTEX3D tip1 = makeVertex(vertices[i + 1].tip, u1, 1.0f, vertices[i + 1].age);
		polygons.push_back(base0);
		polygons.push_back(tip0);
		polygons.push_back(base1);
		polygons.push_back(tip0);
		polygons.push_back(tip1);
		polygons.push_back(base1);
	}
	SetDrawBlendMode(blendMode, 255);
	DrawPolygon3D(polygons.data(), static_cast<int>(polygons.size() / 3), graph, TRUE);
}

void PlayerSwordTrail::Draw()
{
	if (m_points.size() < 2) return;

	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(FALSE);
	SetUseBackCulling(FALSE);

	//区切りごとに帯を描く
	int begin = 0;
	const int size = static_cast<int>(m_points.size());
	for (int i = 1; i <= size; ++i)
	{
		if (i < size && m_points[i].segment == m_points[begin].segment) continue;
		if (i - begin >= 2)
		{
			BuildVertices(m_points, begin, i, m_vertices);
			if (m_style.hazeAlpha > 0.0f) DrawStrip(m_vertices, m_hazeGraph, DX_BLENDMODE_ALPHA, m_style.hazeAlpha);
			if (m_style.streakAlpha > 0.0f) DrawStrip(m_vertices, m_streakGraph, DX_BLENDMODE_ALPHA, m_style.streakAlpha);
			if (m_style.streakGlowAlpha > 0.0f) DrawStrip(m_vertices, m_streakGraph, DX_BLENDMODE_ADD, m_style.streakGlowAlpha);
			if (m_style.edgeAlpha > 0.0f) DrawStrip(m_vertices, m_edgeGraph, DX_BLENDMODE_ADD, m_style.edgeAlpha);
		}
		begin = i;
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
	SetUseBackCulling(TRUE);//Application::Init で有効にしている
	SetWriteZBuffer3D(TRUE);
}
