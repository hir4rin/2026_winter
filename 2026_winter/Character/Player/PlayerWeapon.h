#pragma once
#include <memory>
#include <vector>
#include "DxLib.h"
#include "Math/Vector3.h"
#include "Render/SwordLook.h"

class Player;

//プレイヤーの手元に付ける武器(刀)
//毎フレーム、プレイヤーの右手のボーンの行列に合わせて武器のモデルを動かす
class PlayerWeapon
{
public:
	PlayerWeapon(std::weak_ptr<Player> owner);
	virtual ~PlayerWeapon();

	void Update();
	//isSlashing は斬っている間 true(刀身を光らせる)
	void Draw(bool isSlashing = false);

	//必殺技中の刀身の長い刀に持ち替える//falseで通常の刀に戻す
	void SetUltWeapon(bool isUlt) { m_isUltWeapon = isUlt; m_ultBladeRate = 1.0f; }
	//長い刀の刀身の伸び具合(0=通常の刀の長さ、1=全部)
	void SetUltBladeRate(float rate);
	float GetUltBladeRate()const { return m_ultBladeRate; }

	//刀身の根元と先のワールド座標(今の手のボーンから計算する)
	void GetBladePos(Vector3& base, Vector3& tip) const;

private:
	//今持っている刀のモデルのハンドル//長い刀が読めていなければ通常の刀
	int GetCurrentModelHandle() const;
	//今の手のボーンに合わせた刀の行列
	MATRIX CalcWeaponMatrix() const;
	//モデルの一番長い軸の、原点から遠いほうの端(刀の先)
	static VECTOR CalcTipLocal(int modelHandle);
	//長い刀の刀身のメッシュと、伸ばすときの中心(鍔)を調べる
	void SetupUltBlade();
	//刀身を伸ばす行列(刀のモデルの空間)
	MATRIX CalcBladeStretchMatrix() const;
	//今の伸び具合での刀身の倍率
	float GetBladeStretchScale() const;
	//刀身だけ伸ばして描く
	void DrawStretchedUlt();

	VECTOR m_tipLocal = {};//刀の先(刀のモデルの空間)
	VECTOR m_ultTipLocal = {};//長い刀の先

	std::weak_ptr<Player> m_owner;//持ち主のプレイヤー
	int m_modelHandle = -1;//刀のモデルのハンドル
	int m_ultModelHandle = -1;//必殺技中の刀身の長い刀のモデルのハンドル
	SwordLook m_look;//通常の刀の見た目(鋼・刃文・刃先の光)
	SwordLook m_ultLook;//長い刀の見た目(黒い鋼と流れる血の光)
	bool m_isUltWeapon = false;//trueなら長い刀を持つ
	float m_ultBladeRate = 1.0f;//刀身の伸び具合
	std::vector<bool> m_isUltBladeMesh;//長い刀の刀身のメッシュか
	int m_ultTipAxis = 2;//刀の先の軸(0=x 1=y 2=z)
	float m_ultPivot = 0.0f;//伸ばす中心(鍔)の軸上の位置
	float m_ultStartScale = 1.0f;//伸び具合0のときの倍率(通常の刀の長さ)
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
