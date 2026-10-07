#pragma once
#include <memory>

class Player;

//プレイヤーの手元に付ける武器(刀)
//毎フレーム、プレイヤーの右手のボーンの行列に合わせて武器のモデルを動かす
class PlayerWeapon
{
public:
	PlayerWeapon(std::weak_ptr<Player> owner);
	virtual ~PlayerWeapon();

	void Update();
	void Draw();

	//必殺技中の刀身の長い刀に持ち替える//falseで通常の刀に戻す
	void SetUltWeapon(bool isUlt) { m_isUltWeapon = isUlt; }

private:
	//今持っている刀のモデルのハンドル//長い刀が読めていなければ通常の刀
	int GetCurrentModelHandle() const;

	std::weak_ptr<Player> m_owner;//持ち主のプレイヤー
	int m_modelHandle = -1;//刀のモデルのハンドル
	int m_ultModelHandle = -1;//必殺技中の刀身の長い刀のモデルのハンドル
	bool m_isUltWeapon = false;//trueなら長い刀を持つ
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
