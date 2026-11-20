#include "TitlePlayer.h"
#include "Main/System.h"
#include "DataLoader/DataManager.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace
{
	//体: ゲームのモデル(CharacterLook)
	const char* const kLookCsvPath = "data/Player/Look/PlayerLook.csv";
	//髪: IdaFaber の SK_VEX_01(Unity と同じ描き方。画像は 2048 に縮めてある)
	const char* const kHairModelPath = "data/Title/IdaFaber/SK_VEX_01/SK_VEX_01.mv1";
	const char* const kMaterialsPath = "data/Title/IdaFaber/SK_VEX_01/SK_VEX_01_Materials.txt";
	//IdaFaber のポーズのモーション(骨の数が違うので名前で当てる)
	const char* const kPoseAnimPath = "data/Title/IdaFaber/Ninja_Idle_LookingAround.mv1";
	const char* const kHairMaterial = "MAT_HAIR";//マテリアル名の先頭
	constexpr float kPoseFrame = 116.8077f;//このフレームのポーズで止める

	//置き方(タイトルカメラで z=0 の面が 1単位=1ピクセル)
	constexpr float kScale = 15.0f;
	const VECTOR kHeadPos = { 380.0f, -60.0f, 150.0f };//頭の骨を置く所
	constexpr float kYawDeg = 160.0f;//後ろ向き(少しロゴの方へ)
	constexpr float kDegToRad = DX_PI_F / 180.0f;

	//体の光(CharacterLook)//ふだんは青く弱いふちの光、斬った瞬間に赤く光って少し残る
	const COLOR_F kRimCalm = { 0.10f, 0.13f, 0.22f, 1.0f };
	const COLOR_F kRimRed = { 0.75f, 0.08f, 0.07f, 1.0f };
	//髪の光(Mv1LikeUnity は DxLib の標準ライトを使う)
	const VECTOR kLightDir = { 1.0f, -0.6f, 0.3f };//画面の左上から
	const COLOR_F kLightColor = { 0.9f, 0.92f, 1.0f, 1.0f };
	const COLOR_F kLightRed = { 1.6f, 0.15f, 0.12f, 1.0f };
	constexpr float kSky = 0.12f;//環境光(暗め)
	constexpr float kRimRest = 0.3f;//光った後に残る赤の割合
	constexpr float kRimDecay = 0.93f;

	//息
	constexpr float kBreathSpeed = DX_TWO_PI_F / 270.0f;
	constexpr float kBreathSpineDeg = 0.8f;
	constexpr float kBreathClavicleDeg = 1.2f;

	//風(画面の右から左へ、少し手前へ)
	const VECTOR kWindDir = { -1.0f, 0.35f, -0.35f };
	constexpr float kWindAlign = 0.38f;//1本の骨で風下へ向ける割合
	constexpr float kWindBase = 0.8f;//風の強さ
	constexpr float kWindGust = 0.5f;//突風で増える分
	constexpr float kGustSpeed = DX_TWO_PI_F / 150.0f;
	constexpr float kFlutterDeg = 7.0f;//ばたつき(先ほど大きい)
	constexpr float kFlutterSpeed = DX_TWO_PI_F / 22.0f;
	constexpr float kFlutterLag = 2.2f;//根元から先への遅れ
	constexpr float kGustKickDecay = 0.95f;
	constexpr float kClothPower = 0.6f;//布は髪より弱め
}

TitlePlayer::TitlePlayer()
{
}

TitlePlayer::~TitlePlayer()
{
	Mv1LikeUnity::ReleaseMaterials(m_hair.handle);
	MV1DeleteModel(m_hair.handle);
	MV1DeleteModel(m_body.handle);
	MV1DeleteModel(m_animSrcHandle);
}

void TitlePlayer::Init()
{
	//アニメーションの名前のマップの初期化
	const auto& animData = DataManager::GetInstance().GetPlayerAnimData();
	m_animNames = animData.animNames;

	//体//髪は隠す
	m_body.handle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::PlayerModel));
	m_look.Load(m_body.handle, kLookCsvPath);
	SetHairVisible(m_body.handle, false);
	//再生速度0でポーズのフレームに止める
	m_body.anim.Init(m_body.handle, GetAnimName("TitlePose"), false, 0.0f, -1.0f, kPoseFrame);

	//髪//髪だけ見せる。両面表示・切り抜きは Mv1LikeUnity の Draw がする
	m_hair.handle = MV1LoadModel(kHairModelPath);
	m_materials = &Mv1LikeUnity::LoadMaterials(m_hair.handle, kMaterialsPath);
	SetHairVisible(m_hair.handle, true);
	m_animSrcHandle = MV1LoadModel(kPoseAnimPath);
	m_hair.anim.SetAnimSourceModel(m_animSrcHandle, true);
	m_hair.anim.Init(m_hair.handle, GetAnimName("TitlePose"), false, 0.0f, -1.0f, kPoseFrame);

	SetupBones(m_body);
	SetupBones(m_hair);

	//体と頭の大きさが合うように、髪のモデルの大きさを背骨の上から頭までの長さでそろえる
	auto neckLength = [](int handle)
		{
			MV1SetScale(handle, VGet(1.0f, 1.0f, 1.0f));
			return VSize(VSub(MV1GetFramePosition(handle, MV1SearchFrame(handle, "head")), MV1GetFramePosition(handle, MV1SearchFrame(handle, "spine_05"))));
		};
	const float hairLength = neckLength(m_hair.handle);
	const float hairScale = (hairLength > 0.0f) ? kScale * neckLength(m_body.handle) / hairLength : kScale;
	Place(m_body, kScale);
	Place(m_hair, hairScale);
}

void TitlePlayer::SetupBones(PoseModel& model)
{
	model.spine[0] = MV1SearchFrame(model.handle, "spine_03");
	model.spine[1] = MV1SearchFrame(model.handle, "spine_05");
	model.clavicle[0] = MV1SearchFrame(model.handle, "clavicle_l");
	model.clavicle[1] = MV1SearchFrame(model.handle, "clavicle_r");

	//髪(前は左右6段、後ろは左・真ん中・右の7段、ポニーテール3段)と布
	AddWindChain(model, "hair_front_%02d_%s", "l", 6, 0.0f, 1.0f);
	AddWindChain(model, "hair_front_%02d_%s", "r", 6, 1.3f, 1.0f);
	AddWindChain(model, "hair_back_%02d_%s", "l", 7, 0.7f, 1.0f);
	AddWindChain(model, "hair_back_%02d_%s", "mid", 7, 1.6f, 1.0f);
	AddWindChain(model, "hair_back_%02d_%s", "r", 7, 2.5f, 1.0f);
	AddWindChain(model, "cng_hair_ponytail_%02d%s", "", 3, 3.1f, 1.0f);
	AddWindChain(model, "cng_cloth_%02d%s", "", 4, 0.4f, kClothPower);
	AddWindChain(model, "cng_cloth_back_%02d%s", "", 4, 2.0f, kClothPower);

	//親から順に回すので、骨の番号の小さい順(親の方が小さい)
	std::sort(model.windBones.begin(), model.windBones.end(), [](const WindBone& a, const WindBone& b) { return a.frame < b.frame; });
	for (int f : { model.spine[0], model.spine[1], model.clavicle[0], model.clavicle[1] })
	{
		if (f >= 0)model.userFrames.push_back(f);
	}
	for (auto& bone : model.windBones)model.userFrames.push_back(bone.frame);
}

void TitlePlayer::AddWindChain(PoseModel& model, const char* format, const char* side, int num, float phase, float power)
{
	char name[64];
	int prev = -1;
	for (int i = 1; i <= num; ++i)
	{
		sprintf_s(name, format, i, side);
		const int frame = MV1SearchFrame(model.handle, name);
		if (frame < 0)continue;
		if (prev >= 0)model.windBones[prev].child = frame;
		const float depth = (num > 1) ? (i - 1) / static_cast<float>(num - 1) : 0.0f;
		model.windBones.push_back({ frame, -1, depth, phase, power });
		prev = static_cast<int>(model.windBones.size()) - 1;
	}
}

void TitlePlayer::SetHairVisible(int handle, bool isOnly)
{
	for (int i = 0; i < MV1GetMeshNum(handle); ++i)
	{
		const char* material = MV1GetMaterialName(handle, MV1GetMeshMaterial(handle, i));
		const bool isHair = strncmp(material, kHairMaterial, strlen(kHairMaterial)) == 0;
		MV1SetMeshVisible(handle, i, (isHair == isOnly) ? TRUE : FALSE);
	}
}

void TitlePlayer::Place(PoseModel& model, float scale)
{
	const float yaw = kYawDeg * kDegToRad;
	const MATRIX rot = MGetRotY(yaw);
	m_forward = VTransformSR(VGet(0.0f, 0.0f, -1.0f), rot);
	m_right = VTransformSR(VGet(-1.0f, 0.0f, 0.0f), rot);

	MV1SetScale(model.handle, VGet(scale, scale, scale));
	MV1SetRotationXYZ(model.handle, VGet(0.0f, yaw, 0.0f));
	MV1SetPosition(model.handle, VGet(0.0f, 0.0f, 0.0f));
	const VECTOR head = MV1GetFramePosition(model.handle, MV1SearchFrame(model.handle, "head"));
	MV1SetPosition(model.handle, VSub(kHeadPos, head));
}

void TitlePlayer::Update()
{
	m_frame++;
	//ポーズで止めているのでアニメは進めない
	m_gustKick *= kGustKickDecay;
	m_rimFlash *= kRimDecay;

	//突風の強弱(2つの波を掛けて、むらのある吹き方にする)
	const float t = static_cast<float>(m_frame);
	const float gustWave = (0.5f + 0.5f * std::sin(t * kGustSpeed)) * (0.6f + 0.4f * std::sin(t * kGustSpeed * 2.7f + 1.3f));
	const float strength = kWindBase + kWindGust * gustWave + m_gustKick;

	//体と髪に同じ動きをさせる
	UpdatePose(m_body, strength);
	UpdatePose(m_hair, strength);
}

void TitlePlayer::UpdatePose(PoseModel& model, float windStrength)
{
	//アニメの姿勢に戻してから足す
	for (int f : model.userFrames)MV1ResetFrameUserLocalMatrix(model.handle, f);
	UpdateBreath(model);
	UpdateWind(model, windStrength);
}

const std::string& TitlePlayer::GetAnimName(const std::string& key) const
{
	auto it = m_animNames.find(key);
	assert(it != m_animNames.end() && "指定されたキーが見つかりませんでした");
	return it->second;
}

void TitlePlayer::RotateFrame(int handle, int frame, const MATRIX& worldRot)
{
	if (frame < 0)return;
	const int parent = MV1GetFrameParent(handle, frame);
	const MATRIX parentWorld = (parent >= 0) ? MV1GetFrameLocalWorldMatrix(handle, parent) : MV1GetMatrix(handle);
	MATRIX world = MMult(MV1GetFrameLocalMatrix(handle, frame), parentWorld);
	const VECTOR pivot = VGet(world.m[3][0], world.m[3][1], world.m[3][2]);
	world = MMult(MMult(MMult(world, MGetTranslate(VScale(pivot, -1.0f))), worldRot), MGetTranslate(pivot));
	MV1SetFrameUserLocalMatrix(handle, frame, MMult(world, MInverse(parentWorld)));
}

void TitlePlayer::UpdateBreath(PoseModel& model)
{
	const float breath = std::sin(m_frame * kBreathSpeed);
	const float spine = breath * kBreathSpineDeg * kDegToRad;
	RotateFrame(model.handle, model.spine[0], MGetRotAxis(m_right, spine));
	RotateFrame(model.handle, model.spine[1], MGetRotAxis(m_right, spine * 0.7f));
	//吸うと肩が少し上がる
	const float shoulder = (std::max)(breath, 0.0f) * kBreathClavicleDeg * kDegToRad;
	RotateFrame(model.handle, model.clavicle[0], MGetRotAxis(m_forward, -shoulder));
	RotateFrame(model.handle, model.clavicle[1], MGetRotAxis(m_forward, shoulder));
}

void TitlePlayer::UpdateWind(PoseModel& model, float strength)
{
	const float t = static_cast<float>(m_frame);
	const VECTOR wind = VNorm(kWindDir);
	const int handle = model.handle;

	for (const auto& bone : model.windBones)
	{
		//今の骨の向き(子へ)。子が無い先の骨は親からの向き
		const VECTOR pos = MV1GetFramePosition(handle, bone.frame);
		VECTOR dir;
		if (bone.child >= 0)
		{
			dir = VSub(MV1GetFramePosition(handle, bone.child), pos);
		}
		else
		{
			dir = VSub(pos, MV1GetFramePosition(handle, MV1GetFrameParent(handle, bone.frame)));
		}
		if (VSize(dir) < 0.0001f)continue;
		dir = VNorm(dir);

		VECTOR axis = VCross(dir, wind);
		if (VSize(axis) < 0.0001f)continue;
		axis = VNorm(axis);
		const VECTOR side = VNorm(VCross(axis, dir));

		//風下へ曲げる + ばたつき(先ほど大きく、根元から遅れて伝わる)
		const float toWind = std::acos(std::clamp(VDot(dir, wind), -1.0f, 1.0f));
		const float bend = toWind * kWindAlign * (std::min)(strength, 1.0f) * bone.power;
		const float lag = bone.depth * kFlutterLag;
		const float flutter = (std::sin(t * kFlutterSpeed - lag + bone.phase) + 0.5f * std::sin(t * kFlutterSpeed * 2.3f - lag * 1.7f + bone.phase * 1.9f))
			* kFlutterDeg * kDegToRad * (0.4f + 0.6f * bone.depth) * strength * bone.power;

		//回す向きは、dir が wind に近づく方を選ぶ
		MATRIX bendRot = MGetRotAxis(axis, bend);
		if (VDot(VTransformSR(dir, bendRot), wind) < VDot(dir, wind))bendRot = MGetRotAxis(axis, -bend);
		RotateFrame(handle, bone.frame, MMult(MMult(bendRot, MGetRotAxis(axis, flutter * 0.5f)), MGetRotAxis(side, flutter)));
	}
}

void TitlePlayer::OnImpact()
{
	m_gustKick = 1.0f;
	m_rimFlash = 1.0f;
	m_rimRest = kRimRest;
}

void TitlePlayer::Skip()
{
	m_rimRest = kRimRest;
}

void TitlePlayer::Draw()
{
	const float red = (std::max)(m_rimFlash, m_rimRest);

	//体//タイトル用の光にして描き、戻す
	auto& settings = CharacterLook::Settings();
	const CharacterLook::LookSettings saved = settings;
	settings.keyDir = VGet(-1.0f, 0.6f, -0.3f);//画面の左上から
	settings.keyColor = GetColorF(0.95f, 0.95f, 1.05f, 1.0f);
	settings.skyColor = GetColorF(0.10f, 0.11f, 0.14f, 1.0f);
	settings.groundColor = GetColorF(0.04f, 0.03f, 0.03f, 1.0f);
	settings.rimColor = GetColorF(kRimCalm.r + (kRimRed.r - kRimCalm.r) * red, kRimCalm.g + (kRimRed.g - kRimCalm.g) * red,
		kRimCalm.b + (kRimRed.b - kRimCalm.b) * red, 1.0f);
	settings.rimPower = 2.5f;
	m_look.Draw(m_body.handle);
	settings = saved;

	//髪//DxLib の標準ライトを変えて描き、戻す
	const VECTOR savedDir = GetLightDirection();
	const COLOR_F savedColor = GetLightDifColor();
	auto& env = Mv1LikeUnity::Env();
	const float savedSky = env.sky;
	SetLightDirection(VNorm(kLightDir));
	SetLightDifColor(GetColorF(kLightColor.r + (kLightRed.r - kLightColor.r) * red, kLightColor.g + (kLightRed.g - kLightColor.g) * red,
		kLightColor.b + (kLightRed.b - kLightColor.b) * red, 1.0f));
	env.sky = kSky;
	Mv1LikeUnity::Draw(m_hair.handle, *m_materials);
	SetLightDirection(savedDir);
	SetLightDifColor(savedColor);
	env.sky = savedSky;
}
