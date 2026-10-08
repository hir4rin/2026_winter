#include "ModelShaderManager.h"
#include <cassert>

#pragma comment(lib, "d3dcompiler")//D3DCompileFromFileを使うため

namespace
{
	const std::wstring kModelVertexShaderPath = L"Shader/MV1VertexShader.hlsl";
	const std::wstring kBlackPixelShaderPath = L"Shader/EnemyBlackPS.hlsl";
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

	m_isInit = true;
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

	m_isBlackMode = false;
	m_isInit = false;
}

void ModelShaderManager::DrawModel(int modelHandle)
{
	if (!m_isBlackMode || !m_isInit)
	{
		MV1DrawModel(modelHandle);
		return;
	}
	DrawModelWithShader(modelHandle, m_blackPixelShader);
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

void ModelShaderManager::DrawModelWithShader(int modelHandle, int pixelShader)
{
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

	SetUsePixelShader(pixelShader);

	if (isSameType)
	{
		//まとめて描く
		const int vs = GetVertexShader(firstType);
		MV1SetUseOrigShader(vs != -1);//対応するシェーダーが無い頂点タイプは標準シェーダーで描く
		SetUseVertexShader(vs);
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
			const int vs = GetVertexShader(MV1GetTriangleListVertexType(modelHandle, tList));
			MV1SetUseOrigShader(vs != -1);
			SetUseVertexShader(vs);
			MV1DrawMesh(modelHandle, mesh);
		}
	}

	//元に戻す(他の描画に影響させないため)
	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	MV1SetUseOrigShader(false);
}
