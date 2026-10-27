// ビューアーで変換した ToiletMech.mv1 にテクスチャ(Skin3 の BaseColor を 2048 の PNG にしたもの)を貼って保存し、確認用の画像を出す
// 使い方: build_mv1.exe 元のmv1 テクスチャのフォルダ(Unity の Textures/Skin 3) 出力フォルダ [兜のテクスチャ(png)]   ※出力フォルダに ToiletMech.mv1・Texture/・log.txt・preview.png
#include "DxLib.h"
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
	constexpr int kTexSize = 2048;//縮小後のテクスチャの大きさ
	constexpr float kDif = 0.1f;//拡散・反射の色(他の体のモデルと同じ)
	constexpr float kAmb = 0.9f;//環境色
	const char* kBodyTex = "T_SciFi_ToiletMech_Body_BaseColor_Skin3";
	const char* kToiletTex = "T_SciFi_ToiletMech_Toilet_BaseColor_Skin3";
	const char* kHelmetMat = "M_ToiletMech_Helmet";//便器の代わりの兜(Unity の ToiletMechExport で付ける)
	const char* kHelmetTex = "ToiletMechHelmet_Albedo";
}

// BaseColor に Emissive を足して(顔のパネル・ライトを焼き込む)縮小し、PNG で保存
static bool ShrinkToPng(const std::string& base, const std::string& emissive, const std::string& dst)
{
	int g = LoadGraph(base.c_str());
	if (g < 0) return false;
	int e = LoadGraph(emissive.c_str());
	int screen = MakeScreen(kTexSize, kTexSize, FALSE);
	SetDrawScreen(screen);
	ClearDrawScreen();
	DrawExtendGraph(0, 0, kTexSize, kTexSize, g, FALSE);
	if (e >= 0)
	{
		SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
		DrawExtendGraph(0, 0, kTexSize, kTexSize, e, FALSE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	SaveDrawScreenToPNG(0, 0, kTexSize, kTexSize, dst.c_str());
	SetDrawScreen(DX_SCREEN_BACK);
	DeleteGraph(screen);
	DeleteGraph(g);
	DeleteGraph(e);
	return e >= 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(1280, 720, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (__argc < 4 || DxLib_Init() == -1) return 1;
	const std::string srcMv1 = __argv[1], texDir = std::string(__argv[2]) + "/", outDir = std::string(__argv[3]) + "/";
	const std::string helmetPng = __argc > 4 ? __argv[4] : "";
	CreateDirectoryA((outDir + "Texture").c_str(), nullptr);
	FILE* log = nullptr;
	fopen_s(&log, (outDir + "log.txt").c_str(), "w");

	for (const char* name : { kBodyTex, kToiletTex })
	{
		std::string emissive = name;
		emissive.replace(emissive.find("BaseColor"), 9, "Emissive");
		bool ok = ShrinkToPng(texDir + name + ".tga", texDir + emissive + ".tga", outDir + "Texture/" + name + ".png");
		fprintf(log, "texture %s %s\n", name, ok ? "ok(+emissive)" : "failed");
	}

	//テクスチャのパスが出力先からの相対になるよう、出力フォルダに写してそこで作業する
	CopyFileA(srcMv1.c_str(), (outDir + "_src.mv1").c_str(), FALSE);
	SetCurrentDirectoryA(outDir.c_str());
	int h = MV1LoadModel("_src.mv1");
	fprintf(log, "load %s = %d\n", srcMv1.c_str(), h);
	if (h < 0) { fclose(log); DxLib_End(); return 2; }

	// テクスチャのパスは mv1 からの相対。便器・兜はそのマテリアルがあるときだけ
	auto hasMaterial = [h](const char* key)
	{
		for (int i = 0; i < MV1GetMaterialNum(h); ++i) if (strstr(MV1GetMaterialName(h, i), key)) return true;
		return false;
	};
	MV1AddTexture(h, kBodyTex, (std::string("Texture/") + kBodyTex + ".png").c_str());
	if (hasMaterial("_Toilet_")) MV1AddTexture(h, kToiletTex, (std::string("Texture/") + kToiletTex + ".png").c_str());
	else DeleteFileA((std::string("Texture/") + kToiletTex + ".png").c_str());
	if (!helmetPng.empty() && hasMaterial(kHelmetMat))
	{
		CopyFileA(helmetPng.c_str(), (std::string("Texture/") + kHelmetTex + ".png").c_str(), FALSE);
		MV1AddTexture(h, kHelmetTex, (std::string("Texture/") + kHelmetTex + ".png").c_str());
	}
	//足した後の番号は名前で探す
	int bodyTex = -1, toiletTex = -1, helmetTex = -1;
	for (int i = 0; i < MV1GetTextureNum(h); ++i)
	{
		const char* name = MV1GetTextureName(h, i);
		const char* path = MV1GetTextureColorFilePath(h, i);
		fprintf(log, "tex[%d] name=%s path=%s\n", i, name ? name : "(null)", path ? path : "(null)");
		fflush(log);
		if (path && strstr(path, kBodyTex)) bodyTex = i;
		if (path && strstr(path, kToiletTex)) toiletTex = i;
		if (path && strstr(path, kHelmetTex)) helmetTex = i;
	}
	for (int i = 0; i < MV1GetMaterialNum(h); ++i)
	{
		const char* name = MV1GetMaterialName(h, i);
		MV1SetMaterialDifMapTexture(h, i, strstr(name, kHelmetMat) ? helmetTex : strstr(name, "_Toilet_") ? toiletTex : bodyTex);
		MV1SetMaterialDifColor(h, i, GetColorF(kDif, kDif, kDif, 1.0f));
		MV1SetMaterialSpcColor(h, i, GetColorF(kDif, kDif, kDif, 1.0f));
		MV1SetMaterialAmbColor(h, i, GetColorF(kAmb, kAmb, kAmb, 1.0f));
		int t = MV1GetMaterialDifMapTexture(h, i);
		fprintf(log, "mat[%d] %s tex=%d %s\n", i, name, t, t >= 0 ? MV1GetTextureColorFilePath(h, t) : "-");
	}
	fprintf(log, "save ret=%d\n", MV1SaveModelToMV1File(h, "ToiletMech.mv1"));
	MV1DeleteModel(h);
	DeleteFileA("_src.mv1");

	// 読み直して確認
	int r = MV1LoadModel("ToiletMech.mv1");
	for (int i = 0; i < MV1GetMaterialNum(r); ++i)
	{
		int t = MV1GetMaterialDifMapTexture(r, i);
		fprintf(log, " reload mat[%d] tex=%d %s\n", i, t, t >= 0 ? MV1GetTextureColorFilePath(r, t) : "-");
	}
	fprintf(log, "reload=%d frames=%d meshes=%d textures=%d anims=%d\n", r, MV1GetFrameNum(r), MV1GetMeshNum(r), MV1GetTextureNum(r), MV1GetAnimNum(r));
	for (int i = 0; i < MV1GetAnimNum(r); ++i) fprintf(log, " anim[%d] %s %.1f\n", i, MV1GetAnimName(r, i), MV1GetAnimTotalTime(r, i));
	for (int i = 0; i < MV1GetFrameNum(r); ++i)
	{
		const char* name = MV1GetFrameName(r, i);
		if (MV1GetFrameMeshNum(r, i) > 0 || strcmp(name, "head") == 0 || strcmp(name, "pelvis") == 0 || strcmp(name, "toilet_joint") == 0)
			fprintf(log, " frame[%d] %s meshes=%d\n", i, name, MV1GetFrameMeshNum(r, i));
	}

	// 確認用の画像: 左=Idle2、右=Attack2 の途中
	SetDrawScreen(DX_SCREEN_BACK);
	SetBackgroundColor(60, 60, 70);
	ClearDrawScreen();
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	SetCameraNearFar(10.0f, 5000.0f);
	SetCameraPositionAndTarget_UpVecY(VGet(0.0f, 180.0f, -650.0f), VGet(0.0f, 150.0f, 0.0f));
	MV1SetupReferenceMesh(r, -1, TRUE);
	MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(r, -1, TRUE);
	fprintf(log, "bind min=(%.1f %.1f %.1f) max=(%.1f %.1f %.1f)\n", ref.MinPosition.x, ref.MinPosition.y, ref.MinPosition.z, ref.MaxPosition.x, ref.MaxPosition.y, ref.MaxPosition.z);
	const char* anims[] = { "Idle2", "Attack2" };
	for (int k = 0; k < 2; ++k)
	{
		int idx = MV1GetAnimIndex(r, anims[k]);
		int a = MV1AttachAnim(r, idx < 0 ? 0 : idx, -1, FALSE);
		MV1SetAttachAnimTime(r, a, MV1GetAttachAnimTotalTime(r, a) * 0.4f);
		MV1SetPosition(r, VGet(k == 0 ? -170.0f : 170.0f, 0.0f, 0.0f));
		MV1SetRotationXYZ(r, VGet(0.0f, DX_PI_F, 0.0f));
		MV1DrawModel(r);
		MV1DetachAnim(r, a);
	}
	SaveDrawScreenToPNG(0, 0, 1280, 720, "preview.png");
	fclose(log);
	DxLib_End();
	return 0;
}
