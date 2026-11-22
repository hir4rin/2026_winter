#include "PlayerWeapon.h"
#include "DxLib.h"
#include <cfloat>
#include <algorithm>
#include "Player.h"
#include "Main/System.h"
#include "Render/ModelShaderManager.h"

namespace
{
	constexpr const char* kHandFrameName = "hand_r";//武器を持たせるボーンの名前(右手)
	constexpr int kDefaultSlotIndex = 58;//ボーンが見つからなかった時のフレーム番号

	//刀の手元での位置・向き・大きさ
	constexpr float kWeaponOffsetX = 0.0f;//武器のX軸方向のオフセット
	constexpr float kWeaponOffsetY = 0.0f;//武器のY軸方向のオフセット
	constexpr float kWeaponOffsetZ = 0.0f;//武器のZ軸方向のオフセット
	constexpr float kWeaponRotXDeg = 90.0f;//武器のX軸回転(度)
	constexpr float kWeaponRotYDeg = 90.0f;//武器のY軸回転(度)
	constexpr float kWeaponRotZDeg = 0.0f;//武器のZ軸回転(度)
	constexpr float kWeaponScale = 0.6f;//武器モデルのスケール

	constexpr float kDegToRad = DX_PI_F / 180.0f;

	constexpr float kBladeBaseRate = 0.25f;//刀身の根元(刀の先までの長さに対する割合。手元から)
}

PlayerWeapon::PlayerWeapon(std::weak_ptr<Player> owner)
	: m_owner(owner)
	, m_offsetX(kWeaponOffsetX)
	, m_offsetY(kWeaponOffsetY)
	, m_offsetZ(kWeaponOffsetZ)
	, m_rotXDeg(kWeaponRotXDeg)
	, m_rotYDeg(kWeaponRotYDeg)
	, m_rotZDeg(kWeaponRotZDeg)
	, m_scale(kWeaponScale)
{
	auto player = m_owner.lock();
	if (!player)return;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerWeaponModel));
	m_ultModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerUltWeaponModel));
	m_look.Load(m_modelHandle);
	m_tipLocal = CalcTipLocal(m_modelHandle);
	m_ultTipLocal = CalcTipLocal(m_ultModelHandle);
	SetupUltBlade();
	m_ultLook.LoadUlt(m_ultModelHandle, m_isUltBladeMesh);

	//右手のボーンを名前で探す//見つからなければ固定の番号を使う
	m_slotIndex = MV1SearchFrame(player->m_modelHandle, kHandFrameName);
	if (m_slotIndex < 0)m_slotIndex = kDefaultSlotIndex;
}

PlayerWeapon::~PlayerWeapon()
{
	MV1DeleteModel(m_modelHandle);
	MV1DeleteModel(m_ultModelHandle);
}

int PlayerWeapon::GetCurrentModelHandle() const
{
	if (m_isUltWeapon && m_ultModelHandle >= 0)return m_ultModelHandle;
	return m_modelHandle;
}

VECTOR PlayerWeapon::CalcTipLocal(int modelHandle)
{
	if (modelHandle < 0) return VGet(0.0f, 0.0f, 0.0f);
	//メッシュの頂点はフレームの空間なので、フレームの行列を掛けてモデルの空間の範囲にする
	MV1SetMatrix(modelHandle, MGetIdent());
	VECTOR minPos = VGet(FLT_MAX, FLT_MAX, FLT_MAX);
	VECTOR maxPos = VGet(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (int frame = 0; frame < MV1GetFrameNum(modelHandle); ++frame)
	{
		if (MV1GetFrameMeshNum(modelHandle, frame) == 0) continue;
		const VECTOR localMin = MV1GetFrameMinVertexLocalPosition(modelHandle, frame);
		const VECTOR localMax = MV1GetFrameMaxVertexLocalPosition(modelHandle, frame);
		const MATRIX frameMat = MV1GetFrameLocalWorldMatrix(modelHandle, frame);
		for (int corner = 0; corner < 8; ++corner)
		{
			const VECTOR local = VGet((corner & 1) ? localMax.x : localMin.x, (corner & 2) ? localMax.y : localMin.y, (corner & 4) ? localMax.z : localMin.z);
			const VECTOR pos = VTransform(local, frameMat);
			minPos = VGet(fminf(minPos.x, pos.x), fminf(minPos.y, pos.y), fminf(minPos.z, pos.z));
			maxPos = VGet(fmaxf(maxPos.x, pos.x), fmaxf(maxPos.y, pos.y), fmaxf(maxPos.z, pos.z));
		}
	}
	if (minPos.x > maxPos.x) return VGet(0.0f, 0.0f, 0.0f);
	const float minArr[3] = { minPos.x, minPos.y, minPos.z };
	const float maxArr[3] = { maxPos.x, maxPos.y, maxPos.z };
	int axis = 0;
	for (int i = 1; i < 3; ++i)
	{
		if (maxArr[i] - minArr[i] > maxArr[axis] - minArr[axis]) axis = i;
	}
	float tip[3] = { 0.0f, 0.0f, 0.0f };
	tip[axis] = (fabsf(maxArr[axis]) > fabsf(minArr[axis])) ? maxArr[axis] : minArr[axis];
	return VGet(tip[0], tip[1], tip[2]);
}

void PlayerWeapon::GetBladePos(Vector3& base, Vector3& tip) const
{
	const int modelHandle = GetCurrentModelHandle();
	if (modelHandle < 0) return;
	VECTOR local = (modelHandle == m_ultModelHandle) ? m_ultTipLocal : m_tipLocal;
	//伸ばしている途中は、伸ばした後の先にする
	if (modelHandle == m_ultModelHandle && m_ultBladeRate < 1.0f) local = VTransform(local, CalcBladeStretchMatrix());
	const MATRIX mat = CalcWeaponMatrix();
	tip = Vector3(VTransform(local, mat));
	base = Vector3(VTransform(VScale(local, kBladeBaseRate), mat));
}

void PlayerWeapon::Update()
{
	auto player = m_owner.lock();
	int modelHandle = GetCurrentModelHandle();
	if (!player || modelHandle < 0)return;

	//モデルにマトリクスをセット
	MV1SetMatrix(modelHandle, CalcWeaponMatrix());
}

MATRIX PlayerWeapon::CalcWeaponMatrix() const
{
	auto player = m_owner.lock();
	if (!player) return MGetIdent();

	//右手のボーンのローカル→ワールド行列を取得
	MATRIX mat = MV1GetFrameLocalWorldMatrix(player->m_modelHandle, m_slotIndex);

	//手元での向き(回転の合成)
	MATRIX rotmat = MGetRotX(m_rotXDeg * kDegToRad);
	rotmat = MMult(rotmat, MGetRotY(m_rotYDeg * kDegToRad + DX_PI_F / 2));
	rotmat = MMult(rotmat, MGetRotZ(m_rotZDeg * kDegToRad - DX_PI_F / 2));

	//手元からのずらし
	MATRIX transmat = MGetTranslate(VGet(m_offsetX, m_offsetY, m_offsetZ));
	rotmat = MMult(rotmat, transmat);

	//スケール→回転・移動→ボーンの行列の順に掛ける
	MATRIX scale = MGetScale(VGet(m_scale, m_scale, m_scale));
	return MMult(MMult(scale, rotmat), mat);
}

void PlayerWeapon::Draw(bool isSlashing)
{
	int modelHandle = GetCurrentModelHandle();
	if (modelHandle < 0)return;
	//通常の刀は専用のシェーダーで描く
	if (modelHandle == m_modelHandle)
	{
		m_look.Update(isSlashing);
		m_look.Draw(modelHandle);
		return;
	}
	//長い刀は、斬っている間と刀身が伸びている間に強く光る
	const bool isGrowing = m_ultBladeRate < 1.0f;
	m_ultLook.Update(isSlashing || isGrowing);
	//刀身を伸ばしている途中
	if (isGrowing)
	{
		DrawStretchedUlt();
		return;
	}
	m_ultLook.Draw(modelHandle);
}

void PlayerWeapon::SetUltBladeRate(float rate)
{
	m_ultBladeRate = std::clamp(rate, 0.0f, 1.0f);
}

void PlayerWeapon::SetupUltBlade()
{
	if (m_ultModelHandle < 0)return;
	MV1SetMatrix(m_ultModelHandle, MGetIdent());

	//刀の先の軸と向き
	const float tipArr[3] = { m_ultTipLocal.x, m_ultTipLocal.y, m_ultTipLocal.z };
	m_ultTipAxis = 0;
	for (int i = 1; i < 3; ++i)
	{
		if (fabsf(tipArr[i]) > fabsf(tipArr[m_ultTipAxis])) m_ultTipAxis = i;
	}
	const float tip = tipArr[m_ultTipAxis];
	if (fabsf(tip) <= 0.0f)return;
	const float sign = (tip > 0.0f) ? 1.0f : -1.0f;

	//刀の先の半分より遠くまで届くメッシュを刀身とする
	const int meshNum = MV1GetMeshNum(m_ultModelHandle);
	m_isUltBladeMesh.assign(meshNum, false);
	bool hasPivot = false;
	for (int frame = 0; frame < MV1GetFrameNum(m_ultModelHandle); ++frame)
	{
		//メッシュの範囲はフレームの空間なので、フレームの行列を掛けてモデルの空間にする
		const MATRIX frameMat = MV1GetFrameLocalWorldMatrix(m_ultModelHandle, frame);
		for (int i = 0; i < MV1GetFrameMeshNum(m_ultModelHandle, frame); ++i)
		{
			const int mesh = MV1GetFrameMesh(m_ultModelHandle, frame, i);
			if (mesh < 0 || mesh >= meshNum)continue;
			const VECTOR localMin = MV1GetMeshMinPosition(m_ultModelHandle, mesh);
			const VECTOR localMax = MV1GetMeshMaxPosition(m_ultModelHandle, mesh);
			float minArr[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
			float maxArr[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
			for (int corner = 0; corner < 8; ++corner)
			{
				const VECTOR local = VGet((corner & 1) ? localMax.x : localMin.x, (corner & 2) ? localMax.y : localMin.y, (corner & 4) ? localMax.z : localMin.z);
				const VECTOR pos = VTransform(local, frameMat);
				const float posArr[3] = { pos.x, pos.y, pos.z };
				for (int axis = 0; axis < 3; ++axis)
				{
					minArr[axis] = fminf(minArr[axis], posArr[axis]);
					maxArr[axis] = fmaxf(maxArr[axis], posArr[axis]);
				}
			}
			const float farEnd = (sign > 0.0f) ? maxArr[m_ultTipAxis] : minArr[m_ultTipAxis];
			const float nearEnd = (sign > 0.0f) ? minArr[m_ultTipAxis] : maxArr[m_ultTipAxis];
			if (farEnd * sign < fabsf(tip) * 0.5f) continue;
			m_isUltBladeMesh[mesh] = true;
			//柄の中まで通っているメッシュは鍔の位置に使わない
			if (nearEnd * sign < 0.0f) continue;
			if (!hasPivot || nearEnd * sign < m_ultPivot * sign) m_ultPivot = nearEnd;
			hasPivot = true;
		}
	}

	//伸び具合0のときに、通常の刀と同じ長さにする
	const float tipArrNormal[3] = { m_tipLocal.x, m_tipLocal.y, m_tipLocal.z };
	const float normalLength = fabsf(tipArrNormal[m_ultTipAxis]) - fabsf(m_ultPivot);
	const float ultLength = fabsf(tip) - fabsf(m_ultPivot);
	if (ultLength > 0.0f) m_ultStartScale = std::clamp(normalLength / ultLength, 0.05f, 1.0f);
}

float PlayerWeapon::GetBladeStretchScale() const
{
	return m_ultStartScale + (1.0f - m_ultStartScale) * m_ultBladeRate;
}

MATRIX PlayerWeapon::CalcBladeStretchMatrix() const
{
	const float scale = GetBladeStretchScale();
	VECTOR pivot = VGet(0.0f, 0.0f, 0.0f);
	VECTOR scaleVec = VGet(1.0f, 1.0f, 1.0f);
	if (m_ultTipAxis == 0) { pivot.x = m_ultPivot; scaleVec.x = scale; }
	if (m_ultTipAxis == 1) { pivot.y = m_ultPivot; scaleVec.y = scale; }
	if (m_ultTipAxis == 2) { pivot.z = m_ultPivot; scaleVec.z = scale; }
	//鍔を中心に、刀の先の軸だけ伸ばす
	return MMult(MMult(MGetTranslate(VScale(pivot, -1.0f)), MGetScale(scaleVec)), MGetTranslate(pivot));
}

void PlayerWeapon::DrawStretchedUlt()
{
	const int meshNum = static_cast<int>(m_isUltBladeMesh.size());
	const MATRIX mat = CalcWeaponMatrix();

	//柄・鍔はそのまま
	for (int mesh = 0; mesh < meshNum; ++mesh) MV1SetMeshVisible(m_ultModelHandle, mesh, m_isUltBladeMesh[mesh] ? FALSE : TRUE);
	MV1SetMatrix(m_ultModelHandle, mat);
	m_ultLook.Draw(m_ultModelHandle);

	//刀身だけ伸ばす
	for (int mesh = 0; mesh < meshNum; ++mesh) MV1SetMeshVisible(m_ultModelHandle, mesh, m_isUltBladeMesh[mesh] ? TRUE : FALSE);
	MV1SetMatrix(m_ultModelHandle, MMult(CalcBladeStretchMatrix(), mat));
	m_ultLook.Draw(m_ultModelHandle);

	//戻す
	for (int mesh = 0; mesh < meshNum; ++mesh) MV1SetMeshVisible(m_ultModelHandle, mesh, TRUE);
	MV1SetMatrix(m_ultModelHandle, mat);
}
