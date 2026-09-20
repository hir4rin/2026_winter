#include "Weapon.h"
#include "DxLib.h"
#include "Base/Player.h"
#include "../Math/Vector3.h"
#include "../Math/Matrix4x4.h"
#include "../System.h"

namespace
{
	constexpr float kWeaponOffsetX = 45.0f;//武器のX軸方向のオフセット
	constexpr float kWeaponScale = 0.3f;//武器モデルのスケール

	constexpr float kTitleWeaponOffsetY = 50.0f;//タイトル演出時の武器のY軸方向のオフセット
	constexpr float kTitleWeaponZ = -5000.0f;//タイトル演出時の武器のZ座標
}

Weapon::Weapon(std::weak_ptr<Player> owner)
	: m_owner(owner)
{
	auto player = m_owner.lock();
	if (!player)return;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerWeaponModel));
}

Weapon::~Weapon()
{
	MV1DeleteModel(m_modelHandle);
}

void Weapon::Update()
{
	auto player = m_owner.lock();
	if (!player)return;
	//モデルフレームのローカルワールド行列を取得
	//プレイヤーの状態によってモデルを変える
	int modelHandle = -1;
	int currentModelHandle = player->m_anim.GetModelHandleForCheck();
	if (currentModelHandle == player->m_modelHandle)
	{
		modelHandle = player->m_modelHandle;
	}
	else
	{
		modelHandle = player->m_attackModelHandle;
	}
	

	MATRIX mat = MV1GetFrameLocalWorldMatrix(modelHandle, slotIndex);//モデルフレームのローカルワールド行列を取得

	////武器の位置を取得
	//Vector3 weaponPos = MV1GetFramePosition(m_ownerHandle, slotIndex);

	//剣先を取得する簡単なやり方があるらしい

	//MATRIX transmat = MGetTranslate(weaponPos.ToDxLibVector());
	//90度回転させる
	MATRIX rotmat = MGetRotY(DX_PI_F / 2);//回転行列を作成する//90度回転させる
	MATRIX rotXmat = MGetRotX(DX_PI_F / 2);//回転行列を作成する//90度回転させる
	//回転の合成
	rotmat = MMult(rotXmat, rotmat);//回転行列を掛ける//90度回転させる

	//下方向に移動
	MATRIX transmat = MGetTranslate(VGet(kWeaponOffsetX, 0.0f, 0.0f));//移動行列を作成する//下方向に移動
	rotmat = MMult(transmat, rotmat);//移動行列を掛ける//下方向に移動
	mat = MMult(rotmat, mat);//回転行列を掛ける//90度回転させる


	MATRIX scale = MGetScale(VGet(kWeaponScale, kWeaponScale, kWeaponScale));//スケーリング行列を作成する//モデルの大きさを半分にする

	mat = MMult(scale, mat);//スケーリング行列を掛ける//モデルの大きさを半分にする

	//モデルにマトリクスをセット
	MV1SetMatrix(m_modelHandle, mat);
	////一旦0で初期化して確認
	//Vector3 zero = Vector3(0.0f, 0.0f, 0.0f);
	//MV1SetPosition(m_modelHandle, zero.ToDxLibVector());

}

void Weapon::TitleUpdate()
{

	//剣が突き刺さっているだけ
	auto player = m_owner.lock();
	if (!player)return;
	//MATRIX transmat = MGetTranslate(weaponPos.ToDxLibVector());
	
	MATRIX mat = MGetIdent();//単位行列を取得
	//90度回転させる
	MATRIX rotmat = MGetRotY(DX_PI_F / 2);//回転行列を作成する//90度回転させる
	MATRIX rotXmat = MGetRotX(DX_PI_F);//回転行列を作成する//90度回転させる
	MATRIX rotZmat = MGetRotZ(DX_PI_F / 2);//回転行列を作成する//90度回転させる
	//回転の合成
	rotmat = MMult(rotXmat, rotmat);//回転行列を掛ける//90度回転させる
	rotmat = MMult(rotZmat, rotmat);//回転行列を掛ける//90度回転させる
	//下方向に移動
	MATRIX transmat = MGetTranslate(VGet(0.0f, kTitleWeaponOffsetY, kTitleWeaponZ));//移動行列を作成する//下方向に移動
	rotmat = MMult(rotmat, transmat);//移動行列を掛ける//下方向に移動
	mat = MMult(rotmat, mat);//回転行列を掛ける//90度回転させる


	MATRIX scale = MGetScale(VGet(kWeaponScale, kWeaponScale, kWeaponScale));//スケーリング行列を作成する//モデルの大きさを半分にする

	mat = MMult(scale, mat);//スケーリング行列を掛ける//モデルの大きさを半分にする

	//モデルにマトリクスをセット
	MV1SetMatrix(m_modelHandle, mat);
}



void Weapon::Draw()
{
	//auto player = m_owner.lock();
	//if (!player)return;
	//// 実際の位置を取得して表示
	//VECTOR pos = MV1GetPosition(m_modelHandle);
	////タイトル時
	//if (player->m_isTitleMode)
	//{
	//	if (player->GetPos().z <= kTitleWeaponZ)
	//	{
	//		MV1DrawModel(m_modelHandle);
	//	}
	//}
	////通常時
	//else
	//{
	//	MV1DrawModel(m_modelHandle);

	//}
	//

	
}

void Weapon::TitleDraw()
{
	//auto player = m_owner.lock();
	//if (!player)return;

	//if (player->GetPos().z >= kTitleWeaponZ)
	//{
	//	MV1DrawModel(m_modelHandle);
	//}
}
