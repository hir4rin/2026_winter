#include "PartBrokenKillUI.h"
#include <cmath>
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"

namespace
{
	//画像のパス
	const char* const kBladePath = "data/UI/PartBrokenKill/blade.png";
	const char* const kButtonPath = "data/UI/button/Y.png";

	//全体の大きさ
	constexpr float kUIScale = 0.8f;

	//位置
	constexpr float kEnemyCenterHeight = 100.0f;//敵の足元から体の中心まで(3D)

	//刃(大きさは設計解像度)
	constexpr float kBladeImageWidth = 256.0f;
	constexpr float kBladeImageHeight = 40.0f;
	constexpr float kBladeLength = 116.0f;
	constexpr float kBladeThick = 18.0f;
	constexpr float kCoreLengthRate = 0.8f;//白い芯の長さ(刃に対して)
	constexpr float kCoreThickRate = 0.35f;//白い芯の太さ(刃に対して)
	constexpr float kBladeAngle = DX_PI_F * 0.25f;
	constexpr float kBladeOpenFrame = 7.0f;//伸びきるまで
	constexpr float kSecondBladeDelay = 4.0f;//2本目が走り始めるまで
	constexpr int kRedR = 200;
	constexpr int kRedG = 24;
	constexpr int kRedB = 34;
	constexpr int kCoreR = 255;
	constexpr int kCoreG = 236;
	constexpr int kCoreB = 238;

	//表示中の明滅
	constexpr float kPulseSpeed = 0.1f;
	constexpr float kPulseAlpha = 150.0f;

	//真ん中の丸
	constexpr float kCenterRadius = 22.0f;
	constexpr float kCenterLineWidth = 2.0f;
	constexpr int kCenterPosNum = 48;//円の頂点数
	constexpr float kButtonImageSize = 64.0f;
	constexpr float kButtonSize = 60.0f;
	constexpr float kPopScale = 1.3f;//出始めの大きさ
	constexpr float kPopFrame = 6.0f;

	//確殺が始まったときのリアクション
	constexpr float kExitFrame = 18.0f;//全体の長さ
	constexpr float kExitWhiteFrame = 4.0f;//刃が白くなるまで
	constexpr float kExitLengthScale = 1.4f;//刃をどこまで長くするか
	constexpr float kExitThinRate = 0.25f;//最後の刃の太さ
	constexpr float kExitPopScale = 1.2f;//真ん中の丸をどこまで大きくするか
	constexpr float kExitFlashFrame = 10.0f;//白い光が消えるまで
	constexpr float kExitFlashAlpha = 220.0f;
	constexpr float kExitFadeStartFrame = 6.0f;//全体が消え始める
}

PartBrokenKillUI::PartBrokenKillUI(std::weak_ptr<Player> player) :
	m_player(player)
{
}

PartBrokenKillUI::~PartBrokenKillUI()
{
	DeleteGraph(m_bladeHandle);
	DeleteGraph(m_buttonHandle);
}

void PartBrokenKillUI::Init()
{
	//縮小でふちがにじまないようにする
	SetUsePremulAlphaConvertLoad(TRUE);
	m_bladeHandle = LoadGraph(kBladePath);
	m_buttonHandle = LoadGraph(kButtonPath);
	SetUsePremulAlphaConvertLoad(FALSE);

	m_isVisible = false;
}

void PartBrokenKillUI::Update(Input& input)
{
	//確殺が始まったらリアクションに入る
	auto player = m_player.lock();
	if (m_isOnScreen && player && player->GetIsPartBrokenKilling()) m_isExiting = true;

	if (m_isExiting)
	{
		m_exitFrame++;
		if (m_exitFrame >= kExitFrame) Hide();
		return;
	}

	auto target = FindTarget();
	if (!target)
	{
		Hide();
		return;
	}

	//対象が変わったらアニメをやり直す
	if (target != m_target.lock()) m_frame = 0;
	m_target = target;
	m_frame++;
	m_isVisible = true;
}

void PartBrokenKillUI::Draw()
{
	if (!m_isVisible) return;

	//リアクション中はカメラが動くので位置を止める
	//if (!m_isExiting)
	//{
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
	//}

	const float scale = Game::GetScale() * kUIScale;

	SetDrawMode(DX_DRAWMODE_BILINEAR);
	//剣の刃
	DrawBlades(m_drawX, m_drawY, scale);
	//ボタンなどその他の表示
	DrawCenter(m_drawX, m_drawY, scale);

	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

std::shared_ptr<EnemyBase> PartBrokenKillUI::FindTarget() const
{
	auto player = m_player.lock();
	if (!player) return nullptr;
	//Yで確殺に入れないときは出さない
	if (!player->CanInputPartBrokenState()) return nullptr;
	//Yで暗殺が先に出るときは出さない
	if (player->CanInputAssasinState() && player->CanAssasin()) return nullptr;

	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle || battle->GetIsUltimating() || battle->GetIsEventPlaying()) return nullptr;

	return player->GetPartBrokenTarget();
}

void PartBrokenKillUI::Hide()
{
	m_target.reset();
	m_frame = 0;
	m_isExiting = false;
	m_exitFrame = 0;
	m_isOnScreen = false;
	m_isVisible = false;
}

float PartBrokenKillUI::GetBladeOpenRate(float delay) const
{
	//だんだん遅くなるように伸ばす
	float rate = (m_frame - delay) / kBladeOpenFrame;
	if (rate <= 0.0f) return 0.0f;
	if (rate > 1.0f) rate = 1.0f;
	return 1.0f - (1.0f - rate) * (1.0f - rate);
}

float PartBrokenKillUI::GetExitAlpha() const
{
	const float rate = (m_exitFrame - kExitFadeStartFrame) / (kExitFrame - kExitFadeStartFrame);
	if (rate <= 0.0f) return 1.0f;
	if (rate >= 1.0f) return 0.0f;
	return 1.0f - rate;
}

void PartBrokenKillUI::DrawBlades(float x, float y, float scale)
{
	//確殺が始まったら長く細くして、弾けるように消す
	const float exitRate = m_exitFrame / kExitFrame;
	const float length = kBladeLength * scale * (1.0f + (kExitLengthScale - 1.0f) * exitRate);
	const float thick = kBladeThick * scale * (1.0f - (1.0f - kExitThinRate) * exitRate);
	const float exitAlpha = GetExitAlpha();

	//確殺が始まったら赤から白へ変える
	float whiteRate = m_exitFrame / kExitWhiteFrame;
	if (whiteRate > 1.0f) whiteRate = 1.0f;
	const int r = static_cast<int>(kRedR + (255 - kRedR) * whiteRate);
	const int g = static_cast<int>(kRedG + (255 - kRedG) * whiteRate);
	const int b = static_cast<int>(kRedB + (255 - kRedB) * whiteRate);

	//赤い刃
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * exitAlpha));
	SetDrawBright(r, g, b);
	DrawBladePair(x, y, length, thick);

	//同じ刃を足して明滅させる
	const float pulse = (std::sin(m_frame * kPulseSpeed) + 1.0f) * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, static_cast<int>(kPulseAlpha * pulse * exitAlpha));
	DrawBladePair(x, y, length, thick);

	//白い芯
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * exitAlpha));
	SetDrawBright(kCoreR, kCoreG, kCoreB);
	DrawBladePair(x, y, length * kCoreLengthRate, thick * kCoreThickRate);

	SetDrawBright(255, 255, 255);
}

void PartBrokenKillUI::DrawBladePair(float x, float y, float length, float thick)
{
	//左上から右下、右上から左下の順に走らせる
	DrawBlade(x, y, kBladeAngle, GetBladeOpenRate(0.0f), length, thick);
	DrawBlade(x, y, DX_PI_F - kBladeAngle, GetBladeOpenRate(kSecondBladeDelay), length, thick);
}

void PartBrokenKillUI::DrawBlade(float x, float y, float angle, float openRate, float length, float thick)
{
	if (openRate <= 0.0f) return;

	//刃の根本を軸にして、長さの方向にだけ伸ばす
	const float startX = x - std::cos(angle) * length * 0.5f;
	const float startY = y - std::sin(angle) * length * 0.5f;
	const double rateX = length * openRate / kBladeImageWidth;
	const double rateY = thick / kBladeImageHeight;
	DrawRotaGraph3F(startX, startY, 0.0f, kBladeImageHeight * 0.5f, rateX, rateY, angle, m_bladeHandle, TRUE);
}

void PartBrokenKillUI::DrawCenter(float x, float y, float scale)
{
	//出始めだけ大きくする
	float pop = 1.0f;
	//増やす量の最大*残っている割合
	if (m_frame < kPopFrame) pop += (kPopScale - 1.0f) * (1.0f - m_frame / kPopFrame);

	//確殺が始まったらだんだん遅くなるように大きくする
	float exitRate = m_exitFrame / kExitFlashFrame;
	if (exitRate > 1.0f) exitRate = 1.0f;
	exitRate = 1.0f - (1.0f - exitRate) * (1.0f - exitRate);
	pop *= 1.0f + (kExitPopScale - 1.0f) * exitRate;

	const float exitAlpha = GetExitAlpha();
	const float radius = kCenterRadius * scale * pop;

	//黒い丸と赤いふち
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * exitAlpha));
	DrawCircleAA(x, y, radius, kCenterPosNum, GetColor(18, 8, 10), TRUE);
	DrawCircleAA(x, y, radius, kCenterPosNum, GetColor(kRedR, kRedG, kRedB), FALSE, kCenterLineWidth * scale);

	//Yボタン
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * exitAlpha));
	const double buttonRate = kButtonSize / kButtonImageSize * scale * pop;
	DrawRotaGraphF(x, y, buttonRate, 0.0, m_buttonHandle, TRUE);

	//確殺が始まったときの光
	if (!m_isExiting) return;
	const float flashRate = 1.0f - m_exitFrame / kExitFlashFrame;
	if (flashRate <= 0.0f) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kExitFlashAlpha * flashRate));
	DrawCircleAA(x, y, radius, kCenterPosNum, GetColor(255, 255, 255), TRUE);
}
