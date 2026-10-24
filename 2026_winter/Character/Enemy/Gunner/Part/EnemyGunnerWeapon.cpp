#include "EnemyGunnerWeapon.h"
#include "DxLib.h"
#include "Character/Enemy/Gunner/EnemyGunner.h"
#include "Main/System.h"
#include "Render/ModelShaderManager.h"

namespace
{
	constexpr const char* kHandFrameName = "mixamorig:RightHand";//武器を持たせるボーンの名前(右手)
	constexpr int kDefaultSlotIndex = 47;//ボーンが見つからなかった時のフレーム番号

	//銃の手元での位置・向き・大きさ//Rifle Idleで銃口が左手の方を向くように合わせた
	constexpr float kWeaponOffsetX = 7.15f;//武器のX軸方向のオフセット
	constexpr float kWeaponOffsetY = 8.54f;//武器のY軸方向のオフセット
	constexpr float kWeaponOffsetZ = -2.64f;//武器のZ軸方向のオフセット
	constexpr float kWeaponRotXDeg = -146.4f;//武器のX軸回転(度)
	constexpr float kWeaponRotYDeg = -65.7f;//武器のY軸回転(度)
	constexpr float kWeaponRotZDeg = 42.1f;//武器のZ軸回転(度)
	constexpr float kWeaponScale = 1.0f;//武器モデルのスケール

	constexpr float kDegToRad = DX_PI_F / 180.0f;
}

EnemyGunnerWeapon::EnemyGunnerWeapon(std::weak_ptr<EnemyGunner> owner)
	: m_owner(owner)
	, m_offsetX(kWeaponOffsetX)
	, m_offsetY(kWeaponOffsetY)
	, m_offsetZ(kWeaponOffsetZ)
	, m_rotXDeg(kWeaponRotXDeg)
	, m_rotYDeg(kWeaponRotYDeg)
	, m_rotZDeg(kWeaponRotZDeg)
	, m_scale(kWeaponScale)
{
	auto gunner = m_owner.lock();
	if (!gunner)return;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyGunnerWeaponModel));

	//右手のボーンを名前で探す//見つからなければ固定の番号を使う
	m_slotIndex = MV1SearchFrame(gunner->m_modelHandle, kHandFrameName);
	if (m_slotIndex < 0)m_slotIndex = kDefaultSlotIndex;
}

EnemyGunnerWeapon::~EnemyGunnerWeapon()
{
	MV1DeleteModel(m_modelHandle);
}

void EnemyGunnerWeapon::Update()
{
	auto gunner = m_owner.lock();
	if (!gunner || m_modelHandle < 0)return;

	//右手のボーンのローカル→ワールド行列を取得
	MATRIX mat = MV1GetFrameLocalWorldMatrix(gunner->m_modelHandle, m_slotIndex);

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

void EnemyGunnerWeapon::Draw()
{
	if (m_modelHandle < 0)return;
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);//血殺中は本体と同じく黒く描く
}

void EnemyGunnerWeapon::SetOpacity(float rate)
{
	MV1SetOpacityRate(m_modelHandle, rate);
}
