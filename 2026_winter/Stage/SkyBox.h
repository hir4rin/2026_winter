#pragma once
#include "DxLib.h"
#include "../Math/Vector3.h"
#include <memory>

class CameraManager;
class SkyBox
{
public:
	enum Face
	{
		kFront = 0,
		kBack = 1,
		kLeft = 2,
		kRight = 3,
		kUp = 4,
		kDown = 5,
		kFaceNum = 6
	};
	SkyBox();
	virtual ~SkyBox();
	void Init(std::weak_ptr<CameraManager> cameraMgr);
	void Update();
	void Draw();
private:
	static constexpr int kVertexPerFace = 6;//2ポリゴンx3頂点
	static constexpr float kSize = 1200.0f;//空の大きさ//ボックスの半径

	VERTEX3D m_vertex[kFaceNum][kVertexPerFace];//各面の頂点データ
	int m_textureHandle[kFaceNum];//各面のテクスチャハンドル
	std::weak_ptr<CameraManager> m_cameraManager;//カメラの弱参照

	/// <summary>
	/// 1面分（クワッド）の頂点を設定
	/// </summary>
	/// <param name="face"></param>
	/// <param name="topleft">左上</param>
	/// <param name="topright">右上</param>
	/// <param name="bottomright">左下</param>
	/// <param name="bottomleft">右下</param>
	void SetFaceVertices(int face, Vector3 topleft, Vector3 topright, Vector3 bottomright, Vector3 bottomleft);



};

