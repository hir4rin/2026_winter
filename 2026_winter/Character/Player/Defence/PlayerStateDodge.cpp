#include "PlayerStateDodge.h"
#include "Player.h"
#include "../../../Input.h"
#include "../Game.h"
#include "../../JustDodgeCol.h"
#include "../../../Collider/SphereShape.h"
#include "../../../Managers/CollisionManager.h"
#include "../../../System.h"

namespace
{
	constexpr float kPlayerCenter = 100.0f;//プレイヤーの当たり判定の中心点までのy軸の距離
	constexpr float kJustDodgeColRadius = 120.0f;//ジャスト回避判定の半径//やられ判定(50)より大きくして、かすった攻撃も拾う
	constexpr float kJustDodgeFrame = 10.0f;//ジャスト回避の受付フレーム数//回避開始からこのフレームまで受け付ける

	constexpr float kJustDodgeTimeScale = 0.2f;//ジャスト回避成功時の時間スケール
	constexpr int kJustDodgeSlowFrame = 30;//ジャスト回避成功時にスローにするフレーム数
}

PlayerStateDodge::PlayerStateDodge(std::weak_ptr<Player> player):PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateDodge::~PlayerStateDodge()
{
}

void PlayerStateDodge::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();
	//カメラから見たPlayerの正面方向を取る
	HandlerInput();
	//playerの向きを更新
	 int hasInput = 0;

	if (input.IsPressed("Up"))
	{
		player->m_targetVec = player->forward;
		hasInput++;
	}
	if (input.IsPressed("Down"))
	{
		player->m_targetVec = player->down;
		hasInput++;
	}
	if (input.IsPressed("Right"))
	{
		player->m_targetVec = player->right;
		hasInput++;
	}
	if (input.IsPressed("Left"))
	{
		player->m_targetVec = player->left;
		hasInput++;
	}


	//animationの初期化//ロックオン中もまた分ける
	if (hasInput == 0)
	{
		//入力がなかった場合
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("DodgeBackward"), true, 0.4f);
		m_avoidState = AvoidState::Backward;
	}
	else
	{
		//入力があった場合
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("DodgeForward"), true, 0.4f);
		m_avoidState = AvoidState::Forward;

	}

	//ジャスト回避判定を生成する//回避のたびに作り直すので、毎回新しいidになる
	m_justDodgeTimer = 0.0f;
	m_isJustDodged = false;
	m_justDodgeCol = std::make_shared<JustDodgeCol>(m_owner);
	m_justDodgeCol->ColInit({
		.pos = player->m_rb.m_pos,
		.offset = Vector3(0, kPlayerCenter, 0),
		.shape = std::make_unique<SphereShape>(kJustDodgeColRadius),
		.tag = {Collider::Faction::Player, Collider::ColRole::JustDodge},
		.isActive = true,
		.isTrigger = true
		});//押し戻しはしない
}

void PlayerStateDodge::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//ジャスト回避の受付時間の管理//受付時間が過ぎたら判定を無効にする
	m_justDodgeTimer += 1.0f * System::GetInstance().GetTimeScale();
	if (m_justDodgeCol && m_justDodgeTimer > kJustDodgeFrame)
	{
		m_justDodgeCol->SetIsActive(false);
	}

	if (m_avoidState == AvoidState::Backward)
	{
		player->m_rb.m_vel = player->m_targetVec.Normalize() * Game::kDodgeSpeed * -1;
	}
	else
	{
		player->m_rb.m_vel = player->m_targetVec.Normalize() * Game::kDodgeSpeed;
	}

	if (player->m_anim.GetAnimRate() > 0.3f)
	{
		if (input.IsLeftStickInput())
		{
			//入力があればWalk状態に遷移する
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
			return;
		}
	}

	player->m_anim.Update();
}

void PlayerStateDodge::Exit()
{
	//ジャスト回避判定を削除する//被弾などで途中で状態が変わっても必ず消す
	ReleaseJustDodgeCol();
}

void PlayerStateDodge::DebugDraw()
{
#ifdef _DEBUG
	if (m_isJustDodged)DrawFormatString(0, 16, GetColor(255, 0, 255), "JustDodge!!");
#endif
}

void PlayerStateDodge::OnJustDodge(Collider& other, const CharacterBase::AttackData& data)
{
	//受付時間外、または既に成功していたら何もしない//1回の回避で1回だけ
	if (m_justDodgeTimer > kJustDodgeFrame)return;
	if (m_isJustDodged)return;
	m_isJustDodged = true;

	//スロー演出//反撃への移行などはここに追加する
	System::GetInstance().SetTimeScaleForFrames(kJustDodgeTimeScale, kJustDodgeSlowFrame);
}

bool PlayerStateDodge::IsInvincible() const
{
	//ジャスト回避の受付中と、ジャスト回避成功後は回避が終わるまで無敵
	return m_justDodgeTimer <= kJustDodgeFrame || m_isJustDodged;
}

void PlayerStateDodge::ReleaseJustDodgeCol()
{
	if (!m_justDodgeCol)return;
	CollisionManager::GetInstance().ReleaseCollider(m_justDodgeCol);//当たり判定を削除する
	m_justDodgeCol->SetIsActive(false);
	m_justDodgeCol->SetLifeTimeLimited();
	m_justDodgeCol.reset();
}
