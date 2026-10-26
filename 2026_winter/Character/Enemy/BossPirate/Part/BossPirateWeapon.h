#pragma once

//海賊のボスの右手に持たせる剣(カトラス)
//毎フレーム、右手のボーンの行列に合わせて剣のモデルを動かす
//第一ラウンド(BossEnemyPirateR0)・第二ラウンド(BossEnemyPirateR1)の両方で使うので、持ち主はモデルのハンドルで受け取る
class BossPirateWeapon
{
public:
	BossPirateWeapon(int ownerModelHandle);
	virtual ~BossPirateWeapon();

	void Update();
	void Draw();

	void SetOpacity(float rate);//本体と一緒に消えるように不透明度を設定する

private:
	int m_ownerModelHandle = -1;//持ち主のモデル
	int m_modelHandle = -1;//剣のモデルのハンドル
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
