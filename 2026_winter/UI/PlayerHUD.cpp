#include "PlayerHUD.h"
#include <cmath>
#include "DxLib.h"
#include "Main/Game.h"
#include "Character/Player/Player.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"

namespace
{
	const char* const kFramePath = "data/UI/HUD/PlayerHP.png";
	const char* const kHpPath = "data/UI/HUD/gauge_hp.png";
	const char* const kSkillPath = "data/UI/HUD/gauge_Skill.png";
	const char* const kEmptyPath = "data/UI/HUD/gauge_brack.png";
	const char* const kUltBasePath = "data/UI/HUD/UltGaugeBase.png";
	const char* const kUltFramePath = "data/UI/HUD/UltGauge.png";
	const char* const kUltColorPath = "data/UI/HUD/UltGaugeColor.png";
	const char* const kUltMaxPath = "data/UI/HUD/UltGauge_circle.png";

	//PlayerHP.png の中のバーの範囲(画像のピクセル)
	constexpr float kInnerLeft = 95.0f;
	constexpr float kInnerRight = 652.0f;
	constexpr float kInnerTop = 89.0f;
	constexpr float kInnerBottom = 119.0f;

	//HPバー(外枠の左上・倍率)
	constexpr float kHpX = 140.0f;
	constexpr float kHpY = 10.0f;
	constexpr float kHpScale = 0.6f;
	//スキルバー
	constexpr float kSkillX = 140.0f;
	constexpr float kSkillY = 62.0f;
	constexpr float kSkillScale = 0.45f;

	//必殺技ゲージ
	constexpr float kUltX = 90.0f;
	constexpr float kUltY = 90.0f;
	constexpr float kUltRingScale = 150.0f / 910.0f;//輪の直径150
	constexpr float kUltBaseScale = 115.0f / 453.0f;//紋章の直径115
	constexpr float kUltMaxScale = 160.0f / 436.0f;//満タンの光の直径160
	constexpr int kUltEmptyBright = 25;//たまっていない所の明るさ

	constexpr float kFollowRate = 0.15f;//表示が今の値に近づく速さ
	constexpr float kBlinkSpeed = 0.1f;//満タンの点滅の速さ
}

PlayerHUD::PlayerHUD(std::weak_ptr<Player> player) :
	m_player(player)
{
}

PlayerHUD::~PlayerHUD()
{
	DeleteGraph(m_frameHandle);
	DeleteGraph(m_hpHandle);
	DeleteGraph(m_skillHandle);
	DeleteGraph(m_emptyHandle);
	DeleteGraph(m_ultBaseHandle);
	DeleteGraph(m_ultFrameHandle);
	DeleteGraph(m_ultColorHandle);
	DeleteGraph(m_ultMaxHandle);
}

void PlayerHUD::Init()
{
	//縮小したときに透明な所の白がにじまないよう、乗算済みアルファで読む
	SetUsePremulAlphaConvertLoad(TRUE);
	m_frameHandle = LoadGraph(kFramePath);
	m_hpHandle = LoadGraph(kHpPath);
	m_skillHandle = LoadGraph(kSkillPath);
	m_emptyHandle = LoadGraph(kEmptyPath);
	m_ultBaseHandle = LoadGraph(kUltBasePath);
	m_ultFrameHandle = LoadGraph(kUltFramePath);
	m_ultColorHandle = LoadGraph(kUltColorPath);
	m_ultMaxHandle = LoadGraph(kUltMaxPath);
	SetUsePremulAlphaConvertLoad(FALSE);
	m_isVisible = true;

	//最初は今の値から始める
	auto player = m_player.lock();
	if (!player) return;
	m_dispHpRate = static_cast<float>(player->GetHp()) / player->GetMaxHp();
	m_dispSkillRate = static_cast<float>(player->GetSkillGauge()) / player->GetMaxGauge();
	m_dispUltRate = static_cast<float>(player->GetUltGauge()) / player->GetMaxGauge();
}

void PlayerHUD::Update()
{
	auto player = m_player.lock();
	m_isVisible = (player != nullptr) && !System::GetInstance().GetBattleMgr()->GetFinisherScreen().isPlaying;
	if (!player) return;

	const float hpRate = static_cast<float>(player->GetHp()) / player->GetMaxHp();
	const float skillRate = static_cast<float>(player->GetSkillGauge()) / player->GetMaxGauge();
	const float ultRate = static_cast<float>(player->GetUltGauge()) / player->GetMaxGauge();

	m_dispHpRate = Follow(m_dispHpRate, hpRate);
	m_dispSkillRate = Follow(m_dispSkillRate, skillRate);
	m_dispUltRate = Follow(m_dispUltRate, ultRate);

	m_frame++;
}

float PlayerHUD::Follow(float disp, float target) const
{
	disp += (target - disp) * kFollowRate;
	//近づいたらぴったりにする
	if (std::abs(target - disp) < 0.001f) disp = target;
	if (disp < 0.0f) disp = 0.0f;
	if (disp > 1.0f) disp = 1.0f;
	return disp;
}

void PlayerHUD::Draw()
{
	if (!m_isVisible) return;

	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
	DrawUlt();
	DrawBar(kHpX, kHpY, kHpScale, m_dispHpRate, m_hpHandle);
	DrawBar(kSkillX, kSkillY, kSkillScale, m_dispSkillRate, m_skillHandle);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void PlayerHUD::DrawBar(float x, float y, float scale, float rate, int fillHandle)
{
	int w = 0;
	int h = 0;
	GetGraphSize(m_frameHandle, &w, &h);

	//外枠
	DrawExtendGraphF(Game::ScaleX(x), Game::ScaleY(y),
		Game::ScaleX(x + w * scale), Game::ScaleY(y + h * scale), m_frameHandle, TRUE);

	const float left = x + kInnerLeft * scale;
	const float right = x + kInnerRight * scale;
	const float top = y + kInnerTop * scale;
	const float bottom = y + kInnerBottom * scale;

	//減った所
	DrawExtendGraphF(Game::ScaleX(left), Game::ScaleY(top),
		Game::ScaleX(right), Game::ScaleY(bottom), m_emptyHandle, TRUE);

	//残り
	if (rate <= 0.0f) return;
	const float fillRight = left + (right - left) * rate;
	DrawExtendGraphF(Game::ScaleX(left), Game::ScaleY(top),
		Game::ScaleX(fillRight), Game::ScaleY(bottom), fillHandle, TRUE);
}

void PlayerHUD::DrawUlt()
{
	const float cx = Game::ScaleX(kUltX);
	const float cy = Game::ScaleY(kUltY);
	const float s = Game::GetScale();

	//真ん中の紋章(外側に白いぼかしがあるので輪より先に描く)
	DrawRotaGraphF(cx, cy, kUltBaseScale * s, 0.0, m_ultBaseHandle, TRUE);

	//たまっていない所は暗く
	SetDrawBright(kUltEmptyBright, kUltEmptyBright, kUltEmptyBright);
	DrawRotaGraphF(cx, cy, kUltRingScale * s, 0.0, m_ultColorHandle, TRUE);
	SetDrawBright(255, 255, 255);

	//たまった所(上から時計回り)
	//そのままだと反時計回りなので左右反転する
	DrawCircleGaugeF(cx, cy, m_dispUltRate * 100.0, m_ultColorHandle, 0.0, kUltRingScale * s, TRUE);

	//黒い輪(縄の所は抜けているので下の色が見える)
	DrawRotaGraphF(cx, cy, kUltRingScale * s, 0.0, m_ultFrameHandle, TRUE);

	//満タンなら光らせる
	auto player = m_player.lock();
	if (!player || player->GetUltGauge() < player->GetMaxGauge()) return;
	const float blink = (std::sin(m_frame * kBlinkSpeed) + 1.0f) * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, static_cast<int>(100 + 155 * blink));
	DrawRotaGraphF(cx, cy, kUltMaxScale * s, 0.0, m_ultMaxHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
}
