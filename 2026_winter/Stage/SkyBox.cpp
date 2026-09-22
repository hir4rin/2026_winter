#include "SkyBox.h"
#include "../Camera/CameraManager.h"
#include "../Camera/CameraState/CameraStateBase.h"
namespace
{
	VERTEX3D MakeV(Vector3 pos, float u, float v)
	{
		VERTEX3D vtx;
		vtx.pos = pos.ToDxLibVector();
		vtx.norm = VGet(0.0f, 0.0f, 0.0f);//法線は使わないので0で初期化
		vtx.dif = GetColorU8(255, 255, 255,255);//ディフューズカラーは白で初期化
		vtx.spc = GetColorU8(0, 0, 0, 255);//スペキュラカラーは黒で初期化
		vtx.u = u; vtx.v = v;//テクスチャ座標
		vtx.su = 0.0f; vtx.sv = 0.0f;//補助テクスチャ座標は使わないので0で初期化
		return vtx;
	}
}



SkyBox::SkyBox()
{
	for (int i = 0; i < kFaceNum; i++)
   {
	   m_textureHandle[i] = -1;//テクスチャハンドルを-1で初期化
   }
}

SkyBox::~SkyBox()
{
	for (int i = 0; i < kFaceNum; i++)
	{
		if (m_textureHandle[i] != -1)
			DeleteGraph(m_textureHandle[i]);
	}
}

void SkyBox::Init(std::weak_ptr<CameraManager> cameraMgr)
{
	m_textureHandle[kFront] = LoadGraph("data/SkyBox/front.png");
	m_textureHandle[kBack] = LoadGraph("data/SkyBox/back.png");
	m_textureHandle[kLeft] = LoadGraph("data/SkyBox/left.png");
	m_textureHandle[kRight] = LoadGraph("data/SkyBox/right.png");
	m_textureHandle[kUp] = LoadGraph("data/SkyBox/up.png");
	m_textureHandle[kDown] = LoadGraph("data/SkyBox/down.png");
	// ロード確認（-1 なら読み込み失敗）
	for (int i = 0; i < kFaceNum; i++)
	{
		if (m_textureHandle[i] == -1)
		{
			printfDx("SkyBox texture %d の読み込みに失敗しました\n", i);
		}
	}
	m_cameraManager = cameraMgr;
	Update();
}

void SkyBox::Update()
{
	auto camera = m_cameraManager.lock()->GetActiveCamera();
	if (!camera)return;

	Vector3 cameraPos = camera->GetPos();
	const float size = kSize;
	const float x = cameraPos.x;
	const float y = cameraPos.y;
	const float z = cameraPos.z;

	//前面：内側から見た左上、右上右下、左下の順番で頂点を設定
	SetFaceVertices(kFront, Vector3(x - size, y + size, z + size), Vector3(x + size, y + size, z + size),
		Vector3(x + size, y - size, z + size), Vector3(x - size, y - size, z + size));
	//後面
	SetFaceVertices(kBack, Vector3(x + size, y + size, z - size), Vector3(x - size, y + size, z - size),
		Vector3(x - size, y - size, z - size), Vector3(x + size, y - size, z - size));
	//右面
	SetFaceVertices(kRight, Vector3(x + size, y + size, z + size), Vector3(x + size, y + size, z - size),
		Vector3(x + size, y - size, z - size), Vector3(x + size, y - size, z + size));
	//左面
	SetFaceVertices(kLeft, Vector3(x - size, y + size, z - size), Vector3(x - size, y + size, z + size),
		Vector3(x - size, y - size, z + size), Vector3(x - size, y - size, z - size));
	//上面
	SetFaceVertices(kUp, Vector3(x - size, y + size, z - size), Vector3(x + size, y + size, z - size),
		Vector3(x + size, y + size, z + size), Vector3(x - size, y + size, z + size));
	//下面
	SetFaceVertices(kDown, Vector3(x - size, y - size, z + size), Vector3(x + size, y - size, z + size),
		Vector3(x + size, y - size, z - size), Vector3(x - size, y - size, z - size));
}

void SkyBox::Draw()
{
	SetUseLighting(FALSE);           // ライティングを無効化
	SetUseZBuffer3D(false);
	for (int i = 0; i < kFaceNum; i++)
	{
		DrawPolygon3D(m_vertex[i], 2, m_textureHandle[i], TRUE);
	}
	SetUseZBuffer3D(true);
	SetUseLighting(TRUE);            // ライティングを元に戻す
}

void SkyBox::SetFaceVertices(int face, Vector3 topleft, Vector3 topright, Vector3 bottomright, Vector3 bottomleft)
{
	m_vertex[face][0] = MakeV(topleft, 0.0f, 0.0f);
	m_vertex[face][1] = MakeV(topright, 1.0f, 0.0f);
	m_vertex[face][2] = MakeV(bottomright, 1.0f, 1.0f);
	m_vertex[face][3] = MakeV(topleft, 0.0f, 0.0f);
	m_vertex[face][4] = MakeV(bottomright, 1.0f, 1.0f);
	m_vertex[face][5] = MakeV(bottomleft, 0.0f, 1.0f);
}
