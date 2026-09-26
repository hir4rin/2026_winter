#include "PlayerStateUlt.h"
#include "../../../Input.h"
#include "../../AttackCol.h"
#include "Player.h"
#include "../../../Collider/SphereShape.h"
#include "EffekseerForDXLib.h"
#include "../../../System.h"

namespace
{
	constexpr float kPlayerCenter = 100.0f;//プレイヤーの当たり判定の中心点までのy軸の距離

	constexpr float kEffectTriggerTime = 0.5f;

	constexpr float kAttackDamage = 500.0f;

	constexpr float kRadius = 500.0f;
	constexpr float kAttackColOffset = 30.0f;
	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間


	constexpr float kColStartRate = 0.7f;
	constexpr float kColEndRate = 0.9f;
}
	

PlayerStateUlt::PlayerStateUlt(std::weak_ptr<Player> player):
	PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする//trueで破棄されている
	if (m_owner.expired())return;
}

PlayerStateUlt::~PlayerStateUlt()
{
}

void PlayerStateUlt::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//攻撃の方向を決める
	DetermineAttackDirection();	
	//アニメーションを流す
	player->m_anim.ChangeAnimWithModelHandle(player->m_attackModelHandle, player->GetAnimName("UltAttack"), false, 1.0f);
	//System::GetInstance().GetSoundManager().PlaySE("UltAttackSE");

	//攻撃の当たり判定を生成する
	//データ作成//後ほどデータ化する
	player->m_attackData = {
	.attackPower = kAttackDamage,
	.knockBackPower = Vector3(0, 0,0),
	//.knockBackPower = Vector3(0.0f,node.knockBackY,0.0f),//吹き飛ばない攻撃にする
	.knockBackFrame = 0,
	.hitStopTime = kHitStopTime,
	.kAttackColOffset = kAttackColOffset,
	.isKirimomi = false
	};
	//生成
	m_attackCol = std::make_shared<AttackCol>(m_owner, player->m_attackData);
	Vector3 offset = player->m_targetVec * player->m_attackData.kAttackColOffset
		+ Vector3(0, kPlayerCenter, 0);//プレイヤーの前方に50.0f、y軸方向にkPlayerCenterだけオフセットする
	m_attackCol->ColInit({
		.pos = player->m_rb.m_pos,
		.offset = offset,
		.shape = std::make_unique<SphereShape>(kRadius),
		.tag = {Collider::Faction::Player, Collider::ColRole::UltAttack},
		.isActive = false,
		.isTrigger = true
		});//攻撃の当たり判定を初期化する//最初は無効にしておく
}

void PlayerStateUlt::Update()
{
	//攻撃の当たり判定を有効にする
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();


	float rate = player->m_anim.GetAnimRate();
	if(rate > kColStartRate && rate < kColEndRate)
	{
		m_attackCol->SetIsActive(true);
	}
	else
	{
		m_attackCol->SetIsActive(false);
	}
	//エフェクトを発生、位置、回転の再設定
	EffectCheck();

	//アニメーションが終わったらアイドルに戻す
	//ウルトはおそらくすぐに他の物からキャンセルして出せるようにする（LB&Y）のボタン押し
	if(player->m_anim.GetAnimEndFlag())
	{

		if(input.IsLeftStickInput())
		{
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
			return;
		}
	}


	//アニメーションの更新
	player->m_anim.Update();

}

void PlayerStateUlt::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;
	//攻撃の当たり判定を削除する//
	if (m_attackCol)
	{
		//CollisionManager::GetInstance().ReleaseCollider(m_attackCol);//当たり判定を削除する
		m_attackCol->SetIsActive(false);
		m_attackCol->SetLifeTimeLimited();
		m_attackCol.reset();
	}
#ifdef _DEBUG
	player->AddUltGauge(100);
#endif
}

void PlayerStateUlt::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Ult");
}

void PlayerStateUlt::DetermineAttackDirection()
{
	auto player = m_owner.lock();
	if (!player) return;

	auto& input = Input::GetInstance();
	Vector3 attackDir = Vector3(0, 0, 0);

	//攻撃の方向を決める//カメラの向きと入力から、回避の方向を決める
	if (input.IsPressed("Up"))
	{
		attackDir += player->forward;
	}
	if (input.IsPressed("Down"))
	{
		attackDir += player->down;
	}
	if (input.IsPressed("Left"))
	{
		attackDir += player->left;
	}
	if (input.IsPressed("Right"))
	{
		attackDir += player->right;
	}
	//入力がないときは、playerの向いている方向に進む//あるときはその方向に進む//この処理に問題があるらしい
	if (attackDir.Magnitude() <= 0.0f)
	{
		player->m_targetVec = player->m_targetVec.Normalize();
		//player->m_targetVec = player->forward.Normalize();
	}
	else
	{
		player->m_targetVec = attackDir.Normalize();
	}

}
void PlayerStateUlt::EffectCheck()
{
	auto player = m_owner.lock();
	if (!player)return;

	float rate = player->m_anim.GetAnimRate();//アニメーションの進行率を取得
	if (rate >= kEffectTriggerTime && !m_isTriggerdEffec)
	{
		player->m_efPlayingHandle = PlayEffekseer3DEffect(player->m_efHandle);
		SetPosPlayingEffekseer3DEffect(player->m_efPlayingHandle, player->m_rb.m_pos.x, player->m_rb.m_pos.y + kPlayerCenter, player->m_rb.m_pos.z);
		SetRotationPlayingEffekseer3DEffect(player->m_efPlayingHandle, 0.0f, player->m_rotAngleY + DX_PI_F, 0.0f);
	}
	//エフェクトが出ているとき
	else if (m_isTriggerdEffec)
	{
		//座標の更新
		SetPosPlayingEffekseer3DEffect(player->m_efPlayingHandle, player->m_rb.m_pos.x, player->m_rb.m_pos.y + kPlayerCenter, player->m_rb.m_pos.z);
		SetRotationPlayingEffekseer3DEffect(player->m_efPlayingHandle, 0.0f, player->m_rotAngleY + DX_PI_F, 0.0f);
	}
}
