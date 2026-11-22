#include "TutorialFinale.h"
#include "DxLib.h"
#include "Stage.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Camera/CameraManager.h"
#include "Camera/CameraState/EndingCamera.h"
#include "Character/Player/Player.h"
#include <algorithm>
#include <cmath>

namespace
{
	const char* const kZoneName = "Kick_Long";//長い縦穴の壁キックのゾーン(Unity の TutorialStageTool と同じ名前)
	//カメラ(縦穴の開いている側の後ろから見て、高さだけプレイヤーについていく)
	constexpr float kRiseStartDist = 350.0f;//縦穴の端からこれだけ離れて始める
	constexpr float kRisePullDist = 2200.0f;//登る間にこれだけ遠ざかる
	constexpr float kRiseStartUp = 150.0f;//プレイヤーよりこれだけ上から
	constexpr float kRiseEndUp = 700.0f;//最後はこれだけ上から
	constexpr float kPullFrame = 360.0f;//遠ざかる長さ
	constexpr float kPullCheckLow = 500.0f;//後ろの壁を調べる一番下(足元からの高さ。中庭のコンテナより上)
	constexpr float kPullCheckStep = 200.0f;//後ろの壁を調べる高さの間隔
	constexpr float kPullWallMargin = 200.0f;//後ろの壁からこれだけ手前で止める
	constexpr float kPullSideDiff = 300.0f;//両側の引ける距離がこれ以上違えば、広い側から見る
	//終わり
	constexpr float kBlackStartBelowTop = 700.0f;//縦穴の上のこれだけ下まで登ったら暗くし始める
	constexpr int kMaxClimbFrame = 720;//登りきらなくても、これだけたったら暗くする
	constexpr int kBlackFrame = 40;//真っ黒になるまで
}

TutorialFinale::TutorialFinale(std::weak_ptr<Stage> stage, std::weak_ptr<Player> player, std::weak_ptr<CameraManager> cameraManager) :
	m_stage(stage),
	m_player(player),
	m_cameraManager(cameraManager)
{
}

TutorialFinale::~TutorialFinale()
{
	if (auto player = m_player.lock()) player->SetAutoWallKick(false);
}

void TutorialFinale::Update()
{
	auto battle = System::GetInstance().GetBattleMgr();
	auto stage = m_stage.lock();
	auto player = m_player.lock();
	if (!battle || !stage || !player)return;

	m_frame++;
	switch (m_phase)
	{
	case Phase::Wait:
		//縦穴の中で壁キックしたら始める
		if (player->IsWallKicking() && stage->IsInWallZoneNamed(kZoneName, *player)) Start();
		break;
	case Phase::Climb:
	{
		const float footY = player->GetRigidBody().GetPos().y;
		if (m_blackFrame < 0 && (footY >= m_topY - kBlackStartBelowTop || m_frame >= kMaxClimbFrame)) m_blackFrame = 0;
		if (m_blackFrame >= 0)
		{
			m_blackFrame++;
			battle->GetEndingScreen().blackRate = std::clamp(static_cast<float>(m_blackFrame) / kBlackFrame, 0.0f, 1.0f);
			if (m_blackFrame >= kBlackFrame)
			{
				battle->SetIsBattleEnd(true);//リザルトへ
				m_phase = Phase::Done;
			}
		}
		break;
	}
	case Phase::Done:
		break;
	}
}

void TutorialFinale::Start()
{
	auto battle = System::GetInstance().GetBattleMgr();
	auto stage = m_stage.lock();
	auto player = m_player.lock();
	auto cameraManager = m_cameraManager.lock();
	if (!battle || !stage || !player || !cameraManager)return;

	//ここからは操作させず、張り付いたら自動で蹴る
	battle->SetIsEventPlaying(true);
	player->SetAutoWallKick(true);

	//縦穴の中心の線と、開いている向き(ゾーンの長いほうの横の軸)
	const Vector3 foot = player->GetRigidBody().GetPos();
	m_topY = foot.y;
	Vector3 center = foot;
	Vector3 axis = Vector3(0.0f, 0.0f, 1.0f);
	float halfAlong = 0.0f;
	if (const WallZoneData* zone = stage->FindWallZoneData(kZoneName))
	{
		m_topY = zone->position.y + zone->halfExtents.y;
		center = zone->position;
		const bool isAlongX = zone->halfExtents.x > zone->halfExtents.z;
		axis = isAlongX ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 0.0f, 1.0f);
		halfAlong = isAlongX ? zone->halfExtents.x : zone->halfExtents.z;
	}
	//後ろの建物にめり込まないように、当たり判定の手前までしか引かない。両側を測って広く開いている側から見る
	//(片側は屋上の塔なので、カメラがそちらにいると塔の中に入っていた)。同じくらいなら今のカメラがいる側
	const float startDist = halfAlong + kRiseStartDist;
	const float wantDist = startDist + kRisePullDist;
	const float lowY = foot.y + kPullCheckLow;
	const float limitPlus = CalcPullLimit(*stage, center, axis, halfAlong, wantDist, lowY);
	const float limitMinus = CalcPullLimit(*stage, center, axis * -1.0f, halfAlong, wantDist, lowY);
	const Vector3 camPos = Vector3(GetCameraPosition());
	float side = (camPos - center).Dot(axis) >= 0.0f ? 1.0f : -1.0f;
	if (std::abs(limitPlus - limitMinus) > kPullSideDiff) side = limitPlus > limitMinus ? 1.0f : -1.0f;
	const float endDist = side > 0.0f ? limitPlus : limitMinus;

	//後ろから見て、高さだけついていきながらだんだん遠ざかる
	cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::EndingCamera);
	if (auto camera = GetCamera()) camera->RiseAlong(center, axis * side, startDist, (std::max)(startDist, endDist), kRiseStartUp, kRiseEndUp, kPullFrame);

	m_phase = Phase::Climb;
	m_frame = 0;
}

float TutorialFinale::CalcPullLimit(const Stage& stage, const Vector3& center, const Vector3& back, float fromDist, float wantDist, float lowY) const
{
	//縦穴の端から引きたい所まで、高さを変えて横に線を当てる(カメラが行く高さは縦穴の上+引き上げる分まで)
	float limit = wantDist;
	for (float y = lowY; y <= m_topY + kRiseEndUp; y += kPullCheckStep)
	{
		Vector3 from = center + back * fromDist;
		Vector3 to = center + back * wantDist;
		from.y = y;
		to.y = y;
		const MV1_COLL_RESULT_POLY hit = stage.CollCheckCameraLine(from, to);
		if (!hit.HitFlag) continue;
		const float hitDist = (Vector3::FromDxLibVector(hit.HitPosition) - center).Dot(back);
		limit = (std::min)(limit, hitDist - kPullWallMargin);
	}
	return limit;
}

std::shared_ptr<EndingCamera> TutorialFinale::GetCamera() const
{
	auto camera = m_cameraManager.lock();
	if (!camera)return nullptr;
	return std::dynamic_pointer_cast<EndingCamera>(camera->GetActiveCamera());
}
