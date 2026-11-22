#pragma once
#include <memory>
#include "Math/Vector3.h"

class EnemyBazooka;

//バズーカ兵の手元に付ける武器(RPG7)
//毎フレーム、バズーカ兵の右手のボーンの行列に合わせて武器のモデルを動かす
class EnemyBazookaWeapon
{
public:
	EnemyBazookaWeapon(std::weak_ptr<EnemyBazooka> owner);
	virtual ~EnemyBazookaWeapon();

	void Update();
	void Draw();

	void SetOpacity(float rate);//本体と一緒に消えるように不透明度を設定する
	Vector3 GetMuzzlePos()const;//弾頭の先のワールド座標//ミサイルの発射位置

private:
	Vector3 m_muzzleLocalPos;//モデルの空間での弾頭の先//メッシュの範囲から求める

	std::weak_ptr<EnemyBazooka> m_owner;//持ち主のバズーカ兵
	int m_modelHandle = -1;//RPG7のモデルのハンドル
	int m_slotIndex = -1;//装備しているボーン(右手)のフレーム番号

	int m_muzzleFrame = -1;//弾頭の先のフレーム番号

	//手元からのずれ
	float m_offsetX = 0.0f;//右手のボーンから見たX軸方向のオフセット
	float m_offsetY = 0.0f;//右手のボーンから見たY軸方向のオフセット
	float m_offsetZ = 0.0f;//右手のボーンから見たZ軸方向のオフセット
	float m_rotXDeg = 0.0f;//X軸回転(度)
	float m_rotYDeg = 0.0f;//Y軸回転(度)
	float m_rotZDeg = 0.0f;//Z軸回転(度)
	float m_scale = 1.0f;//モデルのスケール
};
