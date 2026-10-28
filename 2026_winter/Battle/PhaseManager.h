#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Stage/Phase/EnemySpawnData.h"
#include "Stage/Phase/PhaseFlowData.h"
#include "Stage/Phase/PhaseAreaData.h"

class EnemyManager;
class Player;

//フェーズ進行の管理
class PhaseManager
{
public:
	PhaseManager() = default;
	~PhaseManager() = default;


	//最初のフェーズを始める//DataManagerで読み込んだ後に呼ぶ
	void Init(std::weak_ptr<EnemyManager> enemyManager, std::weak_ptr<Player> player);

	void Update();

	int GetCurrentPhase()const { return m_currentPhase; }//0:未開始 -1:終了
	int GetPhaseKillCount()const;//このフェーズの撃破数

	void DebugDraw()const;

	//デバッグ用//ボスの一つ手前のフェーズにして、敵を消し、プレイヤーをボスのエリアの手前に移す
	//ボスのフェーズ中なら次のフェーズへ飛ぶ
	void DebugSkipToBossPrevPhase();

	//エリア(エディタ用)-----------------------------------------------------
	const std::vector<PhaseArea>& GetAreas()const { return m_areas; }
	void SetAreas(const std::vector<PhaseArea>& areas);//名前→番号も引き直す
	bool IsPlayerInArea(int index)const;
	std::vector<std::string> GetMissingAreaNames()const;//遷移表にあってエリアに無い名前
	void DrawAreas(int selectedIndex = -1)const;//箱の線枠(選択中は白)
	//-----------------------------------------------------------------------

private:
	//EnterAreaの行のエリア名を番号に直す(無ければ-1)
	void ResolveAreaIndex();

	//次のフェーズ
	void ChangePhase(int next, PhaseEvent event);

	//条件が満たされたかどうか
	bool IsConditionFulfilled(const PhaseTransition& trans)const;

	//posがエリアの箱の中にあるか
	bool IsInArea(const PhaseArea& area, const Vector3& pos)const;

private:
	std::weak_ptr<EnemyManager> m_enemyManager;
	std::weak_ptr<Player> m_player;//EnterAreaの判定用

	std::vector<PhaseTransition> m_transitions;//遷移表
	std::vector<PhaseArea> m_areas;//EnterArea用のエリア
	int m_currentPhase = 0;
	int m_phaseFrame = 0;//経過フレーム
	int m_killCountAtPhaseStart = 0;//フェーズ開始時の撃破数
};
