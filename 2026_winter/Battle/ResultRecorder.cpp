#include "ResultRecorder.h"
#include <algorithm>

namespace
{
	constexpr int kComboKeepFrame = 90;//この間当たらなければコンボが切れる(実フレーム)
	constexpr int kComboPoint = 20;//最大コンボ1ヒットの点
	constexpr int kKillScoreMax = 20000;
	constexpr int kTimeScoreMax = 20000;
	constexpr int kTimeZeroRate = 3;//目標タイムの何倍で0点か
	constexpr int kFps = 60;
	constexpr float kPenaltyStep = 0.1f;//1回ごとに下がる倍率
	constexpr float kPenaltyMinRate = 0.5f;
}

int ResultData::CalcComboScore() const
{
	return maxCombo * kComboPoint;
}

int ResultData::CalcTechniqueScore(Technique t) const
{
	//1回の点(仮)
	static constexpr std::array<int, kTechniqueNum> kPoints = { 300, 800, 800, 150, 1000, 1500 };
	return GetTechniqueCount(t) * kPoints[static_cast<int>(t)];
}

int ResultData::CalcTechniqueTotal() const
{
	int total = CalcComboScore();
	for (int i = 0; i < kTechniqueNum; ++i)
	{
		total += CalcTechniqueScore(static_cast<Technique>(i));
	}
	return total;
}

int ResultData::CalcKillScore() const
{
	if (enemyNum <= 0) return kKillScoreMax;
	const int kill = (std::min)(killCount, enemyNum);
	return static_cast<int>(static_cast<long long>(kKillScoreMax) * kill / enemyNum);
}

int ResultData::CalcTimeScore() const
{
	const int targetFrame = targetClearSec * kFps;
	if (targetFrame <= 0 || clearFrame <= targetFrame) return kTimeScoreMax;
	const int zeroFrame = targetFrame * kTimeZeroRate;
	if (clearFrame >= zeroFrame) return 0;
	const float rate = 1.0f - static_cast<float>(clearFrame - targetFrame) / static_cast<float>(zeroFrame - targetFrame);
	return static_cast<int>(kTimeScoreMax * rate);
}

float ResultData::CalcItemRate() const
{
	return (std::max)(kPenaltyMinRate, 1.0f - kPenaltyStep * itemUseCount);
}

float ResultData::CalcDeathRate() const
{
	return (std::max)(kPenaltyMinRate, 1.0f - kPenaltyStep * deathCount);
}

int ResultData::CalcTotalScore() const
{
	const int sum = CalcTechniqueTotal() + CalcKillScore() + CalcTimeScore();
	return static_cast<int>(sum * CalcItemRate() * CalcDeathRate());
}

int ResultData::CalcRank() const
{
	//このスコア以上でそのランク(仮)
	static constexpr std::array<int, kRankNum> kBorders = { 0, 20000, 32000, 40000, 46000 };
	const int total = CalcTotalScore();
	int rank = 0;
	for (int i = 0; i < kRankNum; ++i)
	{
		if (total >= kBorders[i]) rank = i;
	}
	return rank;
}

const char* ResultData::GetRankName(int rank)
{
	static constexpr std::array<const char*, kRankNum> kNames = { "下忍", "中忍", "上忍", "頭領", "影" };
	return kNames[std::clamp(rank, 0, kRankNum - 1)];
}

int ResultData::GetKillScoreMax()
{
	return kKillScoreMax;
}

int ResultData::GetTimeScoreMax()
{
	return kTimeScoreMax;
}

void ResultRecorder::Update()
{
	m_clearFrame++;
	if (m_comboTimer > 0)
	{
		m_comboTimer--;
		if (m_comboTimer == 0) m_combo = 0;
	}
}

void ResultRecorder::AddTechnique(ResultData::Technique t)
{
	m_techniqueCounts[static_cast<int>(t)]++;
}

void ResultRecorder::OnPlayerAttackHit()
{
	m_combo++;
	m_maxCombo = (std::max)(m_maxCombo, m_combo);
	m_comboTimer = kComboKeepFrame;
}

void ResultRecorder::OnPlayerDamaged()
{
	m_combo = 0;
	m_comboTimer = 0;
}

ResultData ResultRecorder::MakeResult(int stageType, int targetClearSec, int killCount, int enemyNum) const
{
	ResultData data;
	data.stageType = stageType;
	data.clearFrame = m_clearFrame;
	data.targetClearSec = targetClearSec;
	data.killCount = killCount;
	data.enemyNum = enemyNum;
	data.maxCombo = m_maxCombo;
	data.techniqueCounts = m_techniqueCounts;
	return data;
}
