#pragma once
#include <array>

//リザルト画面に出す結果と点数の計算
struct ResultData
{
	//詳細データの技(コンボは別)
	enum class Technique : int
	{
		PartBreak,//部位破壊
		PartBrokenKill,//確殺
		Assasin,//暗殺(空中も)
		Skill,//スキル技
		SkillKessatsu,//スキル血殺
		TrueUlt,//真必殺技
		Num
	};
	static constexpr int kTechniqueNum = static_cast<int>(Technique::Num);
	static constexpr int kRankNum = 5;

	int stageType = 0;
	int clearFrame = 0;//実フレーム
	int targetClearSec = 300;//これより早ければタイムは満点
	int killCount = 0;
	int enemyNum = 0;
	int maxCombo = 0;
	std::array<int, kTechniqueNum> techniqueCounts = {};
	int itemUseCount = 0;//仮
	int deathCount = 0;//仮

	int GetTechniqueCount(Technique t) const { return techniqueCounts[static_cast<int>(t)]; }
	int CalcComboScore() const;
	int CalcTechniqueScore(Technique t) const;
	int CalcTechniqueTotal() const;
	int CalcKillScore() const;
	int CalcTimeScore() const;
	float CalcItemRate() const;
	float CalcDeathRate() const;
	int CalcTotalScore() const;
	int CalcRank() const;//0が一番下
	static const char* GetRankName(int rank);
	static int GetKillScoreMax();
	static int GetTimeScoreMax();
};

//ゲーム中に結果を数える(BattleManagerが持つ)
class ResultRecorder
{
public:
	void Update();
	void AddTechnique(ResultData::Technique t);
	void OnPlayerAttackHit();
	void OnPlayerDamaged();
	ResultData MakeResult(int stageType, int targetClearSec, int killCount, int enemyNum) const;

private:
	int m_clearFrame = 0;
	int m_combo = 0;
	int m_maxCombo = 0;
	int m_comboTimer = 0;
	std::array<int, ResultData::kTechniqueNum> m_techniqueCounts = {};
};
