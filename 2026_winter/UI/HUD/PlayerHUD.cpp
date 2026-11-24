#include "PlayerHUD.h"
#include <algorithm>
#include <cmath>
#include "Main/Game.h"
#include "Character/Player/Player.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"

namespace
{
	const char* const kFrameSheetPath = "data/UI/HUD/hud_frame.png";
	const char* const kFillSheetPath = "data/UI/HUD/hud_fill.png";
	const char* const kBandSheetPath = "data/UI/HUD/hud_band.png";
	const char* const kTasselPath = "data/UI/HUD/hud_tassel.png";
	const char* const kUltBasePath = "data/UI/HUD/UltGaugeBase.png";
	const char* const kUltFramePath = "data/UI/HUD/UltGauge.png";
	const char* const kUltColorPath = "data/UI/HUD/UltGaugeColor.png";
	const char* const kUltMaxPath = "data/UI/HUD/UltGauge_circle.png";
	const char* const kUltGlowPath = "data/UI/HUD/hud_ultglow.png";
	const char* const kUltSparkPath = "data/UI/HUD/hud_spark.png";

	constexpr float kPi = 3.14159265f;

	//HUD全体を下げる量(弧の上が画面から切れないように)
	constexpr float kOffsetY = 12.0f;

	//シートの画像(1024x256)に描いてある範囲(設計座標)。tools/HudGen/hud_parts.html と合わせる
	constexpr float kSheetW = 640.0f;
	constexpr float kSheetTop = -16.0f;
	constexpr float kSheetH = 160.0f;

	//HPの道: 輪の中心のまわりの弧 → 右へ直線
	constexpr float kCenterX = 90.0f;
	constexpr float kCenterY = 90.0f;
	constexpr float kRadius = 86.0f;
	constexpr float kArcStart = -kPi * 0.62f;
	constexpr float kArcEnd = -kPi * 0.12f;
	constexpr float kHpEndX = 590.0f;
	constexpr float kBandHalf = 10.0f;//切り取る幅の半分(帯は±6)

	//スキルの帯(HPの直線の下)
	constexpr float kSkillOffsetX = 4.0f;
	constexpr float kSkillOffsetY = 12.0f;
	constexpr float kSkillW = 254.0f;
	constexpr float kSkillH = 8.0f;
	constexpr float kSkillSlant = 5.0f;

	//必殺技ゲージ
	constexpr float kUltX = 90.0f;
	constexpr float kUltY = 90.0f;
	constexpr float kUltRingScale = 150.0f / 910.0f;//輪の直径150
	constexpr float kUltBaseScale = 115.0f / 453.0f;//紋章の直径115
	constexpr float kUltMaxScale = 160.0f / 436.0f;//満タンの光の直径160
	constexpr int kUltEmptyBright = 25;//たまっていない所の明るさ
	constexpr int kUltBrightAdd = 204;//たまった所に足す明るさ(255で2倍)
	constexpr float kUltGlowMin = 150.0f;//にじむ光の強さ(呼吸の弱い時〜強い時)
	constexpr float kUltGlowMax = 230.0f;
	constexpr float kUltGlowOverMin = 45.0f;//輪の上に重ねる光
	constexpr float kUltGlowOverMax = 90.0f;
	constexpr float kUltGlowSpeed = 0.05f;
	constexpr float kUltRopeRadius = 63.0f;//縄の半径(先の光を置く所)
	constexpr float kUltSparkScale = 24.0f / 128.0f;//先の光の直径24
	constexpr float kUltSparkSpeed = 0.23f;

	constexpr float kHpFollowRate = 0.25f;
	constexpr int kTrailWait = 40;//残像が減り始めるまで
	constexpr float kTrailRate = 0.08f;
	constexpr float kSkillFollowRate = 0.15f;
	constexpr float kLowHpRate = 0.3f;//これより少ないと赤く脈打つ
	constexpr float kLowHpPulseSpeed = 0.1f;
	constexpr float kBlinkSpeed = 0.1f;//必殺技の満タンの点滅
	constexpr float kReadyFlashDecay = 0.05f;
	constexpr float kUseFlashDecay = 0.067f;

	//象嵌の菱形(HPの1/4ごと)
	constexpr float kMarkHalfW = 4.0f;
	constexpr float kMarkHalfH = 6.0f;
	constexpr float kMarkEdge = 1.2f;

	//房(支点はHPの右端の少し手前の下)
	constexpr float kTasselPivotX = -10.0f;
	constexpr float kTasselPivotY = 9.0f;
	constexpr float kThreadTop = 29.0f;//糸の付け根(房頭の下)
	constexpr float kThreadLen = 28.0f;
	constexpr int kThreadNum = 9;
	constexpr float kThreadHalfW = 3.4f;
	constexpr int kThreadDiv = 8;
	constexpr float kTasselSpriteCx = 32.0f;//hud_tassel.png の支点
	constexpr float kTasselSpriteCy = 8.0f;
	constexpr float kTasselSpriteScale = 0.5f;//画像は設計座標の2倍
	//風
	constexpr float kWindAngle = 38.0f;
	constexpr float kWindFlutter = 3.0f;
	constexpr float kWindTipRate = 1.3f;
	constexpr float kSwingAmp = 0.8f;
	//振り子
	constexpr float kSwingDt = 1.0f / 60.0f;
	constexpr float kSwingSpring = 45.0f;
	constexpr float kSwingDamp = 3.2f;
	constexpr float kTipSpring = 90.0f;
	constexpr float kTipDamp = 7.0f;
	constexpr float kHitImpulse = 150.0f;
	constexpr float kUseImpulse = -70.0f;
	constexpr float kShadowX = 1.6f;
	constexpr float kShadowY = 2.4f;
}

PlayerHUD::PlayerHUD(std::weak_ptr<Player> player) :
	m_player(player)
{
}

PlayerHUD::~PlayerHUD()
{
	DeleteGraph(m_frameSheet);
	DeleteGraph(m_fillSheet);
	DeleteGraph(m_bandSheet);
	DeleteGraph(m_tasselHandle);
	DeleteGraph(m_ultBaseHandle);
	DeleteGraph(m_ultFrameHandle);
	DeleteGraph(m_ultColorHandle);
	DeleteGraph(m_ultMaxHandle);
	DeleteGraph(m_ultGlowHandle);
	DeleteGraph(m_ultSparkHandle);
}

void PlayerHUD::Init()
{
	//縮小したときに透明な所の白がにじまないよう、乗算済みアルファで読む
	SetUsePremulAlphaConvertLoad(TRUE);
	m_frameSheet = LoadGraph(kFrameSheetPath);
	m_fillSheet = LoadGraph(kFillSheetPath);
	m_bandSheet = LoadGraph(kBandSheetPath);
	m_tasselHandle = LoadGraph(kTasselPath);
	m_ultBaseHandle = LoadGraph(kUltBasePath);
	m_ultFrameHandle = LoadGraph(kUltFramePath);
	m_ultColorHandle = LoadGraph(kUltColorPath);
	m_ultMaxHandle = LoadGraph(kUltMaxPath);
	m_ultGlowHandle = LoadGraph(kUltGlowPath);
	m_ultSparkHandle = LoadGraph(kUltSparkPath);
	SetUsePremulAlphaConvertLoad(FALSE);
	m_isVisible = true;

	//最初は今の値から始める
	auto player = m_player.lock();
	if (!player) return;
	m_lastHp = player->GetHp();
	m_lastSkill = player->GetSkillGauge();
	m_dispHpRate = static_cast<float>(m_lastHp) / player->GetMaxHp();
	m_trailRate = m_dispHpRate;
	m_dispSkill = static_cast<float>(m_lastSkill) / player->GetMaxGauge();
	m_dispUltRate = static_cast<float>(player->GetUltGauge()) / player->GetMaxGauge();
}

void PlayerHUD::Update()
{
	auto player = m_player.lock();
	auto battle = System::GetInstance().GetBattleMgr();
	m_isVisible = (player != nullptr) && !battle->GetFinisherScreen().isPlaying && !battle->GetEndingScreen().isPlaying && !battle->GetIsEventPlaying();
	if (!player) return;

	const int hp = player->GetHp();
	const int skill = player->GetSkillGauge();
	const float maxGauge = static_cast<float>(player->GetMaxGauge());

	//被弾
	if (hp < m_lastHp)
	{
		m_trailWait = kTrailWait;
		m_swingVel += kHitImpulse;
	}
	//スキルを使った
	if (skill < m_lastSkill)
	{
		m_useFlash = 1.0f;
		m_useFrom = skill / maxGauge;
		m_useTo = m_lastSkill / maxGauge;
		m_swingVel += kUseImpulse;
	}
	m_lastHp = hp;
	m_lastSkill = skill;

	m_dispHpRate = Follow(m_dispHpRate, static_cast<float>(hp) / player->GetMaxHp(), kHpFollowRate);
	if (m_trailRate < m_dispHpRate) m_trailRate = m_dispHpRate;
	if (m_trailWait > 0) --m_trailWait;
	else m_trailRate = Follow(m_trailRate, m_dispHpRate, kTrailRate);

	//1回分たまった瞬間に光らせる
	const float units = maxGauge / player->GetSkillCost();
	const int before = static_cast<int>(m_dispSkill * units + 0.0001f);
	m_dispSkill = Follow(m_dispSkill, skill / maxGauge, kSkillFollowRate);
	const int after = static_cast<int>(m_dispSkill * units + 0.0001f);
	if (after > before) m_readyFlash = 1.0f;
	m_readyFlash = (std::max)(0.0f, m_readyFlash - kReadyFlashDecay);
	m_useFlash = (std::max)(0.0f, m_useFlash - kUseFlashDecay);

	m_dispUltRate = Follow(m_dispUltRate, player->GetUltGauge() / maxGauge, kHpFollowRate);

	UpdateTassel();
	m_frame++;
}

void PlayerHUD::UpdateTassel()
{
	const float t = m_frame * kSwingDt;
	const float idle = std::sin(t * 1.1f) * 5.0f + std::sin(t * 2.3f) * 2.0f;
	m_swingVel += (-kSwingSpring * m_swing - kSwingDamp * m_swingVel + idle) * kSwingDt;
	m_swing += m_swingVel * kSwingDt;
	//毛先は少し遅れてついてくる
	m_tipSwingVel += (kTipSpring * (m_swing - m_tipSwing) - kTipDamp * m_tipSwingVel) * kSwingDt;
	m_tipSwing += m_tipSwingVel * kSwingDt;
}

float PlayerHUD::Follow(float disp, float target, float rate) const
{
	disp += (target - disp) * rate;
	//近づいたらぴったりにする
	if (std::abs(target - disp) < 0.001f) disp = target;
	return std::clamp(disp, 0.0f, 1.0f);
}

PlayerHUD::PathPoint PlayerHUD::GetPathPoint(float rate) const
{
	const float arcLen = kRadius * (kArcEnd - kArcStart);
	const float lineStartX = kCenterX + kRadius * std::cos(kArcEnd);
	const float lineY = kCenterY + kRadius * std::sin(kArcEnd);
	const float len = rate * (arcLen + kHpEndX - lineStartX);
	if (len < arcLen)
	{
		const float a = kArcStart + len / kRadius;
		return { kCenterX + kRadius * std::cos(a), kCenterY + kRadius * std::sin(a), std::cos(a), std::sin(a) };
	}
	return { lineStartX + (len - arcLen), lineY, 0.0f, -1.0f };
}

VECTOR PlayerHUD::ToScreen(float x, float y) const
{
	return VGet(Game::ScaleX(x), Game::ScaleY(y + kOffsetY), 0.0f);
}

void PlayerHUD::Draw()
{
	if (!m_isVisible) return;

	SetDrawMode(DX_DRAWMODE_BILINEAR);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
	DrawUlt();
	DrawHp();
	DrawSkill();
	DrawTassel();
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

void PlayerHUD::DrawSheetPolygon(const float (*pos)[2], int num, int handle, COLOR_U8 color)
{
	VERTEX2D vtx[256];
	num = (std::min)(num, 256);
	for (int i = 0; i < num; ++i)
	{
		vtx[i].pos = ToScreen(pos[i][0], pos[i][1]);
		vtx[i].rhw = 1.0f;
		vtx[i].dif = color;
		vtx[i].u = pos[i][0] / kSheetW;
		vtx[i].v = (pos[i][1] - kSheetTop) / kSheetH;
	}
	DrawPolygon2D(vtx, num / 3, handle, TRUE);
}

void PlayerHUD::DrawHpRange(int handle, float from, float to, COLOR_U8 color)
{
	if (to <= from) return;
	const float arcLen = kRadius * (kArcEnd - kArcStart);
	const float lineStartX = kCenterX + kRadius * std::cos(kArcEnd);
	const float lineY = kCenterY + kRadius * std::sin(kArcEnd);
	const float total = arcLen + kHpEndX - lineStartX;
	const float len0 = from * total;
	const float len1 = to * total;

	float pos[256][2];
	int n = 0;
	auto push = [&](float x, float y) { pos[n][0] = x; pos[n][1] = y; ++n; };
	auto quad = [&](float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
		{
			push(x0, y0); push(x1, y1); push(x2, y2);
			push(x0, y0); push(x2, y2); push(x3, y3);
		};

	//弧の所: 内と外の半径の帯を細かく区切る
	if (len0 < arcLen)
	{
		const float a0 = kArcStart + len0 / kRadius;
		const float a1 = kArcStart + (std::min)(len1, arcLen) / kRadius;
		const int div = (std::max)(1, static_cast<int>(std::ceil((a1 - a0) / (kPi / 48.0f))));
		const float rIn = kRadius - kBandHalf;
		const float rOut = kRadius + kBandHalf;
		for (int i = 0; i < div; ++i)
		{
			const float s = a0 + (a1 - a0) * i / div;
			const float e = a0 + (a1 - a0) * (i + 1) / div;
			quad(kCenterX + rIn * std::cos(s), kCenterY + rIn * std::sin(s),
				kCenterX + rOut * std::cos(s), kCenterY + rOut * std::sin(s),
				kCenterX + rOut * std::cos(e), kCenterY + rOut * std::sin(e),
				kCenterX + rIn * std::cos(e), kCenterY + rIn * std::sin(e));
		}
	}
	//直線の所
	if (len1 > arcLen)
	{
		const float x0 = lineStartX + (std::max)(0.0f, len0 - arcLen);
		const float x1 = lineStartX + (len1 - arcLen);
		quad(x0, lineY - kBandHalf, x1, lineY - kBandHalf, x1, lineY + kBandHalf, x0, lineY + kBandHalf);
	}
	DrawSheetPolygon(pos, n, handle, color);
}

void PlayerHUD::DrawSkillRange(int handle, float from, float to, COLOR_U8 color)
{
	if (to <= from) return;
	const float x = kCenterX + kRadius * std::cos(kArcEnd) + kSkillOffsetX;
	const float y = kCenterY + kRadius * std::sin(kArcEnd) + kSkillOffsetY;
	const float pos[6][2] =
	{
		{ x + kSkillSlant + kSkillW * from, y }, { x + kSkillSlant + kSkillW * to, y }, { x + kSkillW * to, y + kSkillH },
		{ x + kSkillSlant + kSkillW * from, y }, { x + kSkillW * to, y + kSkillH }, { x + kSkillW * from, y + kSkillH },
	};
	DrawSheetPolygon(pos, 6, handle, color);
}

void PlayerHUD::DrawHp()
{
	const COLOR_U8 white = GetColorU8(255, 255, 255, 255);

	//縁・地・鐺・巻いた下げ緒・スキルの枠
	const VECTOR lt = ToScreen(0.0f, kSheetTop);
	const VECTOR rb = ToScreen(kSheetW, kSheetTop + kSheetH);
	DrawExtendGraphF(lt.x, lt.y, rb.x, rb.y, m_frameSheet, TRUE);

	//減った分の残像
	DrawHpRange(m_bandSheet, m_dispHpRate, m_trailRate, GetColorU8(232, 232, 236, 255));
	//残り
	DrawHpRange(m_fillSheet, 0.0f, m_dispHpRate, white);

	//少ないと赤く脈打つ
	if (m_dispHpRate < kLowHpRate)
	{
		const float pulse = (std::sin(m_frame * kLowHpPulseSpeed) + 1.0f) * 0.5f;
		SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, static_cast<int>(150 * pulse));
		DrawHpRange(m_bandSheet, 0.0f, m_dispHpRate, GetColorU8(255, 64, 88, 255));
		SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
	}

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	const float s = Game::GetScale();
	//象嵌の菱形(HPがそこより減ると暗くなる)
	for (float rate : { 0.25f, 0.5f, 0.75f })
	{
		const PathPoint p = GetPathPoint(rate);
		const VECTOR c = ToScreen(p.x, p.y);
		const float w = kMarkHalfW * s;
		const float h = kMarkHalfH * s;
		const float e = kMarkEdge * s;
		DrawQuadrangleAA(c.x, c.y - h - e, c.x + w + e, c.y, c.x, c.y + h + e, c.x - w - e, c.y, GetColor(6, 4, 6), TRUE);
		const unsigned int col = (m_dispHpRate >= rate) ? GetColor(232, 236, 242) : GetColor(74, 78, 88);
		DrawQuadrangleAA(c.x, c.y - h, c.x + w, c.y, c.x, c.y + h, c.x - w, c.y, col, TRUE);
	}
	//今のHPの先
	if (m_dispHpRate > 0.0f && m_dispHpRate < 1.0f)
	{
		const PathPoint p = GetPathPoint(m_dispHpRate);
		const VECTOR c = ToScreen(p.x, p.y);
		DrawCircleAA(c.x, c.y, 3.0f * s, 16, GetColor(255, 255, 255), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
}

void PlayerHUD::DrawSkill()
{
	auto player = m_player.lock();
	if (!player) return;
	const COLOR_U8 white = GetColorU8(255, 255, 255, 255);
	const float rate = m_dispSkill;
	const bool canUse = player->GetSkillGauge() >= player->GetSkillCost();

	//1回分あれば銀、足りなければ暗い色
	if (canUse) DrawSkillRange(m_fillSheet, 0.0f, rate, white);
	else DrawSkillRange(m_bandSheet, 0.0f, rate, GetColorU8(42, 44, 52, 255));

	//たまっている途中の先
	if (rate > 0.0f && rate < 1.0f)
	{
		SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 217);
		DrawSkillRange(m_bandSheet, (std::max)(0.0f, rate - 0.004f), rate, white);
	}
	//1回分たまった瞬間
	if (m_readyFlash > 0.0f)
	{
		SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(204 * m_readyFlash));
		DrawSkillRange(m_bandSheet, 0.0f, rate, white);
	}
	//使った所が白く弾ける
	if (m_useFlash > 0.0f)
	{
		SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(230 * m_useFlash));
		DrawSkillRange(m_bandSheet, m_useFrom, m_useTo, white);
	}
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
}

void PlayerHUD::DrawTassel()
{
	static constexpr unsigned char kThreadColors[5][3] =
	{
		{ 220, 58, 72 }, { 138, 26, 36 }, { 240, 96, 108 }, { 176, 40, 52 }, { 206, 50, 64 },
	};

	const float t = m_frame * kSwingDt;
	const float wind = kWindAngle + std::sin(t * 7.0f) * kWindFlutter + std::sin(t * 11.3f) * kWindFlutter * 0.4f;
	const float a = m_swing * kSwingAmp + wind;
	const float a2 = m_tipSwing * kSwingAmp + wind * kWindTipRate;

	const float lineY = kCenterY + kRadius * std::sin(kArcEnd);
	const float px = kHpEndX + kTasselPivotX;
	const float py = lineY + kTasselPivotY;
	//支点のまわりに回した位置(設計座標)
	auto rot = [&](float x, float y, float deg, float& ox, float& oy)
		{
			const float r = deg * kPi / 180.0f;
			ox = px + x * std::cos(r) - y * std::sin(r);
			oy = py + x * std::sin(r) + y * std::cos(r);
		};

	//糸を折れ線にする
	float pts[kThreadNum][kThreadDiv + 1][2];
	for (int j = 0; j < kThreadNum; ++j)
	{
		const float xj = (j - (kThreadNum - 1) * 0.5f) * (kThreadHalfW * 2.0f / (kThreadNum - 1));
		const float len = kThreadLen * (0.86f + 0.14f * std::cos(j * 2.1f));
		float sx, sy, cx, cy, ex, ey;
		rot(xj, kThreadTop, a, sx, sy);
		rot(xj * 1.35f, kThreadTop + len * 0.5f, (a + a2) * 0.5f, cx, cy);
		rot(xj * 1.7f, kThreadTop + len, a2 + xj * 0.8f, ex, ey);
		for (int k = 0; k <= kThreadDiv; ++k)
		{
			const float u = static_cast<float>(k) / kThreadDiv;
			const float iu = 1.0f - u;
			pts[j][k][0] = iu * iu * sx + 2.0f * iu * u * cx + u * u * ex;
			pts[j][k][1] = iu * iu * sy + 2.0f * iu * u * cy + u * u * ey;
		}
	}

	const float s = Game::GetScale();
	auto drawThreads = [&](float offX, float offY, float width, bool useColor, unsigned int black)
		{
			for (int j = 0; j < kThreadNum; ++j)
			{
				const unsigned char* c = kThreadColors[j % 5];
				const unsigned int col = useColor ? GetColor(c[0], c[1], c[2]) : black;
				for (int k = 0; k < kThreadDiv; ++k)
				{
					const VECTOR p0 = ToScreen(pts[j][k][0] + offX, pts[j][k][1] + offY);
					const VECTOR p1 = ToScreen(pts[j][k + 1][0] + offX, pts[j][k + 1][1] + offY);
					DrawLineAA(p0.x, p0.y, p1.x, p1.y, col, width * s);
				}
			}
		};
	const unsigned int black = GetColor(6, 4, 6);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 150);
	drawThreads(kShadowX, kShadowY, 2.6f, false, GetColor(0, 0, 0));
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	drawThreads(0.0f, 0.0f, 2.6f, false, black);
	drawThreads(0.0f, 0.0f, 1.3f, true, black);

	//紐・菊結び・房頭
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
	const VECTOR pivot = ToScreen(px, py);
	DrawRotaGraph3F(pivot.x, pivot.y, kTasselSpriteCx, kTasselSpriteCy,
		kTasselSpriteScale * s, kTasselSpriteScale * s, a * kPi / 180.0f, m_tasselHandle, TRUE);
}

void PlayerHUD::DrawUlt()
{
	const VECTOR c = ToScreen(kUltX, kUltY);
	const float cx = c.x;
	const float cy = c.y;
	const float s = Game::GetScale();

	//真ん中の紋章(外側に白いぼかしがあるので輪より先に描く)
	DrawRotaGraphF(cx, cy, kUltBaseScale * s, 0.0, m_ultBaseHandle, TRUE);

	//たまっていない所は暗く
	SetDrawBright(kUltEmptyBright, kUltEmptyBright, kUltEmptyBright);
	DrawRotaGraphF(cx, cy, kUltRingScale * s, 0.0, m_ultColorHandle, TRUE);
	SetDrawBright(255, 255, 255);

	//たまった所(上から時計回り。左右反転すると反時計回りになる)
	const double percent = m_dispUltRate * 100.0;
	DrawCircleGaugeF(cx, cy, percent, m_ultColorHandle, 0.0, kUltRingScale * s, FALSE);
	//同じ画像を加算で重ねて明るくする
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, kUltBrightAdd);
	DrawCircleGaugeF(cx, cy, percent, m_ultColorHandle, 0.0, kUltRingScale * s, FALSE);
	//にじむ光(ゆっくり呼吸する)
	const float breath = (std::sin(m_frame * kUltGlowSpeed) + 1.0f) * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, static_cast<int>(kUltGlowMin + (kUltGlowMax - kUltGlowMin) * breath));
	DrawCircleGaugeF(cx, cy, percent, m_ultGlowHandle, 0.0, kUltRingScale * s, FALSE);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);

	//黒い輪(縄の所は抜けているので下の色が見える)
	DrawRotaGraphF(cx, cy, kUltRingScale * s, 0.0, m_ultFrameHandle, TRUE);
	//輪の上にも光を薄く重ねて、外へにじませる
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, static_cast<int>(kUltGlowOverMin + (kUltGlowOverMax - kUltGlowOverMin) * breath));
	DrawCircleGaugeF(cx, cy, percent, m_ultGlowHandle, 0.0, kUltRingScale * s, FALSE);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);

	//たまっている先の光
	if (m_dispUltRate > 0.0f && m_dispUltRate < 1.0f)
	{
		const float a = -kPi * 0.5f + m_dispUltRate * kPi * 2.0f;
		const float r = kUltRopeRadius * s;
		const float flicker = 0.85f + 0.15f * std::sin(m_frame * kUltSparkSpeed);
		SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, 255);
		DrawRotaGraphF(cx + r * std::cos(a), cy + r * std::sin(a), kUltSparkScale * s * flicker, 0.0, m_ultSparkHandle, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
	}

	//満タンなら光らせる
	auto player = m_player.lock();
	if (!player || player->GetUltGauge() < player->GetMaxGauge()) return;
	const float blink = (std::sin(m_frame * kBlinkSpeed) + 1.0f) * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, static_cast<int>(100 + 155 * blink));
	DrawRotaGraphF(cx, cy, kUltMaxScale * s, 0.0, m_ultMaxHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
}
