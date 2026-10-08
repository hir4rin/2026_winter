#pragma once
#include <vector>
#include <memory>
#include "UIBase.h"

class CameraManager;
//UIをまとめて管理するクラス
//継承はせずにUIbaseのインスタンスとして保持する
class UIManager
{
public:
	UIManager() = default;
	~UIManager() = default;

	//場面ごとの初期化
	//使うUIを生成して登録し、各UIのInitで画像などのハンドルを生成する
	void TitleInit();
	void InGameInit(std::weak_ptr<CameraManager> cameraManager);

	void Add(std::shared_ptr<UIBase> ui);
	//登録しているUIをすべて外す(ハンドルは各UIのデストラクタで解放する)
	void Clear();
	void Update(Input& input);
	//3D描画の途中(プレイヤーを描く前)に呼ぶ//UILayer::BackのUIを描く
	void DrawBack();
	//3D描画がすべて終わった後に呼ぶ//UILayer::FrontのUIを描く
	void Draw();
private:
	//指定したレイヤーのUIだけを描く
	void DrawLayer(UILayer layer);

	//登録されたUIのInitをまとめて呼ぶ
	void InitAll();

	//管理しているUI
	std::vector<std::shared_ptr<UIBase>> m_uiList;
};
