#include "ModelShaderManager.h"
#include <algorithm>
#include <cassert>

#pragma comment(lib, "d3dcompiler")//D3DCompileFromFileを使うため

namespace
{
	const std::wstring kModelVertexShaderPath = L"Render/Shader/MV1VertexShader.hlsl";
	const std::wstring kBlackPixelShaderPath = L"Render/Shader/EnemyBlackPS.hlsl";
	const std::wstring kLitPixelShaderPath = L"Render/Shader/LitModelPS.hlsl";
	const std::wstring kCutPixelShaderPath = L"Render/Shader/EnemyCutPS.hlsl";
	const std::wstring kLookPixelShaderPath = L"Render/Shader/CharacterLookPS.hlsl";
	const char* const kShaderEntryPoint = "main";
	const char* const kVertexShaderTarget = "vs_5_0";
	const char* const kPixelShaderTarget = "ps_5_0";
}

void ModelShaderManager::Init()
{
	if (m_isInit)return;

	//頂点タイプごとにマクロを変えてコンパイルする(マクロの最後は{nullptr,nullptr})
	//FREE_FRAME(9ボーン以上)はシェーダーを用意していないので、DxLibの標準シェーダーで描く
	m_vertexShaders.fill(-1);
	//スキニングなし・ノーマルマップなし
	m_vertexShaders[DX_MV1_VERTEX_TYPE_1FRAME] = LoadShaderWithMacro(kModelVertexShaderPath, kVertexShaderTarget,
		{ {nullptr,nullptr} }, true);
	//4ボーンスキニング
	m_vertexShaders[DX_MV1_VERTEX_TYPE_4FRAME] = LoadShaderWithMacro(kModelVertexShaderPath, kVertexShaderTarget,
		{ {"SKINMESH",""}, {nullptr,nullptr} }, true);
	//8ボーンスキニング
	m_vertexShaders[DX_MV1_VERTEX_TYPE_8FRAME] = LoadShaderWithMacro(kModelVertexShaderPath, kVertexShaderTarget,
		{ {"SKINMESH",""}, {"BONE8",""}, {nullptr,nullptr} }, true);
	//ノーマルマップあり
	m_vertexShaders[DX_MV1_VERTEX_TYPE_NMAP_1FRAME] = LoadShaderWithMacro(kModelVertexShaderPath, kVertexShaderTarget,
		{ {"BUMPMAP",""}, {nullptr,nullptr} }, true);
	//ノーマルマップあり・4ボーンスキニング
	m_vertexShaders[DX_MV1_VERTEX_TYPE_NMAP_4FRAME] = LoadShaderWithMacro(kModelVertexShaderPath, kVertexShaderTarget,
		{ {"SKINMESH",""}, {"BUMPMAP",""}, {nullptr,nullptr} }, true);
	//ノーマルマップあり・8ボーンスキニング
	m_vertexShaders[DX_MV1_VERTEX_TYPE_NMAP_8FRAME] = LoadShaderWithMacro(kModelVertexShaderPath, kVertexShaderTarget,
		{ {"SKINMESH",""}, {"BONE8",""}, {"BUMPMAP",""}, {nullptr,nullptr} }, true);

	m_blackPixelShader = LoadShaderWithMacro(kBlackPixelShaderPath, kPixelShaderTarget, { {nullptr,nullptr} }, false);
	//明かり用はノーマルマップのあり/なしで VS_OUTPUT の並びが違うので2つ
	m_litPixelShader = LoadShaderWithMacro(kLitPixelShaderPath, kPixelShaderTarget, { {nullptr,nullptr} }, false);
	m_litBumpPixelShader = LoadShaderWithMacro(kLitPixelShaderPath, kPixelShaderTarget, { {"BUMPMAP",""}, {nullptr,nullptr} }, false);
	m_lightConstantBuffer = CreateShaderConstantBuffer(sizeof(StageLightConstant));
	m_colorGradeBuffer = CreateShaderConstantBuffer(sizeof(ColorGradeConstant));
	m_guardRimBuffer = CreateShaderConstantBuffer(sizeof(GuardRimConstant));
	m_cutPixelShader = LoadShaderWithMacro(kCutPixelShaderPath, kPixelShaderTarget, { {nullptr,nullptr} }, false);
	m_cutBumpPixelShader = LoadShaderWithMacro(kCutPixelShaderPath, kPixelShaderTarget, { {"BUMPMAP",""}, {nullptr,nullptr} }, false);
	m_cutConstantBuffer = CreateShaderConstantBuffer(sizeof(CutConstant));
	m_lookPixelShader = LoadShaderWithMacro(kLookPixelShaderPath, kPixelShaderTarget, { {nullptr,nullptr} }, false);
	m_lookBumpPixelShader = LoadShaderWithMacro(kLookPixelShaderPath, kPixelShaderTarget, { {"BUMPMAP",""}, {nullptr,nullptr} }, false);

	m_isInit = true;
	SetStageLights({}, 0.0f, 0.0f, 0.0f);//明かりの数を0で入れておく(明かりの無いステージでも CharacterLook が b4 を読むため)
	ClearColorGrade();
	ClearGuardRim();
}

void ModelShaderManager::Terminate()
{
	if (!m_isInit)return;

	for (auto& handle : m_vertexShaders)
	{
		if (handle != -1)DeleteShader(handle);
		handle = -1;
	}
	if (m_blackPixelShader != -1)DeleteShader(m_blackPixelShader);
	m_blackPixelShader = -1;
	if (m_litPixelShader != -1)DeleteShader(m_litPixelShader);
	m_litPixelShader = -1;
	if (m_litBumpPixelShader != -1)DeleteShader(m_litBumpPixelShader);
	m_litBumpPixelShader = -1;
	if (m_lightConstantBuffer != -1)DeleteShaderConstantBuffer(m_lightConstantBuffer);
	m_lightConstantBuffer = -1;
	if (m_colorGradeBuffer != -1)DeleteShaderConstantBuffer(m_colorGradeBuffer);
	m_colorGradeBuffer = -1;
	m_isColorGrade = false;
	if (m_guardRimBuffer != -1)DeleteShaderConstantBuffer(m_guardRimBuffer);
	m_guardRimBuffer = -1;
	m_isGuardRim = false;
	if (m_cutPixelShader != -1)DeleteShader(m_cutPixelShader);
	m_cutPixelShader = -1;
	if (m_cutBumpPixelShader != -1)DeleteShader(m_cutBumpPixelShader);
	m_cutBumpPixelShader = -1;
	if (m_cutConstantBuffer != -1)DeleteShaderConstantBuffer(m_cutConstantBuffer);
	m_cutConstantBuffer = -1;
	if (m_lookPixelShader != -1)DeleteShader(m_lookPixelShader);
	m_lookPixelShader = -1;
	if (m_lookBumpPixelShader != -1)DeleteShader(m_lookBumpPixelShader);
	m_lookBumpPixelShader = -1;
	m_stageLightNum = 0;

	m_isBlackMode = false;
	m_isInit = false;
}

void ModelShaderManager::ResetSceneState()
{
	m_isBlackMode = false;
	m_isCutPlane = false;
	SetStageLights({}, 0.0f, 0.0f, 0.0f);
	ClearColorGrade();
	ClearGuardRim();
}

void ModelShaderManager::SetColorGrade(const COLOR_F& dark, const COLOR_F& bright, float blackLevel, float whiteLevel, float rate)
{
	if (!m_isInit || m_colorGradeBuffer == -1)return;
	auto* buffer = static_cast<ColorGradeConstant*>(GetBufferShaderConstantBuffer(m_colorGradeBuffer));
	buffer->dark = { dark.r, dark.g, dark.b, rate };
	buffer->bright = { bright.r, bright.g, bright.b, 0.0f };
	buffer->level = { blackLevel, whiteLevel, 0.0f, 0.0f };
	UpdateShaderConstantBuffer(m_colorGradeBuffer);
	m_isColorGrade = true;
}

void ModelShaderManager::ClearColorGrade()
{
	if (m_colorGradeBuffer == -1)return;
	auto* buffer = static_cast<ColorGradeConstant*>(GetBufferShaderConstantBuffer(m_colorGradeBuffer));
	buffer->dark.w = 0.0f;
	UpdateShaderConstantBuffer(m_colorGradeBuffer);
	m_isColorGrade = false;
}

void ModelShaderManager::SetGuardRim(const COLOR_F& color, float power)
{
	if (!m_isInit || m_guardRimBuffer == -1)return;
	auto* buffer = static_cast<GuardRimConstant*>(GetBufferShaderConstantBuffer(m_guardRimBuffer));
	buffer->color = { color.r, color.g, color.b, 0.0f };
	buffer->param = { power, 0.0f, 0.0f, 0.0f };
	UpdateShaderConstantBuffer(m_guardRimBuffer);
	m_isGuardRim = true;
}

void ModelShaderManager::ClearGuardRim()
{
	if (m_guardRimBuffer == -1)return;
	auto* buffer = static_cast<GuardRimConstant*>(GetBufferShaderConstantBuffer(m_guardRimBuffer));
	buffer->color = { 0.0f, 0.0f, 0.0f, 0.0f };
	UpdateShaderConstantBuffer(m_guardRimBuffer);
	m_isGuardRim = false;
}

void ModelShaderManager::SetCutPlane(const VECTOR& normal, float distance, float rimWidth)
{
	if (!m_isInit || m_cutConstantBuffer == -1)return;
	auto* buffer = static_cast<CutConstant*>(GetBufferShaderConstantBuffer(m_cutConstantBuffer));
	buffer->plane = { normal.x, normal.y, normal.z, distance };
	buffer->param = { rimWidth, 0.0f, 0.0f, 0.0f };
	UpdateShaderConstantBuffer(m_cutConstantBuffer);
	m_isCutPlane = true;
}

void ModelShaderManager::DrawModel(int modelHandle)
{
	if (!m_isInit)
	{
		MV1DrawModel(modelHandle);
		return;
	}
	if (m_isBlackMode)
	{
		if (m_isCutPlane)
		{
			SetShaderConstantBuffer(m_cutConstantBuffer, DX_SHADERTYPE_PIXEL, kCutSlot);
			DrawModelWithShader(modelHandle, m_cutPixelShader, m_cutBumpPixelShader);
			return;
		}
		DrawModelWithShader(modelHandle, m_blackPixelShader);
		return;
	}
	//明かりの有無に関係なくいつも LitModelPS で描く(DxLibの標準とは陰影が違い、ステージごとに見た目が変わったため)
	if (m_litPixelShader == -1)
	{
		MV1DrawModel(modelHandle);
		return;
	}
	BindStageLights();
	SetShaderConstantBuffer(m_colorGradeBuffer, DX_SHADERTYPE_PIXEL, kColorGradeSlot);
	SetShaderConstantBuffer(m_guardRimBuffer, DX_SHADERTYPE_PIXEL, kGuardRimSlot);
	DrawModelWithShader(modelHandle, m_litPixelShader, m_litBumpPixelShader);
}

void ModelShaderManager::SetStageLights(const std::vector<StageLightParam>& lights, float fogStart, float fogEnd, float wrap)
{
	if (!m_isInit || m_lightConstantBuffer == -1)return;
	auto* buffer = static_cast<StageLightConstant*>(GetBufferShaderConstantBuffer(m_lightConstantBuffer));
	m_stageLightNum = (std::min)(static_cast<int>(lights.size()), kStageLightMax);
	for (int i = 0; i < m_stageLightNum; ++i)
	{
		const auto& light = lights[i];
		//2乗と逆数はここで計算しておく(シェーダーでピクセルごとにしないため)
		buffer->posRange[i] = { light.pos.x, light.pos.y, light.pos.z, light.range * light.range };
		buffer->color[i] = { light.color.r, light.color.g, light.color.b, light.range > 0.0f ? 1.0f / light.range : 0.0f };
	}
	buffer->param = { static_cast<float>(m_stageLightNum), fogStart, fogEnd, wrap };
	UpdateShaderConstantBuffer(m_lightConstantBuffer);
}

void ModelShaderManager::BindStageLights()
{
	if (m_lightConstantBuffer == -1)return;
	SetShaderConstantBuffer(m_lightConstantBuffer, DX_SHADERTYPE_PIXEL, kStageLightSlot);
}

int ModelShaderManager::GetVertexShader(int vertexType)const
{
	if (vertexType < 0 || vertexType >= DX_MV1_VERTEX_TYPE_NUM)return -1;
	return m_vertexShaders[vertexType];
}

int ModelShaderManager::LoadShaderWithMacro(const std::wstring& path, const char* target,
	const std::vector<D3D_SHADER_MACRO>& macros, bool isVertex)
{
	ID3DBlob* pShader = nullptr;//コンパイル済みのシェーダー(vso,psoの中身にあたる)
	ID3DBlob* pMsg = nullptr;//エラーを起こしたときのエラーメッセージ
	HRESULT result = D3DCompileFromFile(path.c_str(),
		macros.data(),
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		kShaderEntryPoint,
		target,
		0, 0, &pShader, &pMsg);

	//失敗したらVisualStudioの「出力」にエラーの内容を出す
	if (FAILED(result))
	{
		if (pMsg != nullptr)
		{
			std::string strMsg(static_cast<char*>(pMsg->GetBufferPointer()), pMsg->GetBufferSize());
			OutputDebugStringA(strMsg.c_str());
			pMsg->Release();
		}
		else
		{
			//ファイルが見つからないときはメッセージが無い
			OutputDebugStringW((L"シェーダーファイルが開けません: " + path + L"\n").c_str());
		}
		assert(0 && "シェーダーのコンパイルに失敗");
		return -1;
	}

	//ここだけDxLibの関数//コンパイル済みのデータからシェーダーハンドルを作る
	const int size = static_cast<int>(pShader->GetBufferSize());
	int handle = isVertex ?
		LoadVertexShaderFromMem(pShader->GetBufferPointer(), size) :
		LoadPixelShaderFromMem(pShader->GetBufferPointer(), size);
	assert(handle >= 0);

	//DirectXのオブジェクトはdeleteではなくReleaseで解放する
	pShader->Release();
	if (pMsg != nullptr)pMsg->Release();

	return handle;
}

bool ModelShaderManager::IsBumpType(int vertexType)
{
	return vertexType == DX_MV1_VERTEX_TYPE_NMAP_1FRAME || vertexType == DX_MV1_VERTEX_TYPE_NMAP_4FRAME || vertexType == DX_MV1_VERTEX_TYPE_NMAP_8FRAME;
}

void ModelShaderManager::DrawModelWithShader(int modelHandle, int pixelShader, int bumpPixelShader)
{
	if (bumpPixelShader == -1)bumpPixelShader = pixelShader;
	//モデル全体が同じ頂点タイプかどうか
	const int tListNum = MV1GetTriangleListNum(modelHandle);
	if (tListNum <= 0)return;
	const int firstType = MV1GetTriangleListVertexType(modelHandle, 0);
	bool isSameType = true;
	for (int i = 1; i < tListNum; ++i)
	{
		if (MV1GetTriangleListVertexType(modelHandle, i) != firstType)
		{
			isSameType = false;
			break;
		}
	}

	if (isSameType)
	{
		//まとめて描く
		const int vs = GetVertexShader(firstType);
		MV1SetUseOrigShader(vs != -1);//対応するシェーダーが無い頂点タイプは標準シェーダーで描く
		SetUseVertexShader(vs);
		SetUsePixelShader(IsBumpType(firstType) ? bumpPixelShader : pixelShader);
		MV1DrawModel(modelHandle);
	}
	else
	{
		//メッシュごとに頂点シェーダーを切り替えて描く
		const int meshNum = MV1GetMeshNum(modelHandle);
		for (int mesh = 0; mesh < meshNum; ++mesh)
		{
			if (MV1GetMeshTListNum(modelHandle, mesh) <= 0)continue;
			const int tList = MV1GetMeshTList(modelHandle, mesh, 0);
			const int type = MV1GetTriangleListVertexType(modelHandle, tList);
			const int vs = GetVertexShader(type);
			MV1SetUseOrigShader(vs != -1);
			SetUseVertexShader(vs);
			SetUsePixelShader(IsBumpType(type) ? bumpPixelShader : pixelShader);
			MV1DrawMesh(modelHandle, mesh);
		}
	}

	//元に戻す(他の描画に影響させないため)
	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	MV1SetUseOrigShader(false);
}
