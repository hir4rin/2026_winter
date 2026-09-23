#pragma once
//#include "DxLib.h"
#include "EffekseerForDXLib.h"
class SceneController; // シーンコントローラーのプロトタイプ宣言


/// <summary>
/// シーン基底クラス
/// </summary>
class Scene
{
protected:
	SceneController& m_controller;
public:
	Scene(SceneController& controller);
	/// <summary>
	/// シーンの情報の更新
	/// </summary>
	virtual void Update() = 0;
	//メンバ関数ポインタで管理
	virtual void FadeInUpdate() = 0;
	virtual void NormalUpdate() = 0;
	virtual void FadeOutUpdate() = 0;
	using UpdateFunc_t = void(Scene::*)();
	/// <summary>
	/// コンストラクタで継承先のポインタにキャストして代入する
	/// </summary>
	UpdateFunc_t m_updateFunc = nullptr;

	/// <summary>
	/// シーンの描画(シーンの持ち物も描画する)
	/// </summary>
	virtual void Draw() = 0;
	//メンバ関数ポインタで管理
	virtual void FadeInDraw() = 0;
	virtual void NormalDraw() = 0;
	virtual void FadeOutDraw() = 0;
	using DrawFunc_t = void(Scene::*)();
	/// <summary>
	/// コンストラクタで継承先のポインタにキャストして代入する
	/// </summary>
	DrawFunc_t m_drawFunc = nullptr;

};

