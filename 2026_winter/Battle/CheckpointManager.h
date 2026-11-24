#pragma once
#include <memory>
#include <vector>
#include "Stage/Phase/CheckpointData.h"

class Player;

//チェックポイントの管理
//次の1つだけエフェクトを出し、踏んだら起動して最後に踏んだ物を覚える
class CheckpointManager
{
public:
	CheckpointManager() = default;
	~CheckpointManager();

	//DataManagerで読み込んだ後に呼ぶ//始まったときに中にいる物は演出なしで起動済みにする
	void Init(std::weak_ptr<Player> player);

	void Update();

	//最後に踏んだチェックポイント(無ければnullptr)
	const CheckpointData* GetLastCheckpoint()const;
	int GetLastIndex()const { return m_lastIndex; }//-1なら無し
	int GetNextIndex()const;//次に踏む物(無ければ-1)
	bool IsActivated(int index)const;

	//エディタ用
	const std::vector<CheckpointData>& GetCheckpoints()const { return m_checkpoints; }
	void SetCheckpoints(const std::vector<CheckpointData>& checkpoints);
	bool IsPlayerInside(int index)const;

private:
	void Activate(int index, bool isPlayEffect);
	void UpdateIdleEffect();//次の物だけIdleを出す
	void StopIdleEffect(int index);
	void StopAllIdleEffects();
	bool IsInside(const CheckpointData& cp, const Vector3& pos)const;

	struct State
	{
		bool isActivated = false;
		int idleHandle = -1;//再生中のIdle(-1なら無し)
	};

	std::weak_ptr<Player> m_player;
	std::vector<CheckpointData> m_checkpoints;
	std::vector<State> m_states;//m_checkpointsと同じ並び
	int m_lastIndex = -1;//最後に踏んだ物
};
