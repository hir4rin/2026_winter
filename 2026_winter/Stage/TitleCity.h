#pragma once
#include "DxLib.h"
#include "StageLights.h"
#include <memory>
#include <vector>

class SkyBox;

//タイトルの背景//ステージ1の街を高い所から見下ろした絵を、別の画面に描いてぼかして貼る
//ステージの明かりは周りを照らすだけで遠くからは光って見えないので、明かりの位置に光の玉を置いてにじませる
class TitleCity
{
public:
	TitleCity();
	~TitleCity();

	void Init();
	//今のカメラ(タイトルカメラ)は描いた後に戻す
	void Draw();

private:
	//カメラの設定を写して戻すため
	struct CameraState
	{
		VECTOR pos;
		VECTOR target;
		float fov;
		float nearZ;
		float farZ;
	};
	//光の玉1つ(明かりの CSV から)
	struct Glow
	{
		VECTOR pos;
		int r, g, b;
		float size;
		float alpha;
		float phase;//瞬きのずれ
	};

	void LoadGlows(float offsetY);
	//ビルの壁にランダムに窓の明かりを置く
	void AddWindowGlows();
	void MakeGlowGraph();
	void DrawCity(const VECTOR& target);
	void DrawGlows();

	int m_modelHandle = -1;
	std::unique_ptr<SkyBox> m_skyBox;
	StageLights m_lights;
	std::vector<Glow> m_glows;
	int m_glowGraph = -1;//ふちのぼけた白い丸
	int m_screen = -1;//街を描く画面(小さめにしてぼかす)
	int m_bloom = -1;//明るい所だけ取り出してにじませる画面
	int m_frame = 0;
};
