#include "TrainingManager.h"
#include "Character/Enemy/EnemyManager.h"
#include "Character/Enemy/EnemyBase.h"
#include "Character/Player/Player.h"
#include "Stage/Phase/EnemySpawnData.h"

namespace
{
	//プレイヤーのスタート(部屋の南の壁の前、北を向く)
	const Vector3 kPlayerStartPos = Vector3(0.0f, 0.0f, -1700.0f);
	//敵を出す円(広場の真ん中)
	const Vector3 kSpawnCenter = Vector3(0.0f, 0.0f, -200.0f);
	//円のまわりの置き場所(円の中心からのずらし。プレイヤーは -Z 側)
	const Vector3 kOneSlots[] = { Vector3(0.0f, 0.0f, 0.0f) };
	const Vector3 kThreeSlots[] = { Vector3(-300.0f, 0.0f, -150.0f), Vector3(0.0f, 0.0f, 50.0f), Vector3(300.0f, 0.0f, -150.0f) };//プレイヤー側に開いた弧
	const Vector3 kFiveSlots[] =//円の上(半径300)
	{
		Vector3(0.0f, 0.0f, 300.0f), Vector3(285.0f, 0.0f, 93.0f), Vector3(176.0f, 0.0f, -243.0f),
		Vector3(-176.0f, 0.0f, -243.0f), Vector3(-285.0f, 0.0f, 93.0f),
	};
	constexpr float kEnemyRotYDeg = 180.0f;//プレイヤー(-Z)の方を向く
	constexpr int kRespawnFrame = 60;//倒れてから出し直すまで
	//混ぜるときの順番
	const EnemyType kMixTypes[] = { EnemyType::Swordman, EnemyType::Gunner, EnemyType::Bazooka };
}

void TrainingManager::Init(std::weak_ptr<EnemyManager> enemyManager, std::weak_ptr<Player> player)
{
	m_enemyManager = enemyManager;
	m_player = player;
	ReturnPlayerToStart();
	Respawn();
}

void TrainingManager::Apply(const Settings& settings)
{
	m_settings = settings;
	Respawn();
}

void TrainingManager::Respawn()
{
	auto enemyMgr = m_enemyManager.lock();
	if (!enemyMgr)return;
	enemyMgr->DebugClearEnemies();
	m_slots.clear();

	const auto typeItem = static_cast<EnemyTypeItem>(m_settings.enemyType);
	if (typeItem == EnemyTypeItem::Boss)
	{
		m_slots.push_back({ static_cast<int>(EnemyType::BossEnemyKingmech), kSpawnCenter });
	}
	else
	{
		const Vector3* offsets = kOneSlots;
		int num = 1;
		if (m_settings.enemyCount == static_cast<int>(EnemyCountItem::Three)) { offsets = kThreeSlots; num = 3; }
		if (m_settings.enemyCount == static_cast<int>(EnemyCountItem::Five)) { offsets = kFiveSlots; num = 5; }
		for (int i = 0; i < num; ++i)
		{
			EnemyType type = EnemyType::Swordman;
			if (typeItem == EnemyTypeItem::Gunner) type = EnemyType::Gunner;
			if (typeItem == EnemyTypeItem::Bazooka) type = EnemyType::Bazooka;
			if (typeItem == EnemyTypeItem::Mix) type = kMixTypes[i % 3];
			m_slots.push_back({ static_cast<int>(type), kSpawnCenter + offsets[i] });
		}
	}
	for (auto& slot : m_slots)
	{
		SpawnSlot(slot);
	}
}

void TrainingManager::SpawnSlot(Slot& slot)
{
	auto enemyMgr = m_enemyManager.lock();
	if (!enemyMgr)return;
	EnemySpawnData data;
	data.type = static_cast<EnemyType>(slot.type);
	data.position = slot.pos;
	data.rotYDeg = kEnemyRotYDeg;
	data.initialState = EnemyInitialState::Idle;
	auto enemy = enemyMgr->SpawnFromData(data);
	slot.enemy = enemy;
	slot.respawnTimer = -1;
	if (enemy) ApplyAi(*enemy);
}

void TrainingManager::ApplyAi(EnemyBase& enemy)const
{
	const EnemyBase::TrainingAi ais[] =
	{
		EnemyBase::TrainingAi::Stand, EnemyBase::TrainingAi::Guard, EnemyBase::TrainingAi::Attack, EnemyBase::TrainingAi::Normal,
	};
	enemy.SetTraining(ais[m_settings.enemyAi], m_settings.enemyHp == 1);
}

void TrainingManager::ReturnPlayerToStart()
{
	auto player = m_player.lock();
	if (!player)return;
	player->SetPos(kPlayerStartPos);
	player->SetFacing(0.0f);
}

void TrainingManager::Update()
{
	//敵: 動きを進める・倒れたら出し直す
	for (auto& slot : m_slots)
	{
		auto enemy = slot.enemy.lock();
		if (enemy && !enemy->GetIsLifeZero())
		{
			//暗殺・確殺の途中はそのまま(倒れてから出し直す)
			if (!enemy->GetIsExecuted()) enemy->UpdateTraining();
			continue;
		}
		if (m_settings.respawn != 0)continue;
		if (slot.respawnTimer < 0) slot.respawnTimer = 0;
		if (++slot.respawnTimer >= kRespawnFrame) SpawnSlot(slot);
	}

	//プレイヤー: HP・ゲージの無限
	auto player = m_player.lock();
	if (!player)return;
	if (m_settings.playerHp == 0) player->RestoreHp();
	if (m_settings.gauge == 0)
	{
		player->MaxSkillGauge();
		player->MaxUltGauge();
	}
}
