#include "TempTopLight.h"
#include "DxLib.h"

namespace
{
	const VECTOR kLightDir = { 0.0f, -1.0f, 0.0f };//真上から
	constexpr float kDifColorValue = 0.5f;//ディフューズ
	constexpr float kSpcColorValue = 0.0f;//スペキュラ//てかりは足さない
	constexpr float kAmbColorValue = 0.58f;//アンビエント//影になる面も少し明るくする//切った標準ライトの環境光(0.33)の分も足している(キャラはアンビエントで明るさが決まる)
}

TempTopLight::~TempTopLight()
{
	if (m_lightHandle != -1)
	{
		DeleteLightHandle(m_lightHandle);
		m_lightHandle = -1;
	}
}

void TempTopLight::Init()
{
#if TEMP_TOP_LIGHT_ENABLE
	if (m_lightHandle != -1)return;
	m_lightHandle = CreateDirLightHandle(kLightDir);
	SetLightDifColorHandle(m_lightHandle, GetColorF(kDifColorValue, kDifColorValue, kDifColorValue, 1.0f));
	SetLightSpcColorHandle(m_lightHandle, GetColorF(kSpcColorValue, kSpcColorValue, kSpcColorValue, 1.0f));
	SetLightAmbColorHandle(m_lightHandle, GetColorF(kAmbColorValue, kAmbColorValue, kAmbColorValue, 1.0f));
#endif
}
