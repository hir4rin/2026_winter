#include "AttackCol.h"
#include "CharacterBase.h"
#include "Player.h"
#include "../Camera/CameraManager.h"
#include "../Camera/CameraState/CameraStateBase.h"
#include "../System.h"
#include "../BattleManager.h"
#include "Enemy/EnemyBase.h"
#include "CharacterBase.h"
#include "HitCol.h"
#include "JustDodgeCol.h"
//#include "../Effect/EffectManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	constexpr float kAttackColOffset = 50.0f;//攻撃判定を前に出す距離//本来はここも攻撃ごとに変えるべき

	constexpr float kCameraShakePower = 2.5f;//カメラの揺れの強さ
	constexpr float kCameraShakeTime = 5.0f;//カメラの揺れの時間

	constexpr float kEfOffset = 80.0f;//エフェクトの座標のオフセット

	constexpr int kUltStartFrame = 120;//必殺技の演出の開始フレーム数
	constexpr float kUltTimeScaleRate = 0.1f;//必殺技演出中の時間スケール

	constexpr int kSkillGaugeGainPerHit = 20;//攻撃ヒット時のスキルゲージ上昇量
	constexpr int kUltGaugeGainPerHitNormal = 10;//通常時の攻撃ヒット時の必殺技ゲージ上昇量
	constexpr int kUltGaugeGainPerHitRaven = 20;//raven状態時の攻撃ヒット時の必殺技ゲージ上昇量

	constexpr float kUltHitEffectOffsetMultiplier = 1.2f;//必殺技被ダメエフェクトのオフセット倍率
	constexpr float kUltHitEffectScale = 0.9f;//必殺技被ダメエフェクトのスケール
	constexpr float kUltHitEffectRotationOffset = DX_PI_F * 0.5f;//必殺技被ダメエフェクトのZ軸傾き分の回転オフセット
}

AttackCol::AttackCol(std::weak_ptr<CharacterBase> owner, const CharacterBase::AttackData& data)
	: m_owner(owner)
{
	if (m_owner.expired())return;
	//AttackDataを保持
	m_attackData = std::make_shared<CharacterBase::AttackData>(data);
	//通常エフェクトを出す
	m_hitEfHandle = System::GetInstance().GetHandle(AsyncData::PlayerHitEffect);
}

AttackCol::~AttackCol()
{
}

void AttackCol::OnCollision(Collider& other)
{
	//idで当たったかどうかを管理する//当たったidのリストにotherのidがないとき、攻撃を当てる
	//当たっていたらotherの被ダメ処理をして、Ownerに当たったことを通知してもよい
	auto owner = m_owner.lock();
	if (!owner) return;

	//Tag処理
	//Staticなら早期リターン
	auto tag = GetTag();

	switch (tag.faction)
	{
	case Collider::Faction::Player:
		//Playerの攻撃が当たった時の処理
		PlayerAttackOnCollision(other);
		break;
	case Collider::Faction::Enemy:
		//Enemyの攻撃が当たった時の処理
		EnemyAttackOnCollision(other);
		break;
	default:
		//早期リターン
		return;
		break;
	}



}

void AttackCol::ApplyPos()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//座標の更新//所有者のvelをもらって更新
	m_rb.m_pos = owner->GetRigidBody().GetPos();
	//m_attackDataに基づいて、このkAttackColOffsetを変えるようにしないといけない
	m_rb.m_pos += owner->GetTargetVec() * kAttackColOffset;//攻撃判定を前に出す
}
void AttackCol::Update()
{

}

void AttackCol::PlayerAttackOnCollision(Collider& other)
{
	auto it = m_owner.lock();
	if (!it)return;
	auto player = std::dynamic_pointer_cast<Player>(it);
	if (!player)return;

	if (other.GetTag().role == Collider::ColRole::Hit &&
		(other.GetTag().faction == Collider::Faction::Enemy || other.GetTag().faction == Collider::Faction::Boss))
	{

		int otherId = other.GetId();
		auto it = std::find(m_hitIds.begin(), m_hitIds.end(), otherId);
		if (it == m_hitIds.end())
		{
			// 初めての敵と当たった場合の処理

			//プレイヤーのゲージ管理//今は複数の敵に当たったらその分ゲージが上がるようになっている
			PlayerGaugeUp(other);
			//リザルト集計用//与えたダメージ、コンボ数(総ヒット数)を加算する
			player->AddAttackResult(m_attackData->attackPower);

			//otherの被ダメ処理

			//hitColのOnDamageInterFaceを呼ぶ
			auto hitCol = dynamic_cast<HitCol*>(&other);
			if (hitCol)
			{
				auto cameraManager = player->GetCameraManager().lock();
				if (!cameraManager)
				{
					m_hitIds.push_back(otherId);//当たったidのリストにotherのidを追加する
					return;
				}
				//ヒットした親
				auto hitOwner = hitCol->GetOwner().lock();
				if (!hitOwner)return;

				///---------
				/// ここで、Playerの初めて当たった時という関数を呼び出して、
				/// そこでカメラを揺らしたり、ターゲットを保存したりする
				///---------

				//最初にあたった攻撃だったらカメラを揺らす//必殺技中は揺らさない
				if (m_hitIds.empty() && GetTag().role != Collider::ColRole::UltAttack)
				{
					cameraManager->StartCameraShake(kCameraShakePower, kCameraShakeTime);//カメラを揺らす
				}

				//attackDataの変更//現在経過時間を引いて、敵の移動距離、時間を決める
				float nowAnimFrame = player->GetAnimation().GetNowAnimFrame();
				m_attackData->knockBackFrame -= nowAnimFrame;
				//ダメージの受け渡し
				hitCol->OnDamageInterFace(*this, *m_attackData);
				//ヒットストップの受け渡し
				//hitCol->SetTimeScaleInterFace(0.3f, 10.0f);

				//プレイヤーの攻撃が当たった時の処理//カメラシェイクや、内部ターゲットのセット
				player->OnAttackHit(otherId);

				//当たった敵(HitColの持ち主)
				auto hitOwnerEnemy = std::dynamic_pointer_cast<EnemyBase>(hitOwner);


				//ownerに当たったことを連絡->AttackMoveを止める
				//ロックオンしていないときは、最初に当たった敵を内部ターゲットにする
				if (!player->IsLockOn() && !player->GetSoftTarget())
				{
					player->SetSoftTarget(hitOwnerEnemy);
				}
				//攻撃の対象に当たったら攻撃の移動を止める
				if (hitOwnerEnemy && hitOwnerEnemy == player->GetAttackTarget())
				{
					//攻撃の移動を止める
					auto& comboInfo = player->GetComboInfo();
					comboInfo.isHit = true;//攻撃が当たったことを通知する//これで、攻撃の移動を止める
				}
			}
			//もしプレイヤーの必殺技攻撃だったら
			if (GetTag().role == Collider::ColRole::UltAttack)
			{
				auto cameraManager = player->GetCameraManager().lock();

				//演出が始まっていなかったら
				bool isUltStart = System::GetInstance().GetBattleMgr()->GetIsUltimating();
				if (!isUltStart)
				{
					System::GetInstance().GetBattleMgr()->SetUltStart(kUltStartFrame);//必殺技の演出をスタートする
					if (!System::GetInstance().GetBattleMgr()->GetIsLastHitEventPlaying())
					{
						System::GetInstance().SetTimeScaleForFrames(kUltTimeScaleRate, kUltStartFrame);//時間を遅くする//60フレームで元に戻す
						//カメラを移行
						cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::UltCamera);
					}
				}
				if (m_hitIds.empty())
				{
					//System::GetInstance().GetSoundManager().PlaySE("UltAttackHitSE");
				}


				//必殺技の被ダメエフェクト
				//m_hitEfPlayingHandle = EffectManager::GetInstance().Play(AsyncData::EnemyHitEffectUlt,
					//Vector3(other.GetPos().x, other.GetPos().y + kEfOffset*kUltHitEffectOffsetMultiplier, other.GetPos().z),0.0f,kUltHitEffectScale);

				//カメラの水平角度をY軸回転に加え、Z軸の傾き(45度)がカメラから見て常に一定になるようにする
				float camAngleH = 0.0f;
				//	auto cameraManager = player->GetCameraManager().lock();
				if (cameraManager)
				{
					auto camera = cameraManager->GetActiveCamera();
					if (camera)
					{
						camAngleH = camera->GetCameraAngleH();
					}
				}
				SetRotationPlayingEffekseer3DEffect(m_hitEfPlayingHandle, 0.0f, camAngleH - kUltHitEffectRotationOffset, 0.0f);
			}
			//もしプレイヤーの通常攻撃だったら
			else
			{
				//現在のコンボインデックスをもとに、蹴り/スキル/剣のヒット音を鳴らし分ける
				if (m_hitIds.empty())
				{
					//System::GetInstance().GetSoundManager().PlaySE(GetHitSeName(*player));
				}
				/*m_hitEfPlayingHandle = PlayEffekseer3DEffect(m_hitEfHandle);
				SetPosPlayingEffekseer3DEffect(m_hitEfPlayingHandle, other.GetPos().x, other.GetPos().y+ kEfOffset, other.GetPos().z);*/
				//m_hitEfPlayingHandle = EffectManager::GetInstance().Play(AsyncData::EnemyHitEffect,
				//	Vector3(other.GetPos().x, other.GetPos().y + kEfOffset, other.GetPos().z));
				////新しく追加したプレイヤーのヒットエフェクトを2つとも同時に出す
				//EffectManager::GetInstance().Play(AsyncData::PlayerSwordHitEffect,
				//	Vector3(other.GetPos().x, other.GetPos().y + kEfOffset, other.GetPos().z));
				//EffectManager::GetInstance().Play(AsyncData::PlayerSwordHitEffect2,
				//	Vector3(other.GetPos().x, other.GetPos().y + kEfOffset, other.GetPos().z));
			}
			m_hitIds.push_back(otherId);//当たったidのリストにotherのidを追加する
		}
	}
	//敵やボス以外と当たったとき
	else
	{
		// 当たっていた場合の処理//なにもしない
		return;
	}
}

void AttackCol::EnemyAttackOnCollision(Collider& other)
{
	//プレイヤー以外(自分自身や他の敵)には当てない
	if (other.GetTag().faction != Collider::Faction::Player)return;

	if (other.GetTag().role == Collider::ColRole::Hit)
	{
		int otherId = other.GetId();
		auto it = std::find(m_hitIds.begin(), m_hitIds.end(), otherId);
		if (it == m_hitIds.end())
		{
			// 当たっていない場合の処理
			//otherの被ダメ処理
			m_hitIds.push_back(otherId);//当たったidのリストにotherのidを追加する
			//hitColのOnDamageInterFaceを呼ぶ
			auto hitCol = dynamic_cast<HitCol*>(&other);
			if (hitCol)
			{
				hitCol->OnDamageInterFace(*this, *m_attackData);
			}
		}
	}
	//プレイヤーのジャスト回避判定に当たったとき
	else if (other.GetTag().role == Collider::ColRole::JustDodge)
	{
		int otherId = other.GetId();
		auto it = std::find(m_hitIds.begin(), m_hitIds.end(), otherId);
		if (it == m_hitIds.end())
		{
			m_hitIds.push_back(otherId);//当たったidのリストにotherのidを追加する
			//JustDodgeColのOnJustDodgeInterFaceを呼ぶ
			auto justDodgeCol = dynamic_cast<JustDodgeCol*>(&other);
			if (justDodgeCol)
			{
				justDodgeCol->OnJustDodgeInterFace(*this, *m_attackData);
			}
		}
	}
	else
	{
		// 当たっていた場合の処理//なにもしない
		return;
	}
}

void AttackCol::PlayerGaugeUp(Collider& other)
{
	auto it = m_owner.lock();
	if (!it)return;
	auto player = std::dynamic_pointer_cast<Player>(it);
	if (!player)return;


	//通常攻撃ならスキル攻撃をあげる
	if (!player->GetIsRaven())
	{
		//スキルゲージの上昇
		player->AddSkillGauge(kSkillGaugeGainPerHit);
		//必殺技ゲージの上昇
		player->AddUltGauge(kUltGaugeGainPerHitNormal);
	}
	//raven状態なら
	else
	{
		//必殺技ではないのならスキルゲージを上げる//スキル攻撃
		if (GetTag().role != Collider::ColRole::UltAttack)
		{
			//必殺技ゲージの上昇
			player->AddUltGauge(kUltGaugeGainPerHitRaven);
		}

	}


}

std::string AttackCol::GetHitSeName(Player& player)const
{
	int comboIndex = player.GetComboInfo().currentComboIndex;
	if (comboIndex < 0)return "AttackSwordSE";//コンボ中でなければ剣攻撃のヒット音をデフォルトにする

	//現在のコンボの振りSE名(seName)から、蹴り/スキル/剣のどれかを判定する
	const std::string& swingSeName = player.GetComboNode(comboIndex).seName;
	if (swingSeName.find("Kick") != std::string::npos)return "KickSE";
	if (swingSeName.find("Skill") != std::string::npos)return "SkillSE";
	return "AttackSwordSE";
}
