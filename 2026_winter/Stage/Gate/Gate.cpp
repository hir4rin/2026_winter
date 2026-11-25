#include "Gate.h"
#include "Collision/PolygonShape.h"
#include "Collision/CollisionManager.h"
#include "Render/ModelShaderManager.h"
#include "Main/System.h"
#include <algorithm>
#include <cmath>

namespace
{
	const std::string kGateModelDir = "data/Stage/Gates/";
	const std::string kGateBoxModel = "data/Stage/Gates/GateBox.mv1";//1m角の箱
	constexpr float kGateBoxSize = 100.0f;
	constexpr float kColOffRate = 0.3f;//開く途中でこれを超えたら通れる
	constexpr float kCamBoxMinHalf = 1.0f;//カメラ用の箱の半分の大きさがこれより小さければ無し(巻き上げきったシャッターなど)
	constexpr float kCamBoxSameSq = 0.01f;//範囲がこれより動かなければ置き直さない(距離の2乗)
	constexpr float kCamBoxPad = 5.0f;//カメラ用の箱を各方向に広げる量

	//ランプ(自発光が赤いマテリアル)の色
	constexpr float kLampFindRed = 0.5f;//自発光の赤がこれ以上、緑・青がkLampFindOther以下ならランプ
	constexpr float kLampFindOther = 0.2f;
	const COLOR_F kLampClosed = { 1.0f, 0.08f, 0.05f, 1.0f };//閉
	const COLOR_F kLampOpen = { 0.1f, 1.0f, 0.3f, 1.0f };//開
	constexpr float kLampBlinkSpeed = 0.12f;//閉のときの明滅の速さ
	constexpr float kLampBlinkMin = 0.45f;//明滅の暗いとき
}

Gate::~Gate()
{
	if (m_modelHandle != -1) MV1DeleteModel(m_modelHandle);
	if (m_colHandle != -1) MV1DeleteModel(m_colHandle);
	if (m_camHandle != -1) MV1DeleteModel(m_camHandle);
}

void Gate::Init(const GateData& data, const Vector3& offset)
{
	m_data = data;
	m_pos = data.pos + offset;
	//model が空なら当たり判定だけ(塀の横など)
	if (!data.model.empty()) m_modelHandle = MV1LoadModel((kGateModelDir + data.model + ".mv1").c_str());
	//ランプのマテリアルを探す
	for (int i = 0; m_modelHandle != -1 && i < MV1GetMaterialNum(m_modelHandle); ++i)
	{
		const COLOR_F emi = MV1GetMaterialEmiColor(m_modelHandle, i);
		if (emi.r >= kLampFindRed && emi.g <= kLampFindOther && emi.b <= kLampFindOther) m_lampMaterials.push_back(i);
	}
	//見た目の範囲(モデルの座標。カメラ用の箱に使う)
	if (m_modelHandle != -1)
	{
		MV1SetupReferenceMesh(m_modelHandle, -1, FALSE);
		const MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(m_modelHandle, -1, FALSE);
		m_modelMin = Vector3::FromDxLibVector(ref.MinPosition);
		m_modelMax = Vector3::FromDxLibVector(ref.MaxPosition);
		MV1TerminateReferenceMesh(m_modelHandle, -1, FALSE);
	}

	//当たり判定の箱(扉の向きに回して置く。動かさない)
	const bool hasCol = data.colHalf.x > 0.0f;
	if (hasCol)
	{
		m_colHandle = MV1LoadModel(kGateBoxModel.c_str());
		const MATRIX rot = MGetRotY(data.rotYDeg * DX_PI_F / 180.0f);
		const VECTOR center = VAdd(m_pos.ToDxLibVector(), VTransformSR(data.colOffset.ToDxLibVector(), rot));
		const VECTOR scale = VScale(data.colHalf.ToDxLibVector(), 2.0f / kGateBoxSize);
		MV1SetMatrix(m_colHandle, MMult(MMult(MGetScale(scale), rot), MGetTranslate(center)));
		MV1SetupCollInfo(m_colHandle, -1);
		MV1RefreshCollInfo(m_colHandle, -1);
	}

	//最初(フェーズ0)の状態にすぐ合わせる
	m_isOpenTarget = IsOpenPhase(0);
	m_openRate = m_isOpenTarget ? 1.0f : 0.0f;
	ApplyModelMatrix();
	ApplyLampColor();
	//カメラ用の箱
	if (m_modelHandle != -1 || hasCol)
	{
		m_camHandle = MV1LoadModel(kGateBoxModel.c_str());
		MV1SetupCollInfo(m_camHandle, -1);
		UpdateCameraBox();
	}

	if (!hasCol) return;
	ColInit({
		.pos = m_pos,
		.offset = Vector3(),
		.shape = std::make_unique<PolygonShape>(),
		.tag = {Collider::Faction::StaticObject, Collider::ColRole::None},
		.isActive = !m_isOpenTarget,
		});
	if (auto polygon = dynamic_cast<PolygonShape*>(&GetShape()))
	{
		polygon->SetModelHandle(m_colHandle);
		//箱を囲む球。遠い相手はポリゴンを調べない(CollisionChecker::IsFarFromPolygon)
		const MATRIX rot = MGetRotY(data.rotYDeg * DX_PI_F / 180.0f);
		const Vector3 center = Vector3::FromDxLibVector(VAdd(m_pos.ToDxLibVector(), VTransformSR(data.colOffset.ToDxLibVector(), rot)));
		polygon->SetBoundSphere(center, data.colHalf.Magnitude());
	}
}

bool Gate::IsOpenPhase(int phase) const
{
	if (phase < 0) return true;//全部終わった
	return phase < m_data.closePhase || phase >= m_data.openPhase;
}

void Gate::Snap(int currentPhase)
{
	m_isOpenTarget = IsOpenPhase(currentPhase);
	m_openRate = m_isOpenTarget ? 1.0f : 0.0f;
	m_waitFrame = 0.0f;
	Update(currentPhase);
}

void Gate::Update(int currentPhase)
{
	const bool isOpen = IsOpenPhase(currentPhase);
	if (isOpen != m_isOpenTarget)
	{
		m_isOpenTarget = isOpen;
		m_waitFrame = static_cast<float>(m_data.delayFrame);
	}

	const float timeScale = System::GetInstance().GetTimeScale();
	m_lampFrame += timeScale;
	if (m_waitFrame > 0.0f)
	{
		m_waitFrame -= timeScale;
	}
	else
	{
		const float step = timeScale / static_cast<float>((std::max)(m_data.moveFrame, 1));
		m_openRate = std::clamp(m_openRate + (m_isOpenTarget ? step : -step), 0.0f, 1.0f);
	}
	ApplyModelMatrix();
	ApplyLampColor();
	UpdateCameraBox();

	//開きはじめたら通れる、閉まるときは動き出したら通れない
	if (m_colHandle != -1)
	{
		SetIsActive(m_isOpenTarget ? m_openRate < kColOffRate : (m_waitFrame <= 0.0f || m_openRate < 1.0f));
	}
}

void Gate::ApplyModelMatrix()
{
	//ゆっくり始まってゆっくり止まる
	const float t = m_openRate * m_openRate * (3.0f - 2.0f * m_openRate);
	const Vector3 one(1.0f, 1.0f, 1.0f);
	const Vector3 rate = one + (m_data.openScale - one) * t;
	m_curScale = Vector3(m_data.size.x * rate.x, m_data.size.y * rate.y, m_data.size.z * rate.z);
	m_curMove = m_data.openMove * t;
	if (m_modelHandle == -1) return;
	const MATRIX rot = MGetRotY(m_data.rotYDeg * DX_PI_F / 180.0f);
	const VECTOR pos = VAdd(m_pos.ToDxLibVector(), VTransformSR(m_curMove.ToDxLibVector(), rot));
	MV1SetMatrix(m_modelHandle, MMult(MMult(MGetScale(m_curScale.ToDxLibVector()), rot), MGetTranslate(pos)));
}

void Gate::UpdateCameraBox()
{
	if (m_camHandle == -1) return;
	//扉の向き基準(回す前、原点 = m_pos)で範囲を求める
	const bool hasModel = m_modelHandle != -1;
	const bool hasCol = m_colHandle != -1;
	Vector3 vmin, vmax, bmin, bmax;
	if (hasModel)
	{
		//今の大きさ・移動での見た目の範囲(大きさはプラスだけ)
		vmin = Vector3(m_modelMin.x * m_curScale.x, m_modelMin.y * m_curScale.y, m_modelMin.z * m_curScale.z) + m_curMove;
		vmax = Vector3(m_modelMax.x * m_curScale.x, m_modelMax.y * m_curScale.y, m_modelMax.z * m_curScale.z) + m_curMove;
	}
	if (hasCol)
	{
		bmin = m_data.colOffset - m_data.colHalf;
		bmax = m_data.colOffset + m_data.colHalf;
	}
	//見た目と箱があれば重なった部分、片方ならそれ
	Vector3 cmin = hasModel ? vmin : bmin, cmax = hasModel ? vmax : bmax;
	if (hasModel && hasCol)
	{
		cmin = Vector3((std::max)(vmin.x, bmin.x), (std::max)(vmin.y, bmin.y), (std::max)(vmin.z, bmin.z));
		cmax = Vector3((std::min)(vmax.x, bmax.x), (std::min)(vmax.y, bmax.y), (std::min)(vmax.z, bmax.z));
	}
	const Vector3 rawHalf = (cmax - cmin) * 0.5f;
	m_isCamBoxActive = rawHalf.x > kCamBoxMinHalf && rawHalf.y > kCamBoxMinHalf && rawHalf.z > kCamBoxMinHalf;
	if (!m_isCamBoxActive) return;
	//少し広げる(左右2枚の扉の継ぎ目の面の上を線が通ると、どちらにも当たらないため)
	const Vector3 pad(kCamBoxPad, kCamBoxPad, kCamBoxPad);
	cmin = cmin - pad;
	cmax = cmax + pad;
	const Vector3 half = (cmax - cmin) * 0.5f;
	//変わったときだけ置き直す(当たり判定の作り直しがあるため)
	if ((cmin - m_camMin).sqMagnitude() < kCamBoxSameSq && (cmax - m_camMax).sqMagnitude() < kCamBoxSameSq && m_isCamBoxPlaced) return;
	m_camMin = cmin;
	m_camMax = cmax;
	m_isCamBoxPlaced = true;
	const MATRIX rot = MGetRotY(m_data.rotYDeg * DX_PI_F / 180.0f);
	const Vector3 center = (cmin + cmax) * 0.5f;
	const VECTOR world = VAdd(m_pos.ToDxLibVector(), VTransformSR(center.ToDxLibVector(), rot));
	MV1SetMatrix(m_camHandle, MMult(MMult(MGetScale(VScale(half.ToDxLibVector(), 2.0f / kGateBoxSize)), rot), MGetTranslate(world)));
	MV1RefreshCollInfo(m_camHandle, -1);
	m_camCenter = Vector3::FromDxLibVector(world);
	m_camRadius = half.Magnitude();
}

bool Gate::CollCheckCameraLine(const Vector3& start, const Vector3& end, MV1_COLL_RESULT_POLY& hit) const
{
	if (m_camHandle == -1 || !m_isCamBoxActive) return false;
	//線分と囲む球が離れていれば調べない
	const Vector3 seg = end - start;
	const float len2 = seg.sqMagnitude();
	float t = len2 > 0.0f ? std::clamp((m_camCenter - start).Dot(seg) / len2, 0.0f, 1.0f) : 0.0f;
	if ((start + seg * t - m_camCenter).sqMagnitude() > m_camRadius * m_camRadius) return false;
	hit = MV1CollCheck_Line(m_camHandle, -1, start.ToDxLibVector(), end.ToDxLibVector());
	return hit.HitFlag != 0;
}

void Gate::ApplyLampColor()
{
	if (m_lampMaterials.empty()) return;
	//閉は赤く明滅、開くにつれて緑へ
	const float blink = kLampBlinkMin + (1.0f - kLampBlinkMin) * (0.5f + 0.5f * sinf(m_lampFrame * kLampBlinkSpeed));
	const float t = m_openRate;
	const float closedRate = (1.0f - t) * blink;
	const COLOR_F col = GetColorF(
		kLampClosed.r * closedRate + kLampOpen.r * t,
		kLampClosed.g * closedRate + kLampOpen.g * t,
		kLampClosed.b * closedRate + kLampOpen.b * t, 1.0f);
	for (int mat : m_lampMaterials)
	{
		MV1SetMaterialEmiColor(m_modelHandle, mat, col);
	}
}

void Gate::Draw() const
{
	if (m_modelHandle == -1) return;
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);
}

void Gate::DrawCol(unsigned int color) const
{
	if (m_colHandle == -1) return;
	DrawBoxLine(m_data.colOffset, m_data.colHalf, color);
}

void Gate::DrawCameraBox(unsigned int color) const
{
	if (m_camHandle == -1 || !m_isCamBoxActive) return;
	DrawBoxLine((m_camMin + m_camMax) * 0.5f, (m_camMax - m_camMin) * 0.5f, color);
}

void Gate::DrawBoxLine(const Vector3& localCenter, const Vector3& half, unsigned int color) const
{
	const MATRIX rot = MGetRotY(m_data.rotYDeg * DX_PI_F / 180.0f);
	const VECTOR center = VAdd(m_pos.ToDxLibVector(), VTransformSR(localCenter.ToDxLibVector(), rot));
	//8つの角をつないだ線枠
	VECTOR corner[8];
	for (int i = 0; i < 8; ++i)
	{
		const VECTOR local = VGet((i & 1) ? half.x : -half.x, (i & 2) ? half.y : -half.y, (i & 4) ? half.z : -half.z);
		corner[i] = VAdd(center, VTransformSR(local, rot));
	}
	const int edge[12][2] = { {0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7} };
	for (auto& e : edge) DrawLine3D(corner[e[0]], corner[e[1]], color);
}
