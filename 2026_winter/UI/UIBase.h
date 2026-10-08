#pragma once
#include "../Math/Vector3.h"

class Input;

//UIを描く順番
enum class UILayer
{
	Back,//3D描画の途中(プレイヤーより後ろ)に描く
	Front,//3D描画がすべて終わった後(一番手前)に描く
};
//UIの共通点を持ってる基底クラス
class UIBase
{
public:
	//純粋仮想関数
	virtual ~UIBase() = default;
	virtual void Init() = 0;
	virtual void Update(Input& input) = 0;
	virtual void Draw() = 0;
	//外部から表示状態を変更する
	void SetVisible(bool isVisible) { m_isVisible = isVisible; }
	//現在表示すべきかどうかを取得するため
	bool IsVisible()const { return m_isVisible; }
	//どのレイヤーで描くか(基本は一番手前)
	virtual UILayer GetLayer() const { return UILayer::Front; }

protected:
	//UIを表示するかどうか
	bool m_isVisible = false;
	//UIの表示位置
	Vector3 m_pos;
};

