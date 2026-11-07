#include "AssasinTargetUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"

namespace
{
	//画像のパス
	const char* const kIconPath = "data/UI/Assasin/icon.png";
	const char* const kButtonPath = "data/UI/button/Y.png";
	const char* const kBandPath = "data/UI/Menu/select_base.png";

	//文字
	const char* const kLabel = "暗殺";
	const char* const kFontName = "游明朝";
	constexpr int kFontSize = 26;
	constexpr int kFontThick = 6;
	constexpr int kFontSpace = 8;//文字の間隔

	//全体の大きさ
	constexpr float kUIScale = 0.8f;

	//位置(大きさは設計解像度)
	constexpr float kEnemyHeight = 200.0f;//敵の足元からの高さ(3D)
	constexpr float kIconLift = 22.0f;//画面で上へずらす量

	//丸いアイコン
	constexpr float kIconImageSize = 128.0f;//画像の大きさ
	constexpr float kIconCenterX = 64.0f;//画像の中の丸の中心
	constexpr float kIconCenterY = 52.0f;
	constexpr float kIconSize = 88.0f;//画面での画像の大きさ
	constexpr float kButtonImageSize = 64.0f;
	constexpr float kButtonSize = 66.0f;
	constexpr float kPopScale = 1.3f;//出始めの大きさ
	constexpr float kPopFrame = 6.0f;

	//赤い帯
	constexpr float kBandWidth = 190.0f;
	constexpr float kBandHeight = 40.0f;
	constexpr float kBandLineWidth = 2.0f;//右端の線
	constexpr float kBandOpenFrame = 10.0f;
	constexpr int kBandAlpha = 255;
	constexpr int kRedR = 200;
	constexpr int kRedG = 24;
	constexpr int kRedB = 34;

	//文字のフェード
	constexpr float kLabelX = 48.0f;//丸の中心から文字の左端まで
	constexpr float kLabelStartFrame = 6.0f;
	constexpr float kLabelFadeFrame = 8.0f;

	//暗殺が始まったときのリアクション
	constexpr float kExitFrame = 18.0f;//全体の長さ
	constexpr float kExitPopScale = 1.25f;//アイコンをどこまで大きくするか
	constexpr float kExitPopFrame = 5.0f;
	constexpr float kExitFlashFrame = 10.0f;//白い光が消えるまで
	constexpr float kExitFlashAlpha = 220.0f;
	constexpr float kExitLabelFadeFrame = 6.0f;//文字が消えるまで
	constexpr float kExitFadeStartFrame = 8.0f;//全体が消え始める
	constexpr float kIconDiscRadius = 46.0f;//画像の中の丸の半径
	constexpr int kFlashPosNum = 48;//光の円の頂点数
}

AssasinTargetUI::AssasinTargetUI(std::weak_ptr<Player> player) :
	m_player(player)
{
}

AssasinTargetUI::~AssasinTargetUI()
{
	DeleteGraph(m_iconHandle);
	DeleteGraph(m_buttonHandle);
	DeleteGraph(m_bandHandle);
	DeleteFontToHandle(m_fontHandle);
}

void AssasinTargetUI::Init()
{
	
	//縮小でふちがにじまないようにする//普通のpngは透明な部分にも見えない色が入っているので、にじまないようにしている
	SetUsePremulAlphaConvertLoad(TRUE);
	m_iconHandle = LoadGraph(kIconPath);
	m_buttonHandle = LoadGraph(kButtonPath);
	SetUsePremulAlphaConvertLoad(FALSE);
	m_bandHandle = LoadGraph(kBandPath);

	const float scale = Game::GetScale() * kUIScale;
	m_fontHandle = CreateFontToHandle(kFontName, static_cast<int>(kFontSize * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_4X4);
	SetFontSpaceToHandle(static_cast<int>(kFontSpace * scale), m_fontHandle);

	m_isVisible = false;
}

void AssasinTargetUI::Update()
{
	//暗殺が始まったらリアクションに入る
	auto player = m_player.lock();
	if (m_isOnScreen && player && player->GetIsAssasinating()) m_isExiting = true;
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

void AssasinTargetUI::Draw()
{
	if (!m_isVisible) return;

	const float scale = Game::GetScale() * kUIScale;

	//リアクション中はカメラが動くので位置を止める
	if (!m_isExiting)
	{
		auto target = m_target.lock();
		if (!target) return;

		//敵の座標から頭の上の位置を求めて画面の座標にする
		const Vector3 uiPos = target->GetRigidBody().GetPos() + Vector3(0.0f, kEnemyHeight, 0.0f);
		const VECTOR screenPos = ConvWorldPosToScreenPos(uiPos.ToDxLibVector());
		//カメラの後ろなら出さない
		m_isOnScreen = screenPos.z > 0.0f && screenPos.z < 1.0f;
		if (!m_isOnScreen) return;

		m_drawX = screenPos.x;
		m_drawY = screenPos.y - kIconLift * scale;
	}

	//周りの4ピクセルを、近さに応じて混ぜる
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	//赤い帯を書く
	DrawBand(m_drawX, m_drawY, scale);
	//アイコンの表示->ボタンの表示
	DrawIcon(m_drawX, m_drawY, scale);
	//暗殺の文字を表示
	DrawLabel(m_drawX, m_drawY, scale);
	//暗殺が始まったときの光
	DrawFlash(m_drawX, m_drawY, scale);

	//設定を元に戻す
	//一番近いピクセルの色をそのまま使う
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

std::shared_ptr<EnemyBase> AssasinTargetUI::FindTarget() const
{
	auto player = m_player.lock();
	if (!player) return nullptr;
	//攻撃中などYで暗殺に入れないときは出さない
	if (!player->CanInputAssasinState()) return nullptr;

	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle || battle->GetIsUltimating() || battle->GetIsEventPlaying()) return nullptr;

	return player->GetAssasinTarget();
}

void AssasinTargetUI::Hide()
{
	m_target.reset();
	m_frame = 0;
	m_isExiting = false;
	m_exitFrame = 0;
	m_isOnScreen = false;
	m_isVisible = false;
}

float AssasinTargetUI::GetIconPop() const
{
	//出始めだけ大きくする
	float pop = 1.0f;
	if (m_frame < kPopFrame) pop += (kPopScale - 1.0f) * (1.0f - m_frame / kPopFrame);

	//暗殺が始まったらだんだん遅くなるように大きくする
	float rate = m_exitFrame / kExitPopFrame;
	if (rate > 1.0f) rate = 1.0f;
	rate = 1.0f - (1.0f - rate) * (1.0f - rate);
	return pop * (1.0f + (kExitPopScale - 1.0f) * rate);
}

float AssasinTargetUI::GetExitAlpha() const
{
	const float rate = (m_exitFrame - kExitFadeStartFrame) / (kExitFrame - kExitFadeStartFrame);
	if (rate <= 0.0f) return 1.0f;
	if (rate >= 1.0f) return 0.0f;
	return 1.0f - rate;
}

void AssasinTargetUI::DrawBand(float x, float y, float scale)
{
	//だんだん遅くなるように伸ばす
	float rate = m_frame / kBandOpenFrame;
	if (rate > 1.0f) rate = 1.0f;
	rate = 1.0f - (1.0f - rate) * (1.0f - rate);

	//暗殺が始まったらだんだん速くなるように縮める
	const float closeRate = m_exitFrame / kExitFrame;
	rate *= 1.0f - closeRate * closeRate;

	const float left = x;
	const float right = x + kBandWidth * scale * rate;
	const float top = y - kBandHeight * scale * 0.5f;
	const float bottom = y + kBandHeight * scale * 0.5f;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kBandAlpha * GetExitAlpha()));
	SetDrawBright(kRedR, kRedG, kRedB);
	//画像は右が濃いので左右を反転して貼る
	DrawModiGraphF(right, top, left, top, left, bottom, right, bottom, m_bandHandle, TRUE);
	SetDrawBright(255, 255, 255);

	//右端の線
	DrawBoxAA(right - kBandLineWidth * scale, top, right, bottom, GetColor(kRedR, kRedG, kRedB), TRUE);
}

void AssasinTargetUI::DrawIcon(float x, float y, float scale)
{
	const float pop = GetIconPop();

	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * GetExitAlpha()));
	const double iconRate = kIconSize / kIconImageSize * scale * pop;
	DrawRotaGraph2F(x, y, kIconCenterX, kIconCenterY, iconRate, 0.0, m_iconHandle, TRUE);
	const double buttonRate = kButtonSize / kButtonImageSize * scale * pop;
	DrawRotaGraphF(x, y, buttonRate, 0.0, m_buttonHandle, TRUE);
}

void AssasinTargetUI::DrawLabel(float x, float y, float scale)
{
	float alpha = (m_frame - kLabelStartFrame) / kLabelFadeFrame;
	if (alpha <= 0.0f) return;
	if (alpha > 1.0f) alpha = 1.0f;

	//暗殺が始まったら帯より先に消す
	alpha *= 1.0f - m_exitFrame / kExitLabelFadeFrame;
	if (alpha <= 0.0f) return;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	const float textY = y - GetFontSizeToHandle(m_fontHandle) * 0.5f;
	DrawStringFToHandle(x + kLabelX * scale, textY, kLabel, GetColor(255, 236, 238), m_fontHandle);
}

void AssasinTargetUI::DrawFlash(float x, float y, float scale)
{
	if (!m_isExiting) return;

	const float rate = 1.0f - m_exitFrame / kExitFlashFrame;
	if (rate <= 0.0f) return;

	//丸に白い円を重ねて、だんだん薄くする
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kExitFlashAlpha * rate));
	const float radius = kIconDiscRadius * kIconSize / kIconImageSize * scale * GetIconPop();
	DrawCircleAA(x, y, radius, kFlashPosNum, GetColor(255, 255, 255), TRUE);
}
