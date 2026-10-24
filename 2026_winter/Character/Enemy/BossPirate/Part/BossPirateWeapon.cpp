#include "BossPirateWeapon.h"
#include "DxLib.h"
#include "Main/System.h"
#include "Render/ModelShaderManager.h"

namespace
{
	constexpr const char* kHandFrameName = "hand_r";//剣を持たせるボーンの名前(右手)
	constexpr int kDefaultSlotIndex = 60;//ボーンが見つからなかった時のフレーム番号

	//剣の手元での位置・向き・大きさ
	//Tポーズで刃が前、護拳が腕の向き(指の側)、握りが手のひらに来るように計算した(tools/PirateSword/sword.cpp)
	constexpr float kWeaponOffsetX = -6.12f;//剣のX軸方向のオフセット
	constexpr float kWeaponOffsetY = -3.64f;//剣のY軸方向のオフセット
	constexpr float kWeaponOffsetZ = 0.05f;//剣のZ軸方向のオフセット
	constexpr float kWeaponRotXDeg = -99.73f;//剣のX軸回転(度)
	constexpr float kWeaponRotYDeg = 4.00f;//剣のY軸回転(度)
	constexpr float kWeaponRotZDeg = -92.40f;//剣のZ軸回転(度)
	constexpr float kWeaponScale = 1.0f;//剣のモデルのスケール//ボスと同じ単位で作ってある

	constexpr float kDegToRad = DX_PI_F / 180.0f;
}

BossPirateWeapon::BossPirateWeapon(int ownerModelHandle)
	: m_ownerModelHandle(ownerModelHandle)
	, m_offsetX(kWeaponOffsetX)
	, m_offsetY(kWeaponOffsetY)
	, m_offsetZ(kWeaponOffsetZ)
	, m_rotXDeg(kWeaponRotXDeg)
	, m_rotYDeg(kWeaponRotYDeg)
	, m_rotZDeg(kWeaponRotZDeg)
	, m_scale(kWeaponScale)
{
	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::BossPirateSwordModel));

	//右手のボーンを名前で探す//見つからなければ固定の番号を使う
	m_slotIndex = MV1SearchFrame(m_ownerModelHandle, kHandFrameName);
	if (m_slotIndex < 0)m_slotIndex = kDefaultSlotIndex;
}

BossPirateWeapon::~BossPirateWeapon()
{
	MV1DeleteModel(m_modelHandle);
}

void BossPirateWeapon::Update()
{
	if (m_ownerModelHandle < 0 || m_modelHandle < 0)return;

	//右手のボーンのローカル→ワールド行列を取得
	MATRIX mat = MV1GetFrameLocalWorldMatrix(m_ownerModelHandle, m_slotIndex);

	//手元での向き(回転の合成)
	MATRIX rotmat = MGetRotX(m_rotXDeg * kDegToRad);
	rotmat = MMult(rotmat, MGetRotY(m_rotYDeg * kDegToRad));
	rotmat = MMult(rotmat, MGetRotZ(m_rotZDeg * kDegToRad));

	//手元からのずらし
	MATRIX transmat = MGetTranslate(VGet(m_offsetX, m_offsetY, m_offsetZ));
	rotmat = MMult(rotmat, transmat);

	//スケール→回転・移動→ボーンの行列の順に掛ける
	MATRIX scale = MGetScale(VGet(m_scale, m_scale, m_scale));
	mat = MMult(MMult(scale, rotmat), mat);

	//モデルにマトリクスをセット
	MV1SetMatrix(m_modelHandle, mat);
}

void BossPirateWeapon::Draw()
{
	if (m_modelHandle < 0)return;
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);//血殺中は本体と同じく黒く描く
}

void BossPirateWeapon::SetOpacity(float rate)
{
	MV1SetOpacityRate(m_modelHandle, rate);
}
