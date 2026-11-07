#include "GuardBreakChanceUI.h"
#include <cmath>
#include "DxLib.h"
#include "Main/Game.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"

namespace
{
	//画像のパス
	const char* const kBladePath = "data/UI/GuardBreak/blade.png";
	const char* const kBladeGlowPath = "data/UI/GuardBreak/blade_glow.png";
	const char* const kWhiteBladePath = "data/UI/PartBrokenKill/blade.png";//白一色の刃
	const char* const kGlowPath = "data/UI/GuardBreak/glow.png";
	const char* const kDiscPath = "data/UI/GuardBreak/disc.png";
	const char* const kButtonPath = "data/UI/GuardBreak/button.png";
	const char* const kBandPath = "data/UI/GuardBreak/band.png";

	//文字
	const char* const kLabel = "ガードブレイク";
	const char* const kLabelFontName = "游明朝";
	constexpr int kLabelFontSize = 22;
	constexpr int kLabelFontThick = 6;
	constexpr int kLabelFontSpace = 3;//文字の間隔
	constexpr int kLabelFontEdge = 2;//文字のふち
	const char* const kButtonLabel = "Y";
	const char* const kButtonFontName = "Arial";
	constexpr int kButtonFontSize = 24;
	constexpr int kButtonFontThick = 6;

	//全体の大きさ
	constexpr float kUIScale = 0.8f;

	//位置
	constexpr float kEnemyCenterHeight = 100.0f;//敵の足元から体の中心まで(3D)

	//色
	constexpr int kBlueR = 24;
	constexpr int kBlueG = 132;
	constexpr int kBlueB = 255;
	constexpr int kLightR = 150;//明るい青
	constexpr int kLightG = 228;
	constexpr int kLightB = 255;
	constexpr int kNavyR = 6;
	constexpr int kNavyG = 34;
	constexpr int kNavyB = 120;
	constexpr int kTextR = 255;
	constexpr int kTextG = 255;
	constexpr int kTextB = 255;

	//刃(大きさは設計解像度)
	constexpr float kBladeImageWidth = 256.0f;
	constexpr float kBladeImageHeight = 40.0f;
	constexpr float kBladeLength = 176.0f;
	constexpr float kBladeThick = 22.0f;
	constexpr float kBladeGlowLengthRate = 1.08f;//光の長さ(刃に対して)
	constexpr float kBladeGlowThickRate = 2.4f;//光の太さ(刃に対して)
	constexpr float kBladeAngle = DX_PI_F * 0.25f;
	constexpr float kBladeOpenFrame = 7.0f;//伸びきるまで
	constexpr float kSecondBladeDelay = 4.0f;//2本目が走り始めるまで

	//表示中の明滅
	constexpr float kPulseSpeed = 0.1f;
	constexpr float kPulseAlpha = 150.0f;

	//残りわずか
	constexpr float kWarningRate = 0.3f;
	constexpr int kWarningBlinkFrame = 5;
	constexpr int kWarningAlpha = 170;

	//後ろの光
	constexpr float kGlowImageSize = 256.0f;
	constexpr float kGlowRadius = 110.0f;

	//真ん中
	constexpr float kDiscImageSize = 256.0f;
	constexpr float kButtonImageSize = 128.0f;
	constexpr float kCenterRadius = 40.0f;//リングまで丸に乗せる
	constexpr float kButtonRadius = 25.0f;
	constexpr int kCenterPosNum = 48;//円の頂点数
	constexpr float kPopScale = 1.3f;//出始めの大きさ
	constexpr float kPopFrame = 6.0f;

	//残り時間のリング
	constexpr float kRingRadius = 31.0f;
	constexpr float kRingThick = 4.5f;
	constexpr int kRingDivNum = 64;//リングの分割数
	constexpr int kRingBaseAlpha = 90;

	//帯
	constexpr float kBandX = 26.0f;//×の中心から帯の左端まで
	constexpr float kBandWidth = 330.0f;//右の方は透明になる
	constexpr float kBandHeight = 40.0f;
	constexpr float kBandStartFrame = 8.0f;
	constexpr float kBandOpenFrame = 10.0f;
	constexpr float kLabelX = 24.0f;//帯の左端から文字の左端まで

	//Yを押したときのリアクション
	constexpr float kExitFrame = 18.0f;//全体の長さ
	constexpr float kExitWhiteFrame = 2.0f;//刃が白くなるまで
	constexpr float kExitLengthScale = 1.4f;//刃をどこまで長くするか
	constexpr float kExitThinRate = 0.25f;//最後の刃の太さ
	constexpr float kExitFlashFrame = 10.0f;//白い光が消えるまで
	constexpr float kExitFlashAlpha = 220.0f;
	constexpr float kExitFadeStartFrame = 6.0f;//全体が消え始める
	constexpr float kExitBandMove = 80.0f;//帯が流れる量
	constexpr float kRippleStartRadius = 40.0f;//波紋
	constexpr float kRippleEndRadius = 240.0f;
	constexpr float kRippleThick = 5.0f;

	//時間切れ
	constexpr float kFadeFrame = 20.0f;
	constexpr float kFadeScale = 0.12f;//どこまで縮めるか
}

GuardBreakChanceUI::GuardBreakChanceUI(std::weak_ptr<Player> player) :
	m_player(player)
{
}

GuardBreakChanceUI::~GuardBreakChanceUI()
{
	DeleteGraph(m_bladeHandle);
	DeleteGraph(m_bladeGlowHandle);
	DeleteGraph(m_whiteBladeHandle);
	DeleteGraph(m_glowHandle);
	DeleteGraph(m_discHandle);
	DeleteGraph(m_buttonHandle);
	DeleteGraph(m_bandHandle);
	DeleteFontToHandle(m_labelFontHandle);
	DeleteFontToHandle(m_buttonFontHandle);
}

void GuardBreakChanceUI::Init()
{
	//縮小でふちがにじまないようにする
	SetUsePremulAlphaConvertLoad(TRUE);
	m_bladeHandle = LoadGraph(kBladePath);
	m_bladeGlowHandle = LoadGraph(kBladeGlowPath);
	m_whiteBladeHandle = LoadGraph(kWhiteBladePath);
	m_glowHandle = LoadGraph(kGlowPath);
	m_discHandle = LoadGraph(kDiscPath);
	m_buttonHandle = LoadGraph(kButtonPath);
	m_bandHandle = LoadGraph(kBandPath);
	SetUsePremulAlphaConvertLoad(FALSE);

	const float scale = Game::GetScale() * kUIScale;
	m_labelFontHandle = CreateFontToHandle(kLabelFontName, static_cast<int>(kLabelFontSize * scale), kLabelFontThick, DX_FONTTYPE_ANTIALIASING_EDGE_4X4, -1, kLabelFontEdge);
	SetFontSpaceToHandle(static_cast<int>(kLabelFontSpace * scale), m_labelFontHandle);
	m_buttonFontHandle = CreateFontToHandle(kButtonFontName, static_cast<int>(kButtonFontSize * scale), kButtonFontThick, DX_FONTTYPE_ANTIALIASING_4X4);

	m_isVisible = false;
}

void GuardBreakChanceUI::Update()
{
	if (m_isExiting)
	{
		m_exitFrame++;
		if (m_exitFrame >= kExitFrame) Hide();
		return;
	}

	auto player = m_player.lock();
	auto chance = player ? player->GetGuardBreakChanceState() : nullptr;
	if (chance)
	{
		//消えている途中に次のチャンスが来たらやり直す
		if (m_isFading) Hide();
		m_target = chance->GetTarget();
		m_remainRate = chance->GetRemainRate();
		m_frame++;
		m_isVisible = true;
		return;
	}

	if (!m_isVisible) return;

	//Yを押したらリアクションに入る
	if (!m_isFading && m_isOnScreen && player && player->IsGuardBreakAttacking())
	{
		m_isExiting = true;
		return;
	}

	//時間切れ
	m_isFading = true;
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame) Hide();
}

void GuardBreakChanceUI::Draw()
{
	if (!m_isVisible) return;

	//消えている間はカメラが動くので位置を止める
	if (!m_isExiting && !m_isFading)
	{
		auto target = m_target.lock();
		if (!target) return;

		//敵の座標から体の中心を求めて画面の座標にする
		const Vector3 uiPos = target->GetRigidBody().GetPos() + Vector3(0.0f, kEnemyCenterHeight, 0.0f);
		const VECTOR screenPos = ConvWorldPosToScreenPos(uiPos.ToDxLibVector());
		//カメラの後ろなら出さない
		m_isOnScreen = screenPos.z > 0.0f && screenPos.z < 1.0f;
		if (!m_isOnScreen) return;

		m_drawX = screenPos.x;
		m_drawY = screenPos.y;
	}
	if (!m_isOnScreen) return;

	float scale = Game::GetScale() * kUIScale;
	if (m_isFading) scale *= 1.0f - kFadeScale * (m_fadeFrame / kFadeFrame);

	SetDrawMode(DX_DRAWMODE_BILINEAR);
	//後ろの光
	DrawGlow(m_drawX, m_drawY, scale);
	//剣の刃
	DrawBlades(m_drawX, m_drawY, scale);
	//帯と文字
	DrawBand(m_drawX, m_drawY, scale);
	DrawLabel(m_drawX, m_drawY, scale);
	//ボタンとリング
	DrawCenter(m_drawX, m_drawY, scale);
	//押したときの波紋
	DrawRipple(m_drawX, m_drawY, scale);

	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void GuardBreakChanceUI::Hide()
{
	m_target.reset();
	m_frame = 0;
	m_remainRate = 1.0f;
	m_isExiting = false;
	m_exitFrame = 0;
	m_isFading = false;
	m_fadeFrame = 0;
	m_isOnScreen = false;
	m_isVisible = false;
}

float GuardBreakChanceUI::GetBladeOpenRate(float delay) const
{
	//だんだん遅くなるように伸ばす
	float rate = (m_frame - delay) / kBladeOpenFrame;
	if (rate <= 0.0f) return 0.0f;
	if (rate > 1.0f) rate = 1.0f;
	return 1.0f - (1.0f - rate) * (1.0f - rate);
}

float GuardBreakChanceUI::GetAlpha() const
{
	if (m_isFading) return 1.0f - m_fadeFrame / kFadeFrame;

	const float rate = (m_exitFrame - kExitFadeStartFrame) / (kExitFrame - kExitFadeStartFrame);
	if (rate <= 0.0f) return 1.0f;
	if (rate >= 1.0f) return 0.0f;
	return 1.0f - rate;
}

bool GuardBreakChanceUI::IsWarning() const
{
	if (m_isExiting || m_isFading) return false;
	if (m_remainRate >= kWarningRate) return false;
	return (m_frame / kWarningBlinkFrame) % 2 == 0;
}

void GuardBreakChanceUI::DrawBlades(float x, float y, float scale)
{
	//押したら長く細くして、弾けるように消す
	const float exitRate = m_exitFrame / kExitFrame;
	const float length = kBladeLength * scale * (1.0f + (kExitLengthScale - 1.0f) * exitRate);
	const float thick = kBladeThick * scale * (1.0f - (1.0f - kExitThinRate) * exitRate);
	const float alpha = GetAlpha();

	//刃の周りの光
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * alpha));
	DrawBladePair(m_bladeGlowHandle, x, y, length * kBladeGlowLengthRate, thick * kBladeGlowThickRate);

	//色は画像に入っている
	DrawBladePair(m_bladeHandle, x, y, length, thick);

	//同じ刃を足して明滅させる
	const float pulse = (std::sin(m_frame * kPulseSpeed) + 1.0f) * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, static_cast<int>(kPulseAlpha * pulse * alpha));
	DrawBladePair(m_bladeHandle, x, y, length, thick);

	//残りわずかなら明るい青を重ねる
	if (IsWarning())
	{
		SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, kWarningAlpha);
		SetDrawBright(kLightR, kLightG, kLightB);
		DrawBladePair(m_whiteBladeHandle, x, y, length, thick);
		SetDrawBright(255, 255, 255);
	}

	//押したら白を重ねる
	float whiteRate = m_exitFrame / kExitWhiteFrame;
	if (whiteRate > 1.0f) whiteRate = 1.0f;
	if (whiteRate <= 0.0f) return;
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * whiteRate * alpha));
	DrawBladePair(m_whiteBladeHandle, x, y, length, thick);
}

void GuardBreakChanceUI::DrawGlow(float x, float y, float scale)
{
	float openRate = m_frame / kBladeOpenFrame;
	if (openRate > 1.0f) openRate = 1.0f;

	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * GetAlpha() * openRate));
	const double rate = kGlowRadius * 2.0f * scale / kGlowImageSize;
	DrawRotaGraphF(x, y, rate, 0.0, m_glowHandle, TRUE);
}

void GuardBreakChanceUI::DrawBladePair(int handle, float x, float y, float length, float thick)
{
	//左上から右下、右上から左下の順に走らせる
	DrawBlade(handle, x, y, kBladeAngle, GetBladeOpenRate(0.0f), length, thick);
	DrawBlade(handle, x, y, DX_PI_F - kBladeAngle, GetBladeOpenRate(kSecondBladeDelay), length, thick);
}

void GuardBreakChanceUI::DrawBlade(int handle, float x, float y, float angle, float openRate, float length, float thick)
{
	if (openRate <= 0.0f) return;

	//刃の根本を軸にして、長さの方向にだけ伸ばす
	const float startX = x - std::cos(angle) * length * 0.5f;
	const float startY = y - std::sin(angle) * length * 0.5f;
	const double rateX = length * openRate / kBladeImageWidth;
	const double rateY = thick / kBladeImageHeight;
	DrawRotaGraph3F(startX, startY, 0.0f, kBladeImageHeight * 0.5f, rateX, rateY, angle, handle, TRUE);
}

void GuardBreakChanceUI::DrawBand(float x, float y, float scale)
{
	//だんだん遅くなるように伸ばす
	float rate = (m_frame - kBandStartFrame) / kBandOpenFrame;
	if (rate <= 0.0f) return;
	if (rate > 1.0f) rate = 1.0f;
	rate = 1.0f - (1.0f - rate) * (1.0f - rate);

	//押したら右へ流す
	const float exitRate = m_exitFrame / kExitFrame;
	const float left = x + (kBandX + kExitBandMove * exitRate) * scale;
	const float right = left + kBandWidth * scale * rate;
	const float top = y - kBandHeight * scale * 0.5f;
	const float bottom = y + kBandHeight * scale * 0.5f;

	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * GetAlpha()));
	DrawModiGraphF(left, top, right, top, right, bottom, left, bottom, m_bandHandle, TRUE);
}

void GuardBreakChanceUI::DrawLabel(float x, float y, float scale)
{
	//帯が伸びきってから出す
	if (m_frame < kBandStartFrame + kBandOpenFrame) return;

	const float exitRate = m_exitFrame / kExitFrame;
	const float textX = x + (kBandX + kLabelX + kExitBandMove * exitRate) * scale;
	const float textY = y - GetFontSizeToHandle(m_labelFontHandle) * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * GetAlpha()));
	DrawStringFToHandle(textX, textY, kLabel, GetColor(kTextR, kTextG, kTextB), m_labelFontHandle, GetColor(kNavyR, kNavyG, kNavyB));
}

void GuardBreakChanceUI::DrawCenter(float x, float y, float scale)
{
	//出始めだけ大きくする
	float pop = 1.0f;
	if (m_frame < kPopFrame) pop += (kPopScale - 1.0f) * (1.0f - m_frame / kPopFrame);

	const float alpha = GetAlpha();
	const float radius = kCenterRadius * scale * pop;

	//青の丸
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * alpha));
	DrawRotaGraphF(x, y, radius * 2.0f / kDiscImageSize, 0.0, m_discHandle, TRUE);

	//残り時間のリング
	const float ringRadius = kRingRadius * scale * pop;
	const float ringThick = kRingThick * scale;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kRingBaseAlpha * alpha));
	DrawRing(x, y, ringRadius, 1.0f, ringThick, GetColor(kBlueR, kBlueG, kBlueB));
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	DrawRing(x, y, ringRadius, m_remainRate, ringThick, GetColor(kLightR, kLightG, kLightB));

	//Yボタン
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * alpha));
	DrawRotaGraphF(x, y, kButtonRadius * 2.0f * scale * pop / kButtonImageSize, 0.0, m_buttonHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	const int textWidth = GetDrawStringWidthToHandle(kButtonLabel, 1, m_buttonFontHandle);
	const float textX = x - textWidth * 0.5f;
	const float textY = y - GetFontSizeToHandle(m_buttonFontHandle) * 0.5f;
	DrawStringFToHandle(textX, textY, kButtonLabel, GetColor(kTextR, kTextG, kTextB), m_buttonFontHandle);

	//押したときの光
	if (!m_isExiting) return;
	const float flashRate = 1.0f - m_exitFrame / kExitFlashFrame;
	if (flashRate <= 0.0f) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kExitFlashAlpha * flashRate));
	DrawCircleAA(x, y, radius, kCenterPosNum, GetColor(255, 255, 255), TRUE);
}

void GuardBreakChanceUI::DrawRing(float x, float y, float radius, float rate, float thick, unsigned int color)
{
	if (rate <= 0.0f) return;

	//真上から時計回りに線でつなぐ
	const int divNum = static_cast<int>(std::ceil(kRingDivNum * rate));
	const float angleStep = DX_TWO_PI_F * rate / divNum;
	float angle = -DX_PI_F * 0.5f;
	for (int i = 0; i < divNum; i++)
	{
		const float nextAngle = angle + angleStep;
		DrawLineAA(x + std::cos(angle) * radius, y + std::sin(angle) * radius,
			x + std::cos(nextAngle) * radius, y + std::sin(nextAngle) * radius, color, thick);
		angle = nextAngle;
	}
}

void GuardBreakChanceUI::DrawRipple(float x, float y, float scale)
{
	if (!m_isExiting) return;

	//だんだん遅くなるように広げる
	float rate = m_exitFrame / kExitFrame;
	if (rate > 1.0f) rate = 1.0f;
	const float easeRate = 1.0f - (1.0f - rate) * (1.0f - rate);
	const float radius = (kRippleStartRadius + (kRippleEndRadius - kRippleStartRadius) * easeRate) * scale;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * (1.0f - rate)));
	DrawCircleAA(x, y, radius, kCenterPosNum, GetColor(kLightR, kLightG, kLightB), FALSE, kRippleThick * (1.0f - rate) * scale + 1.0f);
}
