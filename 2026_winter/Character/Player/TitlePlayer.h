#pragma once
#include "DxLib.h"
#include "Render/CharacterLook.h"
#include "Render/Mv1LikeUnity.h"
#include "Character/Animation.h"
#include <string>
#include <unordered_map>
#include <vector>

//タイトルの後ろ姿のプレイヤー//ポーズで止めて、強い風で髪と布をなびかせる・息
//体はゲームのモデル(CharacterLook)、髪は IdaFaber の SK_VEX_01(Unity と同じ描き方 Mv1LikeUnity)を重ねて描く
class TitlePlayer
{
public:
	TitlePlayer();
	~TitlePlayer();

	void Init();
	void Update();
	void Draw();

	//斬った瞬間//突風で髪をあおる・光を赤くする
	void OnImpact();
	//斬り終わった所まで飛ばす
	void Skip();

private:
	//風で動かす骨
	struct WindBone
	{
		int frame = -1;
		int child = -1;//向きを測る子(無ければ-1)
		float depth = 0.0f;//根元からの段(0〜1)
		float phase = 0.0f;//房ごとのずれ
		float power = 1.0f;//髪1、布は弱め
	};
	//同じポーズ・同じ骨の動きをさせるモデル1つ分
	struct PoseModel
	{
		int handle = -1;
		Animation anim;
		int spine[2] = { -1, -1 };
		int clavicle[2] = { -1, -1 };
		std::vector<WindBone> windBones;
		std::vector<int> userFrames;//毎フレーム戻す骨
	};

	void SetupBones(PoseModel& model);
	void AddWindChain(PoseModel& model, const char* format, const char* side, int num, float phase, float power);
	//頭の骨を同じ所に置く
	void Place(PoseModel& model, float scale);
	//マテリアル名の先頭が name のメッシュだけ見せる(isOnly)か、それだけ隠す
	void SetHairVisible(int handle, bool isOnly);
	//アニメの姿勢に、骨の位置を中心にしたワールドの回転を足す
	void RotateFrame(int handle, int frame, const MATRIX& worldRot);
	void UpdatePose(PoseModel& model, float windStrength);
	void UpdateBreath(PoseModel& model);
	void UpdateWind(PoseModel& model, float strength);
	const std::string& GetAnimName(const std::string& key)const;//アニメーションの名前を取得する

	PoseModel m_body;//ゲームのモデル(髪を隠す)
	PoseModel m_hair;//IdaFaber のモデル(髪だけ)
	int m_animSrcHandle = -1;//IdaFaber のポーズのモーションを持っているモデル
	std::unordered_map<std::string, std::string> m_animNames;//アニメーションの名前を管理
	CharacterLook m_look;//体の描き方
	Mv1LikeUnity::Materials* m_materials = nullptr;//髪の描き方(Mv1LikeUnity が持つ)

	VECTOR m_forward = VGet(0.0f, 0.0f, -1.0f);//体の前(ワールド)
	VECTOR m_right = VGet(1.0f, 0.0f, 0.0f);

	int m_frame = 0;
	float m_gustKick = 0.0f;//斬った時の突風(0〜1)
	float m_rimFlash = 0.0f;//赤い光の強さ(1で一番)
	float m_rimRest = 0.0f;
};
