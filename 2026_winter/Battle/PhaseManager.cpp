#include "PhaseManager.h"
#include "Character/Enemy/EnemyManager.h"
#include "Character/Player/Player.h"
#include "DataLoader/DataManager.h"
#include "Collision/BoxShape.h"
#include "Battle/BattleManager.h"
#include "Main/System.h"
#include "Camera/CameraManager.h"
#include "DxLib.h"
#include <cassert>
#include <cmath>

namespace
{
	constexpr float kDebugSkipBackDistance = 300.0f;//F4で移すとき、エリアの手前どれだけ離すか
	constexpr float kBossFovOffset = 20.0f;//ボス戦で視野角を広げる角度(度)
	constexpr float kBossFovBlendFrame = 60.0f;//広げるのにかけるフレーム数
}

void PhaseManager::Init(std::weak_ptr<EnemyManager> enemyManager, std::weak_ptr<Player> player)
{
	m_enemyManager = enemyManager;
	m_player = player;
	m_currentPhase = 0;

	auto& data = DataManager::GetInstance();
	m_transitions = data.GetPhaseFlow();
	m_areas = data.GetPhaseAreas();
	assert(!m_transitions.empty() && "遷移表CSVがありません");

	if (auto enemyMgr = m_enemyManager.lock())
	{
		enemyMgr->SetSpawnData(data.GetEnemySpawnData());
	}

	ResolveAreaIndex();
	assert(GetMissingAreaNames().empty() && "遷移表のエリア名がエリアCSVにありません");

	//Startの行から始める
	for (const auto& t : m_transitions)
	{
		if (t.from != 0 || t.condition != PhaseConditionType::Start)continue;
		ChangePhase(t.to, t.event);
		return;
	}
	assert(false && "遷移表にStartの行がありません");
}

void PhaseManager::ResolveAreaIndex()
{
	//EnterAreaの行は、エリア名を番号に直しておく
	for (auto& t : m_transitions)
	{
		if (t.condition != PhaseConditionType::EnterArea)continue;

		t.areaIndex = -1;//引き直しのときに前の番号が残らないように
		for (int i = 0; i < static_cast<int>(m_areas.size()); i++)
		{
			if (m_areas[i].name != t.param)continue;
			//名前が一緒なら
			t.areaIndex = i;
			break;
		}
	}
}

void PhaseManager::Update()
{
	if (m_currentPhase <= 0)return;
	m_phaseFrame++;

	//次のフェーズへ進条件を満たしたかを調べ、満たしたら次のを出すようにする

	for (const auto& trans : m_transitions)
	{
		if (trans.from != m_currentPhase)continue;
		//条件が満たされているかチェック
		if (!IsConditionFulfilled(trans))continue;

		ChangePhase(trans.to, trans.event);
		return;
	}
}

void PhaseManager::ChangePhase(int next, PhaseEvent event)
{
	m_currentPhase = next;
	m_phaseFrame = 0;

	if (auto enemyMgr = m_enemyManager.lock())
	{
		m_killCountAtPhaseStart = enemyMgr->GetTotalKillCount();
		//-1(終了)なら敵は出さない
		if (next > 0)
		{
			enemyMgr->StartPhase(next);
		}
		else if(next == -1)
		{
			//終了の合図だけ出す(遷移はSceneMain)
			System::GetInstance().GetBattleMgr()->SetIsBattleEnd(true);
		}
	}

	//eventの処理
	switch (event)
	{
	case PhaseEvent::BossAppear:
		//printfDx("BossAppear\n");//仮
		//ボス戦は視野角を広げる
		if (auto player = m_player.lock())
		{
			if (auto cameraMgr = player->GetCameraManager().lock())
			{
				cameraMgr->SetFovOffset(kBossFovOffset, kBossFovBlendFrame);
			}
		}
		break;
	case PhaseEvent::GameClear:
		//printfDx("GameClear\n");//仮
		break;
	default:
		break;
	}

}

bool PhaseManager::IsConditionFulfilled(const PhaseTransition& trans) const
{
	auto enemyMgr = m_enemyManager.lock();
	if (!enemyMgr)return false;


	//一旦いまはこれだけ
	switch (trans.condition)
	{
	case PhaseConditionType::Start:
		return true;
	case PhaseConditionType::AllDead:
		if (trans.param.empty())return enemyMgr->IsPhaseEnemiesAllDead();
		return enemyMgr->IsGroupAllDead(trans.param);
		break;
	case PhaseConditionType::Timer:
		return m_phaseFrame >= trans.intParam;
	case PhaseConditionType::KillCount:
		return GetPhaseKillCount() >= trans.intParam;
	case PhaseConditionType::EnterArea:
	{
		auto player = m_player.lock();
		if (!player)return false;
		//Releaseではassertが無いので、ここでも弾く
		if (trans.areaIndex < 0 || trans.areaIndex >= static_cast<int>(m_areas.size()))return false;
		//足元で判定する
		return IsInArea(m_areas[trans.areaIndex], player->GetRigidBody().GetPos());
	}

	default:
		return false;
	}


	return false;
}

void PhaseManager::DebugSkipToBossPrevPhase()
{
	const int r0Phase = FindEnemyPhase(EnemyType::BossEnemyPirateR0);
	const int r1Phase = FindEnemyPhase(EnemyType::BossEnemyPirateR1);

	//第一ラウンド中なら第二ラウンドへ//プレイヤーはそのまま
	if (r0Phase > 0 && r1Phase > 0 && m_currentPhase == r0Phase)
	{
		if (auto enemyMgr = m_enemyManager.lock())
		{
			enemyMgr->DebugClearEnemies();
		}
		ChangePhase(r1Phase, PhaseEvent::None);
		return;
	}

	//ボスがいるフェーズ//第一ラウンド・第二ラウンドのうち早いほう
	int bossPhase = r0Phase;
	if (bossPhase <= 0 || (r1Phase > 0 && r1Phase < bossPhase))bossPhase = r1Phase;
	if (bossPhase <= 0)return;

	//ボスのフェーズへ進む行
	const PhaseTransition* toBoss = nullptr;
	for (const auto& t : m_transitions)
	{
		if (t.to != bossPhase || t.from <= 0)continue;
		toBoss = &t;
		break;
	}
	if (!toBoss)return;

	//今いる敵を消す
	if (auto enemyMgr = m_enemyManager.lock())
	{
		enemyMgr->DebugClearEnemies();
	}
	ChangePhase(toBoss->from, PhaseEvent::None);

	//エリアの手前(-Z側)に移す//高さは箱の中心にして、床に落とす
	if (toBoss->condition != PhaseConditionType::EnterArea || toBoss->areaIndex < 0)return;
	auto player = m_player.lock();
	if (!player)return;
	const PhaseArea& area = m_areas[toBoss->areaIndex];
	float back = area.halfExtents.z + kDebugSkipBackDistance;
	float rad = area.rotYDeg * DX_PI_F / 180.0f;
	player->SetPos(area.position + Vector3(-sinf(rad) * back, 0.0f, -cosf(rad) * back));
	player->GetRigidBody().ResetVel();
}

int PhaseManager::FindEnemyPhase(EnemyType type) const
{
	//同じ種類が何体かいたら早いほう
	int phase = -1;
	for (const auto& data : DataManager::GetInstance().GetEnemySpawnData())
	{
		if (data.type != type)continue;
		if (phase < 0 || data.phase < phase)phase = data.phase;
	}
	return phase;
}

bool PhaseManager::IsInArea(const PhaseArea& area, const Vector3& pos) const
{
	//箱の向きに合わせて、中心から見た位置に直す
	BoxShape box(area.halfExtents, area.rotYDeg * DX_PI_F / 180.0f);
	//playerを回転させることで箱と比較する(回転を影響させなくする
	Vector3 local = box.WorldToLocalDir(pos - area.position);

	return fabsf(local.x) <= area.halfExtents.x &&
		fabsf(local.y) <= area.halfExtents.y &&
		fabsf(local.z) <= area.halfExtents.z;
}

int PhaseManager::GetPhaseKillCount() const
{
	auto enemyMgr = m_enemyManager.lock();
	if (!enemyMgr)return 0;
	//開始時の撃破数を引く
	return enemyMgr->GetTotalKillCount() - m_killCountAtPhaseStart;
}

void PhaseManager::DebugDraw() const
{
	DrawFormatString(10, 150, GetColor(0, 255, 0), "Phase:%d Frame:%d Kill:%d",
		m_currentPhase, m_phaseFrame, GetPhaseKillCount());
}

void PhaseManager::SetAreas(const std::vector<PhaseArea>& areas)
{
	m_areas = areas;
	//名前や順番が変わっているかもしれないので引き直す(エディタ中はassertしない)
	ResolveAreaIndex();
}

bool PhaseManager::IsPlayerInArea(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_areas.size()))return false;
	auto player = m_player.lock();
	if (!player)return false;
	return IsInArea(m_areas[index], player->GetRigidBody().GetPos());
}

std::vector<std::string> PhaseManager::GetMissingAreaNames() const
{
	std::vector<std::string> names;
	for (const auto& t : m_transitions)
	{
		if (t.condition != PhaseConditionType::EnterArea)continue;
		if (t.areaIndex < 0)names.push_back(t.param);
	}
	return names;
}

void PhaseManager::DrawAreas(int selectedIndex) const
{
	for (int i = 0; i < static_cast<int>(m_areas.size()); i++)
	{
		const PhaseArea& area = m_areas[i];
		unsigned int color;
		if (i == selectedIndex)
		{
			color = GetColor(255, 255, 255);//選択中は白
		}
		else if (IsPlayerInArea(i))
		{
			color = GetColor(255, 0, 0);//中にいるときは赤
		}
		else
		{
			color = GetColor(255, 255, 0);//黄
		}
		//判定と同じBoxShapeで描く
		BoxShape box(area.halfExtents, area.rotYDeg * DX_PI_F / 180.0f);
		box.DebugDraw(area.position, color);
	}
}
