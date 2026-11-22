#include "EnemyBazookaWeapon.h"
#include "DxLib.h"
#include "Character/Enemy/Bazooka/EnemyBazooka.h"
#include "Main/System.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>

namespace
{
	constexpr const char* kHandFrameName = "mixamorig:RightHand";//武器を持たせるボーンの名前(右手)
	constexpr int kDefaultSlotIndex = 47;//ボーンが見つからなかった時のフレーム番号

	//RPG7の手元での位置・向き・大きさ//ガンナーと同じ値(RPG7のモデルもAK74と同じく原点=引き金、+Zが前、+Yが上にしてある)
	constexpr float kWeaponOffsetX = 7.15f;//武器のX軸方向のオフセット
	constexpr float kWeaponOffsetY = 8.54f;//武器のY軸方向のオフセット
	constexpr float kWeaponOffsetZ = -2.64f;//武器のZ軸方向のオフセット
	constexpr float kWeaponRotXDeg = -146.4f;//武器のX軸回転(度)
	constexpr float kWeaponRotYDeg = -65.7f;//武器のY軸回転(度)
	constexpr float kWeaponRotZDeg = 42.1f;//武器のZ軸回転(度)
	constexpr float kWeaponScale = 1.0f;//武器モデルのスケール

	constexpr float kDegToRad = DX_PI_F / 180.0f;
}

EnemyBazookaWeapon::EnemyBazookaWeapon(std::weak_ptr<EnemyBazooka> owner)
	: m_owner(owner)
	, m_offsetX(kWeaponOffsetX)
	, m_offsetY(kWeaponOffsetY)
	, m_offsetZ(kWeaponOffsetZ)
	, m_rotXDeg(kWeaponRotXDeg)
	, m_rotYDeg(kWeaponRotYDeg)
	, m_rotZDeg(kWeaponRotZDeg)
	, m_scale(kWeaponScale)
{
	auto bazooka = m_owner.lock();
	if (!bazooka)return;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyBazookaWeaponModel));

	//右手のボーンを名前で探す//見つからなければ固定の番号を使う
	m_slotIndex = MV1SearchFrame(bazooka->m_modelHandle, kHandFrameName);
	if (m_slotIndex < 0)m_slotIndex = kDefaultSlotIndex;

	// TODO(手順5) 弾頭の先の位置(モデルの空間)を m_muzzleLocalPos に入れる
	//   考え方:
	//   - RPG7.mv1 は原点が引き金、+Z が前(弾頭)、+Y が上にしてある(作業メモの 10/18)。
	//     なので「一番前(Zが一番大きい所)」が弾頭の先。
	//   - モデルの空間で1回だけ求めておき、撃つたびに今の行列でワールドに直す(GetMuzzlePos)。
	//     手に持って動くので、ワールドの位置は毎フレーム変わるが、モデルの中での位置は変わらない。
	//   - 求め方: 全部のメッシュの範囲を MV1GetMeshMinPosition / MV1GetMeshMaxPosition で取って合わせる
	//       (MV1GetMeshNum でメッシュの数。min・max は x・y・z ごとに (std::min) / (std::max))。
	//       m_muzzleLocalPos = Vector3((min.x + max.x) / 2, (min.y + max.y) / 2, max.z)
	//       X・Y は真ん中、Z は一番前。
	//   - 数字を手で決めて定数にしてもよい(画面で見てずれていたら、定数のほうが直しやすい)。
}

EnemyBazookaWeapon::~EnemyBazookaWeapon()
{
	MV1DeleteModel(m_modelHandle);
}

void EnemyBazookaWeapon::Update()
{
	auto bazooka = m_owner.lock();
	if (!bazooka || m_modelHandle < 0)return;

	//右手のボーンのローカル→ワールド行列を取得
	MATRIX mat = MV1GetFrameLocalWorldMatrix(bazooka->m_modelHandle, m_slotIndex);

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

void EnemyBazookaWeapon::Draw()
{
	if (m_modelHandle < 0)return;
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);//血殺中は本体と同じく黒く描く
}

void EnemyBazookaWeapon::SetOpacity(float rate)
{
	MV1SetOpacityRate(m_modelHandle, rate);
}

Vector3 EnemyBazookaWeapon::GetMuzzlePos() const
{
	// TODO(手順5) m_muzzleLocalPos をワールドの座標に直して返す
	//   書き方: Vector3::FromDxLibVector(VTransform(m_muzzleLocalPos.ToDxLibVector(), MV1GetMatrix(m_modelHandle)))
	//   - MV1GetMatrix は Update で MV1SetMatrix した行列(右手のボーン × ずらし × 大きさ)。
	//     VTransform でモデルの空間の点にこの行列を掛けると、ワールドでの位置になる。
	//   - m_modelHandle が -1 のときは Vector3() を返す。
	return Vector3();//仮
}
