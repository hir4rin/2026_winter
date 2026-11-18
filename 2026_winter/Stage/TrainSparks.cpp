#include "TrainSparks.h"
#include "Main/System.h"
#include "EffekseerForDXLib.h"
#include <algorithm>

namespace
{
	//右は +X、左は -X(外側)へ飛ぶ。マイナスの大きさで反転するとビルボードが壊れて画面が白くなったので2つに分けた
	const char* const kEffectPathRight = "data/Effect/Train/TrainSpark.efk";
	const char* const kEffectPathLeft = "data/Effect/Train/TrainSparkL.efk";
	constexpr float kEffectScale = 45.0f;//エフェクトは1/20で作った。20倍だと小さいので大きめに出す

	//車両(TrainStage.mv1 の z の範囲。機関車も入れる)
	constexpr float kCarRanges[][2] = {
		{ -7872.0f, -350.0f }, { -150.0f, 1850.0f }, { 2050.0f, 4050.0f }, { 4250.0f, 8296.0f }, { 8497.0f, 10497.0f },
		{ 10697.0f, 14744.0f }, { 14944.0f, 16843.0f }, { 17144.0f, 21190.0f }, { 21391.0f, 23391.0f } };
	constexpr float kBogieInset = 350.0f;//車両の端から台車まで
	constexpr float kSparkX = 825.0f;//車体の横(±800)のすぐ外(内側だと車体に隠れる)
	constexpr float kSparkY = -75.0f;//車体の下の端(-60)の少し下

	//出す間隔(フレーム)と出す距離
	constexpr int kIntervalMin = 40;
	constexpr int kIntervalMax = 160;
	constexpr float kActiveDistance = 3000.0f;
}

TrainSparks::~TrainSparks()
{
	for (int handle : m_playingHandles)
	{
		StopEffekseer3DEffect(handle);
	}
	if (m_effectRight != -1) DeleteEffekseerEffect(m_effectRight);
	if (m_effectLeft != -1) DeleteEffekseerEffect(m_effectLeft);
}

void TrainSparks::Init(const Vector3& stagePos)
{
	m_effectRight = LoadEffekseerEffect(kEffectPathRight);
	m_effectLeft = LoadEffekseerEffect(kEffectPathLeft);
	m_points.clear();
	for (const auto& range : kCarRanges)
	{
		for (float z : { range[0] + kBogieInset, range[1] - kBogieInset })
		{
			for (bool isLeft : { false, true })
			{
				const Vector3 pos = stagePos + Vector3(isLeft ? -kSparkX : kSparkX, kSparkY, z);
				//最初はばらばらに出るように間隔の中の乱数から始める
				m_points.push_back({ pos, isLeft, static_cast<float>(GetRand(kIntervalMax)) });
			}
		}
	}
}

void TrainSparks::Update(const Vector3& playerPos)
{
	if (m_effectRight == -1 || m_effectLeft == -1) return;

	//終わったものを外す
	m_playingHandles.erase(std::remove_if(m_playingHandles.begin(), m_playingHandles.end(),
		[](int handle) { return IsEffekseer3DEffectPlaying(handle) != 0; }), m_playingHandles.end());

	//スローのときは間隔もゆっくり進める
	const float step = System::GetInstance().GetTimeScale();
	for (Point& point : m_points)
	{
		point.timer -= step;
		if (point.timer > 0.0f) continue;
		point.timer += RandomInterval();
		if ((point.pos - playerPos).Magnitude() >= kActiveDistance) continue;

		const int handle = PlayEffekseer3DEffect(point.isLeft ? m_effectLeft : m_effectRight);
		if (handle == -1) continue;
		SetPosPlayingEffekseer3DEffect(handle, point.pos.x, point.pos.y, point.pos.z);
		SetScalePlayingEffekseer3DEffect(handle, kEffectScale, kEffectScale, kEffectScale);
		m_playingHandles.push_back(handle);
	}
}

float TrainSparks::RandomInterval()const
{
	return static_cast<float>(kIntervalMin + GetRand(kIntervalMax - kIntervalMin));
}
