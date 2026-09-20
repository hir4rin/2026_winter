#pragma once
#include "DxLib.h"

class SceneMain
{
public:
	SceneMain();
	~SceneMain();

	void Init();
	void Update();
	void Draw();
private:
	void DrawGrid();
	void DrawModelDebugInfo();

	//十字キーの入力から移動方向を求める(入力なしなら長さ0)
	VECTOR GetMoveInput() const;
	//モデルの位置・向きを更新する
	void UpdateMove();
	//アニメーションをブレンドしながら切り替える(すでに再生中の名前なら何もしない)
	void ChangeAnim(const char* animName, bool isLoop = true);
	//Zキーで前転/後転を始める
	void UpdateRollInput();
	//再生中のアニメーションの時間・ブレンド率を更新する
	void UpdateAnim();
	//カメラをモデルに追従させる
	void UpdateCamera();

private:
	int m_frameCount;

	int m_modelHandle = -1;
	int m_lightHandle = -1;

	VECTOR m_pos = { 0.0f, 0.0f, 0.0f };
	float m_rotY = 0.0f;

	//アニメーション(現在 + ブレンド元)
	int m_animHandle = -1;
	float m_animCount = 0.0f;
	float m_animTotal = 0.0f;
	int m_prevAnimHandle = -1;
	float m_prevAnimCount = 0.0f;
	float m_prevAnimTotal = 0.0f;
	float m_blendRate = 1.0f;

	bool m_isAnimLoop = true;
	bool m_isRolling = false;//前転/後転の再生中(入力を受け付けない)
	bool m_isRollBack = false;//再生中の転がりが後転かどうか
	bool m_wasZPressed = false;

	const char* m_currentAnimName = nullptr;
	bool m_isAnimMissing = false;//data/PlayerAnim.xlsxの名前がmv1に見つからなかったとき
};
