#pragma once
#include <memory>

class EnemyGunner;

//ガンナーの手元に付ける武器(銃)
//毎フレーム、ガンナーの右手のボーンの行列に合わせて武器のモデルを動かす
class EnemyGunnerWeapon
{
public:
	EnemyGunnerWeapon(std::weak_ptr<EnemyGunner> owner);
	virtual ~EnemyGunnerWeapon();

	void Update();
	void Draw();

	void SetOpacity(float rate);//本体と一緒に消えるように不透明度を設定する

private:
	std::weak_ptr<EnemyGunner> m_owner;//持ち主のガンナー
	int m_modelHandle = -1;//銃のモデルのハンドル
	int m_slotIndex = -1;//装備しているボーン(右手)のフレーム番号

	//手元からのずれ
	float m_offsetX = 0.0f;//右手のボーンから見たX軸方向のオフセット
	float m_offsetY = 0.0f;//右手のボーンから見たY軸方向のオフセット
	float m_offsetZ = 0.0f;//右手のボーンから見たZ軸方向のオフセット
	float m_rotXDeg = 0.0f;//X軸回転(度)
	float m_rotYDeg = 0.0f;//Y軸回転(度)
	float m_rotZDeg = 0.0f;//Z軸回転(度)
	float m_scale = 1.0f;//モデルのスケール
};
