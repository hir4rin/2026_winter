#include "ResultUI.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "DxLib.h"
#include "Main/Game.h"
#include "UIFont.h"

namespace
{
	const char* const kFramePath = "data/UI/Menu/frame9.png";
	const char* const kSelectBasePath = "data/UI/Menu/select_base.png";
	const char* const kHalftonePath = "data/UI/Menu/halftone.png";
	const char* const kLineFadePath = "data/UI/Menu/line_fade.png";
	const char* const kGridPath = "data/UI/Menu/grid.png";
	const char* const kRainPath = "data/UI/Menu/rain.png";
	const char* const kVignettePath = "data/UI/Menu/vignette.png";
	const char* const kButtonAPath = "data/UI/button/A.png";
	const char* const kButtonYPath = "data/UI/button/Y.png";

	constexpr int kFontThick = 6;

	//色
	constexpr int kRedR = 230, kRedG = 45, kRedB = 70;
	constexpr int kBgR = 14, kBgG = 8, kBgB = 16;
	const unsigned int kWhite = 0xE8E2D6;
	const unsigned int kDim = 0x9A9389;
	const unsigned int kRed = 0xE62D46;
	const unsigned int kRailColor = 0x6B655D;
	const unsigned int kDarkRed = 0x5A2422;
	const unsigned int kTeal = 0x5DCAA5;

	//背景(ステージセレクトのMenuUIと同じ値)
	constexpr float kGridTile = 64.0f;
	constexpr int kGridAlpha = 14;
	constexpr float kRainTile = 256.0f;
	constexpr float kRainSpeed = 0.004f;
	constexpr int kRainAlpha = 170;

	//配置
	constexpr float kHeaderX1 = 40.0f, kHeaderX2 = 1240.0f, kHeaderLineY = 66.0f, kHeaderTextY = 24.0f;
	constexpr float kRailLX = 402.0f, kRailRX = 712.0f, kRailY1 = 106.0f, kRailY2 = 632.0f;
	constexpr float kTopLineX2 = 1089.0f;
	constexpr float kTechY = 146.0f, kKillY = 286.0f, kTimeY = 436.0f;
	constexpr float kItemY = 146.0f, kDeathY = 286.0f;
	constexpr float kTextOffsetX = 42.0f;
	constexpr float kIconOffsetY = 22.0f;
	constexpr float kIconRadius = 24.0f;
	constexpr float kLabelOffsetY = 48.0f, kSubOffsetY = 78.0f;
	constexpr float kFormulaY = 586.0f;
	constexpr float kPanelX1 = 932.0f, kPanelY1 = 146.0f, kPanelX2 = 1246.0f, kPanelY2 = 632.0f;
	constexpr float kPanelPad = 18.0f;
	constexpr float kDetailRowY = 262.0f, kDetailPitch = 46.0f;
	constexpr float kDetailCountRight = 1124.0f;//回数は右揃え
	constexpr float kLeftX = 46.0f;
	constexpr float kStampX = 196.0f, kStampY = 232.0f;//紋章の中心
	constexpr float kEmblemSize = 320.0f;//紋章を描く大きさ(画像は正方形)
	constexpr float kRankLabelY = 394.0f, kRankLineY = 424.0f, kRankLineX2 = 330.0f;
	constexpr float kTotalLabelY = 452.0f, kTotalY = 492.0f;
	constexpr float kNewRecordY = 104.0f;
	constexpr float kGuideY = 676.0f, kGuideLineX1 = 380.0f, kGuideLineX2 = 900.0f, kGuideIconSize = 28.0f;

	//出る順番(実フレーム)
	constexpr int kFadeFrame = 12;
	constexpr int kHeaderLineStart = 0, kHeaderTextStart = 9, kStageClearStart = 15;
	constexpr int kRailLStart = 24, kRailRStart = 29, kRailLen = 20;
	constexpr int kTopLineStart = 34, kTopLineLen = 14, kDownLineStart = 41, kDownLineLen = 8;
	constexpr int kPanelStart = 43, kDetailRowStart = 48, kDetailRowStep = 4;
	constexpr int kTechStart = 45, kKillStart = 81, kTimeStart = 117, kCountLen = 33;
	constexpr int kItemStart = 153, kDeathStart = 168;
	constexpr int kTotalStart = 186, kTotalCountStart = 189, kTotalCountLen = 40;
	constexpr int kDarkStart = 228, kDarkInLen = 12, kDarkOutLen = 8, kDarkAlpha = 110;
	constexpr int kStampStart = 249, kStampLen = 12, kShakeLen = 17;
	constexpr int kNewRecordStart = 273, kBlinkLen = 45, kBlinkPeriod = 15;
	constexpr int kGuideStart = 288;
	constexpr int kEndFrame = 300;
	constexpr int kPopLen = 13;

	//紋章(上から押す)
	constexpr float kStampStartScale = 2.8f;
	constexpr float kStampOverScale = 0.93f;
	constexpr float kStampHitRate = 0.8f;
	constexpr float kShakePower = 10.0f;
}

ResultUI::~ResultUI()
{
	DeleteGraph(m_frameHandle);
	DeleteGraph(m_selectBaseHandle);
	DeleteGraph(m_halftoneHandle);
	DeleteGraph(m_lineFadeHandle);
	DeleteGraph(m_gridHandle);
	DeleteGraph(m_rainHandle);
	DeleteGraph(m_vignetteHandle);
	DeleteGraph(m_emblemHandle);
	DeleteGraph(m_buttonAHandle);
	DeleteGraph(m_buttonYHandle);
	DeleteFontToHandle(m_headerFont);
	DeleteFontToHandle(m_valueFont);
	DeleteFontToHandle(m_pointFont);
	DeleteFontToHandle(m_labelFont);
	DeleteFontToHandle(m_smallFont);
	DeleteFontToHandle(m_scoreFont);
	DeleteFontToHandle(m_iconFont);
}

void ResultUI::Init(const ResultData& data, const std::string& stageName, bool isNewRecord)
{
	m_data = data;
	m_stageName = stageName;
	m_isNewRecord = isNewRecord;
	m_rank = data.CalcRank();
	m_frame = 0;
	m_bgFrame = 0;

	m_frameHandle = LoadGraph(kFramePath);
	m_selectBaseHandle = LoadGraph(kSelectBasePath);
	m_halftoneHandle = LoadGraph(kHalftonePath);
	m_lineFadeHandle = LoadGraph(kLineFadePath);
	m_gridHandle = LoadGraph(kGridPath);
	m_rainHandle = LoadGraph(kRainPath);
	m_vignetteHandle = LoadGraph(kVignettePath);
	m_emblemHandle = LoadEmblem(m_rank);
	m_buttonAHandle = LoadGraph(kButtonAPath);
	m_buttonYHandle = LoadGraph(kButtonYPath);

	const float scale = Game::GetScale();
	auto create = [scale](int size, int space)
		{
			const int font = UIFont::Create(static_cast<int>(size * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_4X4);
			SetFontSpaceToHandle(static_cast<int>(space * scale), font);
			return font;
		};
	m_headerFont = create(22, 4);
	m_valueFont = create(30, 1);
	m_pointFont = create(24, 1);
	m_labelFont = create(20, 2);
	m_smallFont = create(18, 1);
	m_scoreFont = create(60, 2);
	m_iconFont = create(24, 0);
}

int ResultUI::LoadEmblem(int rank)
{
	//tools/RankEmblemGen/MakeRankEmblem.ps1 で作る(番号は ResultData::CalcRank と同じ)
	static constexpr const char* kEmblemPaths[ResultData::kRankNum] =
	{
		"data/UI/Result/rank_0_C.png",
		"data/UI/Result/rank_1_B.png",
		"data/UI/Result/rank_2_A.png",
		"data/UI/Result/rank_3_S.png",
		"data/UI/Result/rank_4_SSS.png",
	};
	//ふちの光がにじまないように乗算済みアルファで読む
	SetUsePremulAlphaConvertLoad(TRUE);
	const int handle = LoadGraph(kEmblemPaths[std::clamp(rank, 0, ResultData::kRankNum - 1)]);
	SetUsePremulAlphaConvertLoad(FALSE);
	return handle;
}

void ResultUI::Update()
{
	m_frame = (std::min)(m_frame + 1, kEndFrame);
	//スキップしても背景のドットは飛ばない
	m_bgFrame++;

	//ハンコで揺らす(上のランクほど強い)
	const int shakeFrame = m_frame - kStampStart - kStampLen * 4 / 5;
	if (shakeFrame >= 0 && shakeFrame < kShakeLen)
	{
		const float power = kShakePower * (0.6f + 0.2f * m_rank) * (1.0f - static_cast<float>(shakeFrame) / kShakeLen);
		m_shakeX = power * std::sin(shakeFrame * 2.1f);
		m_shakeY = power * std::cos(shakeFrame * 2.7f);
	}
	else
	{
		m_shakeX = 0.0f;
		m_shakeY = 0.0f;
	}
}

void ResultUI::Skip()
{
	m_frame = (m_frame < kDarkStart) ? kDarkStart : kEndFrame;
}

bool ResultUI::IsFinished() const
{
	return m_frame >= kGuideStart;
}

void ResultUI::Draw()
{
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawBackground();
	DrawHeader();
	DrawRails();
	DrawAddRows();
	DrawRateRows();
	DrawDetailPanel();
	DrawTotal();
	DrawDark();
	DrawStamp();
	DrawNewRecord();
	DrawGuide();
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
}

void ResultUI::DrawBackground()
{
	const int w = Game::GetScreenWidth();
	const int h = Game::GetScreenHeight();
	DrawBox(0, 0, w, h, GetColor(kBgR, kBgG, kBgB), TRUE);

	//斜めの格子と流れるドット(加算)
	const float dw = static_cast<float>(Game::kScreenWidth);
	const float dh = static_cast<float>(Game::kScreenHeight);
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
	DrawTiled(m_gridHandle, 0, 0, dw, dh, kGridTile, 0.0f, 0.0f, kRedR, kRedG, kRedB, kGridAlpha);
	DrawTiled(m_rainHandle, 0, 0, dw, dh, kRainTile, 0.0f, -m_bgFrame * kRainSpeed, kRedR, kRedG, kRedB, kRainAlpha);

	SetDrawBlendMode(DX_BLENDMODE_MUL, 255);
	DrawExtendGraph(0, 0, w, h, m_vignetteHandle, FALSE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawHeader()
{
	DrawGrowLineH(kHeaderX1, kHeaderX2, kHeaderLineY, kHeaderLineStart, kRailLen, kDarkRed);
	//出始めはちらつかせる
	const int textFrame = m_frame - kHeaderTextStart;
	const bool isFlicker = textFrame >= 0 && textFrame < 6 && (textFrame % 2) == 1;
	const int alpha = isFlicker ? FadeAlpha(kHeaderTextStart) / 3 : FadeAlpha(kHeaderTextStart);
	DrawLabel(kHeaderX1, kHeaderTextY, m_stageName + "  ───  STAGE CLEAR", kRed, m_headerFont, alpha);
	DrawLabelRight(kHeaderX2, kHeaderTextY + 4.0f, "RESULT", kDim, m_smallFont, FadeAlpha(kStageClearStart));
}

void ResultUI::DrawRails()
{
	DrawGrowLineV(kRailLX, kRailY1, kRailY2, kRailLStart, kRailLen, kRailColor);
	DrawGrowLineV(kRailRX, kRailY1, kRailY2, kRailRStart, kRailLen, kRailColor);
	DrawGrowLineH(kRailLX, kTopLineX2, kRailY1, kTopLineStart, kTopLineLen, kRed);
	DrawGrowLineV(kTopLineX2, kRailY1, kPanelY1, kDownLineStart, kDownLineLen, kRed);
}

void ResultUI::DrawAddRows()
{
	//操作できるようになったら、詳細データを出している行に帯
	DrawSelectBand(kRailLX + 4.0f, kTechY - 6.0f, kRailRX - 14.0f, kTechY + 70.0f, kGuideStart);

	const float textX = kRailLX + kTextOffsetX;
	DrawIcon(kRailLX, kTechY + kIconOffsetY, "技", kTechStart);
	DrawRowText(textX, kTechY, FormatNumber(CountUp(m_data.CalcTechniqueTotal(), kTechStart, kCountLen)), "", "【テクニック】", "", kTechStart, false);

	const int kill = (std::min)(m_data.killCount, m_data.enemyNum);
	const int killPercent = m_data.enemyNum > 0 ? kill * 100 / m_data.enemyNum : 100;
	DrawIcon(kRailLX, kKillY + kIconOffsetY, "切", kKillStart);
	DrawRowText(textX, kKillY, FormatNumber(CountUp(m_data.CalcKillScore(), kKillStart, kCountLen)), " / " + FormatNumber(ResultData::GetKillScoreMax()),
		"【切る数】", std::to_string(kill) + " / " + std::to_string(m_data.enemyNum) + "体　" + std::to_string(killPercent) + "%", kKillStart, false);

	DrawIcon(kRailLX, kTimeY + kIconOffsetY, "時", kTimeStart);
	DrawRowText(textX, kTimeY, FormatNumber(CountUp(m_data.CalcTimeScore(), kTimeStart, kCountLen)), " / " + FormatNumber(ResultData::GetTimeScoreMax()),
		"【クリアタイム】", FormatTime(m_data.clearFrame), kTimeStart, false);

	DrawLabel(textX, kFormulaY, "( 技 + 切 + 時 ) × 道 × 死", kDim, m_smallFont, FadeAlpha(kTotalStart));
}

void ResultUI::DrawRateRows()
{
	const float textX = kRailRX + kTextOffsetX;
	DrawIcon(kRailRX, kItemY + kIconOffsetY, "道", kItemStart);
	DrawRowText(textX, kItemY, FormatRate(m_data.CalcItemRate()), "", "【アイテム】", "使用 " + std::to_string(m_data.itemUseCount), kItemStart, true);
	DrawIcon(kRailRX, kDeathY + kIconOffsetY, "死", kDeathStart);
	DrawRowText(textX, kDeathY, FormatRate(m_data.CalcDeathRate()), "", "【死亡】", "死亡 " + std::to_string(m_data.deathCount), kDeathStart, true);
}

void ResultUI::DrawDetailPanel()
{
	const int alpha = FadeAlpha(kPanelStart);
	if (alpha <= 0) return;
	DrawFrame(kPanelX1, kPanelY1, kPanelX2, kPanelY2, alpha);
	const float x = kPanelX1 + kPanelPad;
	const float right = kPanelX2 - kPanelPad;
	DrawLabel(x, kPanelY1 + 20.0f, "【テクニック】", kRed, m_labelFont, alpha);
	DrawLabel(x, kPanelY1 + 54.0f, "詳細データ", kWhite, m_labelFont, alpha);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	SetDrawBright(kRedR, kRedG, kRedB);
	DrawExtendGraphF(ToScreenX(x), ToScreenY(kPanelY1 + 88.0f), ToScreenX(right), ToScreenY(kPanelY1 + 90.0f), m_lineFadeHandle, TRUE);
	SetDrawBright(255, 255, 255);

	using T = ResultData::Technique;
	struct Row { const char* name; std::string count; int score; };
	const Row rows[] =
	{
		{ "コンボ", "最大" + std::to_string(m_data.maxCombo), m_data.CalcComboScore() },
		{ "部位破壊", "x" + std::to_string(m_data.GetTechniqueCount(T::PartBreak)), m_data.CalcTechniqueScore(T::PartBreak) },
		{ "確殺", "x" + std::to_string(m_data.GetTechniqueCount(T::PartBrokenKill)), m_data.CalcTechniqueScore(T::PartBrokenKill) },
		{ "暗殺", "x" + std::to_string(m_data.GetTechniqueCount(T::Assasin)), m_data.CalcTechniqueScore(T::Assasin) },
		{ "スキル技", "x" + std::to_string(m_data.GetTechniqueCount(T::Skill)), m_data.CalcTechniqueScore(T::Skill) },
		{ "スキル血殺", "x" + std::to_string(m_data.GetTechniqueCount(T::SkillKessatsu)), m_data.CalcTechniqueScore(T::SkillKessatsu) },
		{ "真必殺技", "x" + std::to_string(m_data.GetTechniqueCount(T::TrueUlt)), m_data.CalcTechniqueScore(T::TrueUlt) },
	};
	for (int i = 0; i < static_cast<int>(std::size(rows)); ++i)
	{
		const int rowAlpha = FadeAlpha(kDetailRowStart + kDetailRowStep * i);
		if (rowAlpha <= 0) continue;
		const float y = kDetailRowY + kDetailPitch * i;
		//0回の技は暗く
		const bool isZero = rows[i].score == 0;
		DrawLabel(x, y + 2.0f, rows[i].name, isZero ? kDim : kWhite, m_smallFont, rowAlpha);
		DrawLabelRight(kDetailCountRight, y + 2.0f, rows[i].count, kDim, m_smallFont, rowAlpha);
		DrawLabelRight(right, y - 2.0f, FormatNumber(rows[i].score), isZero ? kDim : kWhite, m_pointFont, rowAlpha);
	}
}

void ResultUI::DrawTotal()
{
	const int alpha = FadeAlpha(kTotalStart);
	if (alpha <= 0) return;
	DrawLabel(kLeftX, kRankLabelY, "クリア評価", kDim, m_smallFont, alpha);
	DrawGrowLineH(kLeftX, kRankLineX2, kRankLineY, kTotalStart, kRailLen, kDarkRed);
	//ランクの名前は紋章が当たってから出す
	DrawLabelRight(kRankLineX2, kRankLabelY - 8.0f, ResultData::GetRankName(m_rank), kWhite, m_valueFont, FadeAlpha(kStampStart + kStampLen * 4 / 5));

	//「総合スコア」の枠
	const int labelW = GetDrawStringWidthToHandle("総合スコア", static_cast<int>(strlen("総合スコア")), m_labelFont);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	const float boxX2 = ToScreenX(kLeftX + 16.0f) + labelW + ToScreenX(16.0f) - ToScreenX(0.0f);
	DrawBoxAA(ToScreenX(kLeftX), ToScreenY(kTotalLabelY), boxX2, ToScreenY(kTotalLabelY + 30.0f), kRed, FALSE, 1.5f);
	DrawLabel(kLeftX + 16.0f, kTotalLabelY + 3.0f, "総合スコア", kRed, m_labelFont, alpha);

	DrawLabel(kLeftX, kTotalY, FormatNumber(CountUp(m_data.CalcTotalScore(), kTotalCountStart, kTotalCountLen)), kWhite, m_scoreFont, alpha);
}

void ResultUI::DrawDark()
{
	//ハンコの前のため
	float rate = 0.0f;
	if (m_frame >= kDarkStart && m_frame < kStampStart) rate = Rate(kDarkStart, kDarkInLen);
	else if (m_frame >= kStampStart) rate = 1.0f - Rate(kStampStart, kDarkOutLen);
	if (rate <= 0.0f) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kDarkAlpha * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawStamp()
{
	if (m_frame < kStampStart) return;
	const float scaleUI = Game::GetScale();
	const float cx = ToScreenX(kStampX);
	const float cy = ToScreenY(kStampY);

	//上から押す
	const float t = Rate(kStampStart, kStampLen);
	float scale = 1.0f;
	if (t < kStampHitRate)
	{
		const float k = t / kStampHitRate;
		scale = kStampStartScale + (kStampOverScale - kStampStartScale) * k * k;
	}
	else
	{
		const float k = (t - kStampHitRate) / (1.0f - kStampHitRate);
		scale = kStampOverScale + (1.0f - kStampOverScale) * k;
	}
	const int alpha = static_cast<int>(255 * (std::min)(1.0f, t * 3.0f));

	//紋章の画像(乗算済みアルファ)
	int imageW = 0, imageH = 0;
	GetGraphSize(m_emblemHandle, &imageW, &imageH);
	if (imageW <= 0) return;
	const float exRate = kEmblemSize / imageW * scale * scaleUI;
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, alpha);
	DrawRotaGraphF(cx, cy, exRate, 0.0, m_emblemHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawNewRecord()
{
	if (!m_isNewRecord || m_frame < kNewRecordStart) return;
	const int f = m_frame - kNewRecordStart;
	if (f < kBlinkLen && (f % kBlinkPeriod) >= kBlinkPeriod / 2) return;
	const char* text = "新記録";
	const int w = GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), m_smallFont);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawBoxAA(ToScreenX(kLeftX), ToScreenY(kNewRecordY), ToScreenX(kLeftX + 20.0f) + w, ToScreenY(kNewRecordY + 26.0f), kTeal, FALSE, 1.5f);
	DrawLabel(kLeftX + 10.0f, kNewRecordY + 3.0f, text, kTeal, m_smallFont, 255);
}

void ResultUI::DrawGuide()
{
	const int alpha = FadeAlpha(kGuideStart);
	if (alpha <= 0) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	SetDrawBright(kRedR, kRedG, kRedB);
	DrawExtendGraphF(ToScreenX(kGuideLineX1), ToScreenY(kGuideY - 1.0f), ToScreenX(kGuideLineX2), ToScreenY(kGuideY + 2.0f), m_lineFadeHandle, TRUE);
	SetDrawBright(255, 255, 255);

	const float cx = Game::kScreenWidth * 0.5f;
	const float iconY = kGuideY - kGuideIconSize * 0.5f;
	const float aX = cx - 150.0f;
	const float yX = cx + 30.0f;
	DrawBox(static_cast<int>(ToScreenX(aX - 12.0f)), static_cast<int>(ToScreenY(iconY - 2.0f)), static_cast<int>(ToScreenX(yX + 140.0f)), static_cast<int>(ToScreenY(iconY + kGuideIconSize + 2.0f)), GetColor(kBgR, kBgG, kBgB), TRUE);
	DrawExtendGraphF(ToScreenX(aX), ToScreenY(iconY), ToScreenX(aX + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), m_buttonAHandle, TRUE);
	DrawLabel(aX + kGuideIconSize + 8.0f, iconY + 2.0f, "次へ", kRed, m_smallFont, alpha);
	DrawExtendGraphF(ToScreenX(yX), ToScreenY(iconY), ToScreenX(yX + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), m_buttonYHandle, TRUE);
	DrawLabel(yX + kGuideIconSize + 8.0f, iconY + 2.0f, "リトライ", kRed, m_smallFont, alpha);
}

void ResultUI::DrawIcon(float cx, float cy, const char* kanji, int start)
{
	if (m_frame < start) return;
	const float scale = PopScale(start);
	const int alpha = (std::min)(255, (m_frame - start) * 255 / 4 + 1);
	const float r = kIconRadius * scale * Game::GetScale();
	const float sx = ToScreenX(cx);
	const float sy = ToScreenY(cy);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawCircleAA(sx, sy, r, 32, GetColor(kBgR, kBgG, kBgB), TRUE);
	DrawCircleAA(sx, sy, r, 32, kWhite, FALSE, 1.5f);
	const int len = static_cast<int>(strlen(kanji));
	const int w = GetDrawStringWidthToHandle(kanji, len, m_iconFont);
	const int h = GetFontSizeToHandle(m_iconFont);
	DrawRotaStringToHandle(static_cast<int>(sx), static_cast<int>(sy), scale, scale, w * 0.5, h * 0.5, 0.0, kWhite, m_iconFont, 0, FALSE, kanji);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawRowText(float x, float y, const std::string& value, const std::string& valueSub, const char* label, const std::string& sub, int start, bool isPop)
{
	const int alpha = FadeAlpha(start);
	if (alpha <= 0) return;
	if (isPop)
	{
		//倍率はカウントせずに置く
		const float scale = PopScale(start);
		const int w = GetDrawStringWidthToHandle(value.c_str(), static_cast<int>(value.size()), m_valueFont);
		const int h = GetFontSizeToHandle(m_valueFont);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		DrawRotaStringToHandle(static_cast<int>(ToScreenX(x) + w * 0.5f), static_cast<int>(ToScreenY(y) + h * 0.5f), scale, scale, w * 0.5, h * 0.5, 0.0, kWhite, m_valueFont, 0, FALSE, value.c_str());
	}
	else
	{
		DrawLabel(x, y, value, kWhite, m_valueFont, alpha);
	}
	if (!valueSub.empty())
	{
		const int w = GetDrawStringWidthToHandle(value.c_str(), static_cast<int>(value.size()), m_valueFont);
		DrawLabel(x + w / Game::GetScale() + 4.0f, y + 11.0f, valueSub, kDim, m_smallFont, alpha);
	}
	DrawLabel(x, y + kLabelOffsetY, label, kRed, m_labelFont, FadeAlpha(start + 2));
	if (!sub.empty()) DrawLabel(x, y + kSubOffsetY, sub, kDim, m_smallFont, FadeAlpha(start + 6));
}

void ResultUI::DrawSelectBand(float x1, float y1, float x2, float y2, int start)
{
	const int alpha = FadeAlpha(start);
	if (alpha <= 0) return;
	const float t = Rate(start, 10);
	const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	const float bx2 = x1 + (x2 - x1) * ease;
	//左が濃いグラデを反転して描く
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha * 110 / 255);
	SetDrawBright(kRedR, kRedG, kRedB);
	DrawModiGraphF(ToScreenX(bx2), ToScreenY(y1), ToScreenX(x1), ToScreenY(y1), ToScreenX(x1), ToScreenY(y2), ToScreenX(bx2), ToScreenY(y2), m_selectBaseHandle, TRUE);
	SetDrawBright(255, 255, 255);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawTiled(m_halftoneHandle, x1, y1, bx2, y2, 6.0f, 0.0f, 0.0f, 40, 0, 10, alpha * 70 / 255);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawBoxAA(ToScreenX(x1), ToScreenY(y1), ToScreenX(x1 + 3.0f), ToScreenY(y2), GetColor(255, 90, 110), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawFrame(float x1, float y1, float x2, float y2, int alpha)
{
	//9スライス(MenuUIと同じ画像)
	constexpr int kSize = 48;
	constexpr int kCorner = 16;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	SetDrawBright(kRedR, kRedG, kRedB);
	const float c = static_cast<float>(kCorner);
	const float xs[4] = { x1, x1 + c, x2 - c, x2 };
	const float ys[4] = { y1, y1 + c, y2 - c, y2 };
	const int srcPos[3] = { 0, kCorner, kSize - kCorner };
	const int srcLen[3] = { kCorner, kSize - kCorner * 2, kCorner };
	for (int iy = 0; iy < 3; ++iy)
	{
		for (int ix = 0; ix < 3; ++ix)
		{
			if (ix == 1 && iy == 1) continue;
			DrawRectExtendGraphF(ToScreenX(xs[ix]), ToScreenY(ys[iy]), ToScreenX(xs[ix + 1]), ToScreenY(ys[iy + 1]),
				srcPos[ix], srcPos[iy], srcLen[ix], srcLen[iy], m_frameHandle, TRUE);
		}
	}
	SetDrawBright(255, 255, 255);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawTiled(int handle, float x1, float y1, float x2, float y2, float tileSize, float u, float v, int r, int g, int b, int a)
{
	if (x2 <= x1 || y2 <= y1) return;
	const float u2 = u + (x2 - x1) / tileSize;
	const float v2 = v + (y2 - y1) / tileSize;
	const float sx1 = ToScreenX(x1);
	const float sx2 = ToScreenX(x2);
	const float sy1 = ToScreenY(y1);
	const float sy2 = ToScreenY(y2);
	const COLOR_U8 dif = GetColorU8(r, g, b, a);

	VERTEX2D vtx[6];
	const float px[6] = { sx1, sx2, sx1, sx2, sx2, sx1 };
	const float py[6] = { sy1, sy1, sy2, sy1, sy2, sy2 };
	const float pu[6] = { u, u2, u, u2, u2, u };
	const float pv[6] = { v, v, v2, v, v2, v2 };
	for (int i = 0; i < 6; ++i)
	{
		vtx[i].pos = VGet(px[i], py[i], 0.0f);
		vtx[i].rhw = 1.0f;
		vtx[i].dif = dif;
		vtx[i].u = pu[i];
		vtx[i].v = pv[i];
	}
	SetTextureAddressMode(DX_TEXADDRESS_WRAP);
	DrawPolygon2D(vtx, 2, handle, TRUE);
	SetTextureAddressMode(DX_TEXADDRESS_CLAMP);
}

void ResultUI::DrawLabel(float x, float y, const std::string& str, unsigned int color, int font, int alpha)
{
	if (alpha <= 0) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringFToHandle(ToScreenX(x), ToScreenY(y), str.c_str(), color, font);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawLabelRight(float x, float y, const std::string& str, unsigned int color, int font, int alpha)
{
	if (alpha <= 0) return;
	const int w = GetDrawStringWidthToHandle(str.c_str(), static_cast<int>(str.size()), font);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringFToHandle(ToScreenX(x) - w, ToScreenY(y), str.c_str(), color, font);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

void ResultUI::DrawGrowLineH(float x1, float x2, float y, int start, int len, unsigned int color)
{
	const float t = Rate(start, len);
	if (t <= 0.0f) return;
	const float ease = 1.0f - (1.0f - t) * (1.0f - t);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawLineAA(ToScreenX(x1), ToScreenY(y), ToScreenX(x1 + (x2 - x1) * ease), ToScreenY(y), color, 1.5f);
}

void ResultUI::DrawGrowLineV(float x, float y1, float y2, int start, int len, unsigned int color)
{
	const float t = Rate(start, len);
	if (t <= 0.0f) return;
	const float ease = 1.0f - (1.0f - t) * (1.0f - t);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawLineAA(ToScreenX(x), ToScreenY(y1), ToScreenX(x), ToScreenY(y1 + (y2 - y1) * ease), color, 1.5f);
}

float ResultUI::Rate(int start, int len) const
{
	if (len <= 0) return m_frame >= start ? 1.0f : 0.0f;
	return std::clamp(static_cast<float>(m_frame - start) / len, 0.0f, 1.0f);
}

int ResultUI::FadeAlpha(int start) const
{
	return static_cast<int>(255 * Rate(start, kFadeFrame));
}

int ResultUI::CountUp(int value, int start, int len) const
{
	const float t = Rate(start, len);
	const float ease = 1.0f - (1.0f - t) * (1.0f - t);
	return static_cast<int>(std::round(value * ease));
}

float ResultUI::PopScale(int start) const
{
	//大きく出て少し縮んでから戻る
	const float t = Rate(start, kPopLen);
	if (t < 0.7f)
	{
		const float k = t / 0.7f;
		return 1.9f + (0.9f - 1.9f) * k;
	}
	const float k = (t - 0.7f) / 0.3f;
	return 0.9f + 0.1f * k;
}

std::string ResultUI::FormatNumber(int value)
{
	std::string digits = std::to_string(value < 0 ? -value : value);
	std::string result;
	const int len = static_cast<int>(digits.size());
	for (int i = 0; i < len; ++i)
	{
		result += digits[i];
		const int rest = len - 1 - i;
		if (rest > 0 && rest % 3 == 0) result += ',';
	}
	return value < 0 ? "-" + result : result;
}

std::string ResultUI::FormatTime(int frame)
{
	const int centi = frame * 100 / 60;
	char buf[32];
	snprintf(buf, sizeof(buf), "%d:%02d.%02d", centi / 6000, (centi / 100) % 60, centi % 100);
	return buf;
}

std::string ResultUI::FormatRate(float rate)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "x%.1f", rate);
	return buf;
}

float ResultUI::ToScreenX(float x) const
{
	return Game::ScaleX(x) + m_shakeX * Game::GetScale();
}

float ResultUI::ToScreenY(float y) const
{
	return Game::ScaleY(y) + m_shakeY * Game::GetScale();
}
