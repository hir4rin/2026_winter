#include "PlayerWeapon.h"
#include "DxLib.h"
#include "Base/Player.h"
#include "../../System.h"

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

void PlayerWeapon::Update()
{
	auto player = m_owner.lock();
	int modelHandle = GetCurrentModelHandle();
	if (!player || modelHandle < 0)return;

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
	mat = MMult(MMult(scale, rotmat), mat);

	//モデルにマトリクスをセット
	MV1SetMatrix(modelHandle, mat);
}

void PlayerWeapon::Draw()
{
	int modelHandle = GetCurrentModelHandle();
	if (modelHandle < 0)return;
	MV1DrawModel(modelHandle);
}
