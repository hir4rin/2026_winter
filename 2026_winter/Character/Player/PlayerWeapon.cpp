#include "PlayerWeapon.h"
#include "DxLib.h"
#include <cfloat>
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
	m_tipLocal = CalcTipLocal(m_modelHandle);
	m_ultTipLocal = CalcTipLocal(m_ultModelHandle);

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
	const VECTOR local = (modelHandle == m_ultModelHandle) ? m_ultTipLocal : m_tipLocal;
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

void PlayerWeapon::Draw()
{
	int modelHandle = GetCurrentModelHandle();
	if (modelHandle < 0)return;
	ModelShaderManager::GetInstance().DrawModel(modelHandle);//ステージの明かりの光も足す
}
