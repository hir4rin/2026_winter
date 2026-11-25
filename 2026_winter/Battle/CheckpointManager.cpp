#include "CheckpointManager.h"
#include "Character/Player/Player.h"
#include "DataLoader/DataManager.h"
#include "Render/EffectManager.h"
#include "EffekseerForDXLib.h"
#include "DxLib.h"
#include <algorithm>

namespace
{
	constexpr float kEffectScale = 20.0f;//エフェクトは1/20で作ってある
	constexpr float kInsideHeight = 300.0f;//踏んだとみなす高さの差(上下)
}

CheckpointManager::~CheckpointManager()
{
	StopAllIdleEffects();
}

void CheckpointManager::Init(std::weak_ptr<Player> player, int startIndex)
{
	m_player = player;
	m_checkpoints = DataManager::GetInstance().GetCheckpoints();
	StopAllIdleEffects();
	m_states.assign(m_checkpoints.size(), State());
	m_lastIndex = -1;
	if (startIndex >= 0 && startIndex < static_cast<int>(m_checkpoints.size()))
	{
		Activate(startIndex, false);
	}

	//始まったときに中にいる物は演出なしで起動済みにする
	if (auto p = m_player.lock())
	{
		const Vector3 pos = p->GetRigidBody().GetPos();
		for (int i = static_cast<int>(m_checkpoints.size()) - 1; i > m_lastIndex; i--)
		{
			if (!IsInside(m_checkpoints[i], pos))continue;
			Activate(i, false);
			break;
		}
	}
	UpdateIdleEffect();
}

void CheckpointManager::Update()
{
	auto p = m_player.lock();
	if (!p)return;
	const Vector3 pos = p->GetRigidBody().GetPos();

	//まだ踏んでいない物(次より先も見る。飛ばしたとき用)
	for (int i = static_cast<int>(m_checkpoints.size()) - 1; i > m_lastIndex; i--)
	{
		if (!IsInside(m_checkpoints[i], pos))continue;
		Activate(i, true);
		break;
	}

	UpdateIdleEffect();
}

const CheckpointData* CheckpointManager::GetLastCheckpoint() const
{
	if (m_lastIndex < 0 || m_lastIndex >= static_cast<int>(m_checkpoints.size()))return nullptr;
	return &m_checkpoints[m_lastIndex];
}

int CheckpointManager::GetNextIndex() const
{
	const int next = m_lastIndex + 1;
	return next < static_cast<int>(m_checkpoints.size()) ? next : -1;
}

bool CheckpointManager::IsActivated(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_states.size()))return false;
	return m_states[index].isActivated;
}

void CheckpointManager::SetCheckpoints(const std::vector<CheckpointData>& checkpoints)
{
	if (checkpoints.size() == m_checkpoints.size())
	{
		//数が同じなら状態はそのままで、出ているIdleだけ動かす
		m_checkpoints = checkpoints;
		for (int i = 0; i < static_cast<int>(m_states.size()); i++)
		{
			const int handle = m_states[i].idleHandle;
			if (handle < 0)continue;
			const CheckpointData& cp = m_checkpoints[i];
			SetPosPlayingEffekseer3DEffect(handle, cp.position.x, cp.position.y, cp.position.z);
			SetRotationPlayingEffekseer3DEffect(handle, 0.0f, cp.rotYDeg * DX_PI_F / 180.0f, 0.0f);
		}
		return;
	}

	//数が変わったら作り直す(踏んだ番号までは起動済みのまま)
	StopAllIdleEffects();
	m_checkpoints = checkpoints;
	m_states.assign(m_checkpoints.size(), State());
	m_lastIndex = (std::min)(m_lastIndex, static_cast<int>(m_checkpoints.size()) - 1);
	for (int i = 0; i <= m_lastIndex; i++)
	{
		m_states[i].isActivated = true;
	}
	UpdateIdleEffect();
}

bool CheckpointManager::IsPlayerInside(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_checkpoints.size()))return false;
	auto p = m_player.lock();
	if (!p)return false;
	return IsInside(m_checkpoints[index], p->GetRigidBody().GetPos());
}

void CheckpointManager::Activate(int index, bool isPlayEffect)
{
	//飛ばした物も含めて、ここまでを起動済みにする
	for (int i = 0; i <= index; i++)
	{
		m_states[i].isActivated = true;
		StopIdleEffect(i);
	}
	m_lastIndex = index;

	if (isPlayEffect)
	{
		//Idleと同じ見た目から始まるので、同じ位置で入れ替える
		const CheckpointData& cp = m_checkpoints[index];
		EffectManager::GetInstance().Play(AsyncData::CheckpointActivateEffect, cp.position,
			cp.rotYDeg * DX_PI_F / 180.0f, kEffectScale);
	}
}

void CheckpointManager::UpdateIdleEffect()
{
	const int next = GetNextIndex();
	for (int i = 0; i < static_cast<int>(m_states.size()); i++)
	{
		State& state = m_states[i];
		if (i != next)
		{
			StopIdleEffect(i);
			continue;
		}
		//出ていない・寿命で終わったら出し直す//IsEffekseer3DEffectPlayingは再生中なら0
		if (state.idleHandle < 0 || IsEffekseer3DEffectPlaying(state.idleHandle) != 0)
		{
			const CheckpointData& cp = m_checkpoints[i];
			state.idleHandle = EffectManager::GetInstance().Play(AsyncData::CheckpointIdleEffect, cp.position,
				cp.rotYDeg * DX_PI_F / 180.0f, kEffectScale);
		}
	}
}

void CheckpointManager::StopIdleEffect(int index)
{
	State& state = m_states[index];
	if (state.idleHandle < 0)return;
	StopEffekseer3DEffect(state.idleHandle);
	state.idleHandle = -1;
}

void CheckpointManager::StopAllIdleEffects()
{
	for (int i = 0; i < static_cast<int>(m_states.size()); i++)
	{
		StopIdleEffect(i);
	}
}

bool CheckpointManager::IsInside(const CheckpointData& cp, const Vector3& pos) const
{
	const float dx = pos.x - cp.position.x;
	const float dz = pos.z - cp.position.z;
	const float dy = pos.y - cp.position.y;
	return dx * dx + dz * dz <= cp.radius * cp.radius && dy > -kInsideHeight && dy < kInsideHeight;
}
