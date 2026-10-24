#pragma once

//============================================================================
// 一時的なライト：ゲーム内が暗くて見づらいので、真上から照らすライトを足す
//
// ・下の TEMP_TOP_LIGHT_ENABLE を 0 にすると、ライトを作らなくなる(まとめてオフ)
// ・完全に消すときは、このファイルと.cppを削除して「[TempTopLight]」で検索した行を消す
//   (SceneMain.h の include とメンバー、SceneMain.cpp の Init 呼び出し、vcxproj/filters)
//============================================================================
#define TEMP_TOP_LIGHT_ENABLE 1

class TempTopLight
{
public:
	~TempTopLight();

	//ライトを作る//SceneMain::Init で呼ぶ
	void Init();

private:
	int m_lightHandle = -1;
};
