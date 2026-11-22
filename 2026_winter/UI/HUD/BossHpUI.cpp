#include "BossHpUI.h"
#include <algorithm>
#include <cmath>
#include "DxLib.h"
#include "UI/UIFont.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Character/Enemy/EnemyManager.h"
#include "Character/Enemy/EnemyBase.h"

namespace
{
	//バー(設計解像度)
	constexpr float kBarX = 256.0f;
	constexpr float kBarY = 652.0f;
	constexpr float kBarWidth = 768.0f;
	constexpr float kBarHeight = 10.0f;
	constexpr float kBarMargin = 3.0f;//外枠と中身の間

	//名前
	constexpr float kNameX = 258.0f;
	constexpr float kNameY = 622.0f;
	constexpr int kFontSize = 22;
	constexpr int kFontThick = 3;
	constexpr int kFontSpace = 4;

	//登場
	constexpr int kIntroNameFrame = 30;//名前が出るまで
	constexpr int kIntroBarStart = 10;//バーが伸び始める
	constexpr int kIntroBarFrame = 45;//バーが伸びきるまで
	constexpr int kIntroFrame = 60;

	//白い所(遅れて減る)
	constexpr int kWhiteWait = 30;//最後に減ってから待つ
	constexpr float kWhiteFollow = 0.08f;//近づく速さ
	constexpr float kWhiteMinStep = 0.002f;

	//残り少ない
	constexpr float kLowRate = 0.25f;
	constexpr float kBlinkSpeed = 0.25f;

	//撃破
	constexpr int kBreakFrame = 40;
	constexpr int kSlashGrowFrame = 4;//斬撃の線が伸びる
	constexpr int kSlashEndFrame = 16;//線が消える
	constexpr int kSplitStart = 5;//割れ始める
	constexpr float kSplitDistance = 16.0f;//上下に離れる量
	constexpr float kSlashTilt = 8.0f;//線の傾き(左右の高さの差の半分)
	constexpr float kSlashOver = 40.0f;//バーの外へはみ出す長さ
	constexpr float kSlashThick = 3.0f;

	//色
	constexpr unsigned int kBackColor = 0x121010;
	constexpr unsigned int kFrameColor = 0xc9b98a;
	constexpr unsigned int kWhiteColor = 0xe8e2d0;
	constexpr unsigned int kRedColor = 0xa8231c;
	constexpr unsigned int kLowColor = 0xe04a3a;
	constexpr unsigned int kNameColor = 0xe8e2d0;
	constexpr unsigned int kNameEdgeColor = 0x0a0808;
	constexpr int kBackAlpha = 200;
}

BossHpUI::BossHpUI(std::weak_ptr<EnemyManager> enemyManager) :
	m_enemyManager(enemyManager)
{
}

BossHpUI::~BossHpUI()
{
	DeleteFontToHandle(m_fontHandle);
}

void BossHpUI::Init()
{
	const float scale = Game::GetScale();
	m_fontHandle = UIFont::Create(static_cast<int>(kFontSize * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_EDGE_4X4, 2);
	SetFontSpaceToHandle(static_cast<int>(kFontSpace * scale), m_fontHandle);
	m_isVisible = true;
}

std::shared_ptr<EnemyBase> BossHpUI::FindBoss() const
{
	auto enemyMgr = m_enemyManager.lock();
	if (!enemyMgr) return nullptr;
	for (const auto& enemy : enemyMgr->GetEnemies())
	{
		if (!enemy || !enemy->IsBoss()) continue;
		if (enemy->GetIsDead() || enemy->GetHp() <= 0) continue;
		return enemy;
	}
	return nullptr;
}

void BossHpUI::ChangeState(State state)
{
	m_state = state;
	m_frame = 0;
}

bool BossHpUI::IsHideScene() const
{
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle) return true;
	return battle->GetIsEventPlaying() || battle->GetFinisherScreen().isPlaying || battle->GetEndingScreen().isPlaying;
}

void BossHpUI::Update()
{
	m_blinkFrame++;

	//ボスを見つけたら出す
	if (m_state == State::Hidden)
	{
		auto boss = FindBoss();
		if (!boss || IsHideScene()) return;
		m_boss = boss;
		m_hpRate = 1.0f;
		m_whiteRate = 1.0f;
		m_whiteWait = 0;
		ChangeState(State::Intro);
		return;
	}

	if (m_state == State::Break)
	{
		m_frame++;
		if (m_frame >= kBreakFrame) ChangeState(State::Hidden);
		return;
	}

	auto boss = m_boss.lock();
	if (!boss)
	{
		ChangeState(State::Hidden);
		return;
	}

	//HP
	const int maxHp = static_cast<int>(boss->GetStatus().hp);
	const float rate = (maxHp > 0) ? std::clamp(static_cast<float>(boss->GetHp()) / maxHp, 0.0f, 1.0f) : 0.0f;
	if (rate < m_hpRate) m_whiteWait = kWhiteWait;
	m_hpRate = rate;
	if (m_whiteRate < m_hpRate) m_whiteRate = m_hpRate;

	//白い所は待ってから追いつく
	if (m_whiteWait > 0)
	{
		m_whiteWait--;
	}
	else if (m_whiteRate > m_hpRate)
	{
		const float step = (std::max)((m_whiteRate - m_hpRate) * kWhiteFollow, kWhiteMinStep);
		m_whiteRate = (std::max)(m_whiteRate - step, m_hpRate);
	}

	//0になったら斬って消す
	if (m_hpRate <= 0.0f || boss->GetIsDead())
	{
		ChangeState(State::Break);
		return;
	}

	if (m_state == State::Intro)
	{
		//イベント中は進めない
		if (IsHideScene()) return;
		m_frame++;
		if (m_frame >= kIntroFrame) ChangeState(State::Active);
	}
}

void BossHpUI::Draw()
{
	if (!m_isVisible || m_state == State::Hidden) return;

	if (m_state == State::Break)
	{
		DrawBreak();
		return;
	}
	if (IsHideScene()) return;

	int nameAlpha = 255;
	if (m_state == State::Intro)
	{
		nameAlpha = (std::min)(255, m_frame * 255 / kIntroNameFrame);
	}
	DrawName(nameAlpha);
	DrawBar(0.0f, 255);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void BossHpUI::DrawName(int alpha) const
{
	auto boss = m_boss.lock();
	if (!boss || alpha <= 0) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringFToHandle(Game::ScaleX(kNameX), Game::ScaleY(kNameY), boss->GetBossName(), kNameColor, m_fontHandle, kNameEdgeColor);
}

void BossHpUI::DrawBar(float dy, int alpha) const
{
	if (alpha <= 0) return;

	//登場中は左から伸ばす
	float grow = 1.0f;
	if (m_state == State::Intro)
	{
		const float t = std::clamp(static_cast<float>(m_frame - kIntroBarStart) / kIntroBarFrame, 0.0f, 1.0f);
		grow = 1.0f - (1.0f - t) * (1.0f - t);
	}
	if (grow <= 0.0f) return;

	const float left = kBarX;
	const float top = kBarY + dy;
	const float bottom = top + kBarHeight;
	const float right = left + kBarWidth * grow;

	//下地
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, kBackAlpha * alpha / 255);
	DrawBoxAA(Game::ScaleX(left - kBarMargin), Game::ScaleY(top - kBarMargin),
		Game::ScaleX(right + kBarMargin), Game::ScaleY(bottom + kBarMargin), kBackColor, TRUE);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	//外枠
	DrawBoxAA(Game::ScaleX(left - kBarMargin), Game::ScaleY(top - kBarMargin),
		Game::ScaleX(right + kBarMargin), Game::ScaleY(bottom + kBarMargin), kFrameColor, FALSE, Game::GetScale());

	//遅れて減る所
	const float whiteRight = left + kBarWidth * (std::min)(m_whiteRate, grow);
	if (whiteRight > left)
	{
		DrawBoxAA(Game::ScaleX(left), Game::ScaleY(top), Game::ScaleX(whiteRight), Game::ScaleY(bottom), kWhiteColor, TRUE);
	}

	//残り(少ないときは明るい赤で点滅)
	const float redRight = left + kBarWidth * (std::min)(m_hpRate, grow);
	if (redRight > left)
	{
		unsigned int color = kRedColor;
		if (m_hpRate <= kLowRate)
		{
			const float blink = (std::sin(m_blinkFrame * kBlinkSpeed) + 1.0f) * 0.5f;
			color = (blink > 0.5f) ? kLowColor : kRedColor;
		}
		DrawBoxAA(Game::ScaleX(left), Game::ScaleY(top), Game::ScaleX(redRight), Game::ScaleY(bottom), color, TRUE);
	}
}

void BossHpUI::DrawBreak() const
{
	//全体のフェード
	const float fade = 1.0f - std::clamp(static_cast<float>(m_frame - kSplitStart) / (kBreakFrame - kSplitStart), 0.0f, 1.0f);
	const int alpha = static_cast<int>(255 * fade);

	DrawName(alpha);

	//線で上下に割る
	const float splitT = std::clamp(static_cast<float>(m_frame - kSplitStart) / (kBreakFrame - kSplitStart), 0.0f, 1.0f);
	const float split = kSplitDistance * (1.0f - (1.0f - splitT) * (1.0f - splitT));
	const int screenW = Game::GetScreenWidth();
	const int screenH = Game::GetScreenHeight();
	const int centerY = static_cast<int>(Game::ScaleY(kBarY + kBarHeight * 0.5f));

	SetDrawArea(0, 0, screenW, centerY);
	DrawBar(-split, alpha);
	SetDrawArea(0, centerY, screenW, screenH);
	DrawBar(split, alpha);
	SetDrawAreaFull();

	//斬撃の線
	if (m_frame < kSlashEndFrame)
	{
		const float grow = (std::min)(1.0f, static_cast<float>(m_frame + 1) / kSlashGrowFrame);
		const float thin = 1.0f - std::clamp(static_cast<float>(m_frame - kSlashGrowFrame) / (kSlashEndFrame - kSlashGrowFrame), 0.0f, 1.0f);
		const float cy = kBarY + kBarHeight * 0.5f;
		const float x1 = kBarX - kSlashOver;
		const float y1 = cy + kSlashTilt;
		const float x2 = x1 + (kBarWidth + kSlashOver * 2.0f) * grow;
		const float y2 = y1 - kSlashTilt * 2.0f * grow;
		const float half = kSlashThick * thin;

		SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
		DrawTriangleAA(Game::ScaleX(x1), Game::ScaleY(y1),
			Game::ScaleX(x2), Game::ScaleY(y2 - half), Game::ScaleX(x2), Game::ScaleY(y2 + half), 0xffffff, TRUE);
		DrawTriangleAA(Game::ScaleX(x2), Game::ScaleY(y2),
			Game::ScaleX(x1), Game::ScaleY(y1 - half), Game::ScaleX(x1), Game::ScaleY(y1 + half), 0xffffff, TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
