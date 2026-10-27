#include "BossArm.h"
#include "DxLib.h"
#include "Main/System.h"
#include "DataLoader/DataManager.h"
#include "Render/ModelShaderManager.h"
#include "Character/Enemy/State/Locomotion/EnemyIdle.h"
#include "Character/Enemy/BossPirate/State/BossArmDetached.h"
#include "Character/Enemy/BossPirate/State/BossArmReturn.h"
#include "imgui.h"
#include <cstdio>
#include <cfloat>

namespace
{
	constexpr const char* kRootFrameName = "spine_03";//付け根にするボスのフレーム
	constexpr const char* kArmSpineFrameName = "spine_03";//腕のモデルの背骨(鎖骨の親)

	//付け根からのずれ(左腕)//右腕は左右反転して使う//ImGuiで決めて書き写す
	//全部0・倍率1なら、ボスの左腕にぴったり重なる
	constexpr float kOffsetPosX = 116.0f;
	constexpr float kOffsetPosY = 0.0f;
	constexpr float kOffsetPosZ = 0.0f;
	constexpr float kOffsetRotXDeg = 0.0f;
	constexpr float kOffsetRotYDeg = 0.0f;
	constexpr float kOffsetRotZDeg = 94.0f;
	constexpr float kArmScale = 1.25f;//ボスの大きさに対する倍率

	constexpr float kDegToRad = DX_PI_F / 180.0f;

	constexpr float kDebugSphereRadius = 10.0f;
	constexpr int kDebugSphereDivNum = 8;
	constexpr float kDebugTextHeight = 30.0f;//付け根から文字までの高さ
}

BossArm::BossArm(std::weak_ptr<Player> player, std::weak_ptr<EnemyBase> boss, int bossModelHandle, Side side) :
	EnemyBase(player),
	m_boss(boss),
	m_bossModelHandle(bossModelHandle),
	m_side(side),
	m_offsetPos(kOffsetPosX, kOffsetPosY, kOffsetPosZ),
	m_offsetRotDeg(kOffsetRotXDeg, kOffsetRotYDeg, kOffsetRotZDeg),
	m_scale(kArmScale)
{
	//右腕は左腕の値を左右反転する
	if (m_side == Side::Right)
	{
		m_offsetPos.x = -m_offsetPos.x;
		m_offsetRotDeg.y = -m_offsetRotDeg.y;
		m_offsetRotDeg.z = -m_offsetRotDeg.z;
	}

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(
		m_side == Side::Left ? AsyncData::BossArmLModel : AsyncData::BossArmRModel));

	m_rootFrame = MV1SearchFrame(m_bossModelHandle, kRootFrameName);
	if (m_rootFrame < 0)m_rootFrame = 0;//ImGuiで選び直す
	m_armSpineFrame = MV1SearchFrame(m_modelHandle, kArmSpineFrameName);

	//アニメはボスのモデルのものを使う//名前の割り当ては腕用のCSV(BossArmAnim.csv)
	m_animNames = DataManager::GetInstance().GetBossArmAnimData().animNames;
	m_animSpeeds = DataManager::GetInstance().GetBossArmAnimData().animSpeeds;//アニメCSVの再生速度
	m_anim.SetAnimSourceModel(m_bossModelHandle);

	//Idleから他のステートに移らないように
	m_status.idleTime = FLT_MAX;
}

BossArm::~BossArm()
{
	MV1DeleteModel(m_modelHandle);
}

void BossArm::Init()
{
	m_anim.Init(m_modelHandle, GetAnimName("Idle"), true, GetAnimSpeed("Idle"));
	//当たり判定はまだ無い//付け根の位置だけ合わせる
	m_rb.m_pos = Vector3::FromDxLibVector(MV1GetFramePosition(m_bossModelHandle, m_rootFrame));
	ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));
}

void BossArm::Update()
{
	//ボスが消えたら一緒に消える
	auto boss = m_boss.lock();
	if (!boss || boss->GetIsVanished())
	{
		m_isVanished = true;
		return;
	}

	if (m_currentState)
	{
		m_currentState->Update();
	}
	m_anim.Update(m_ownTimeScale);

	//付け根の位置//描画はDrawで合わせ直す
	//m_rb.m_pos = Vector3::FromDxLibVector(MV1GetFramePosition(m_bossModelHandle, m_rootFrame));

	//離れているとき
	if (m_isDetached)
	{

		const MATRIX spine = CalcDetachedSpineMatrix();
		m_rb.m_pos = Vector3(spine.m[3][0], spine.m[3][1], spine.m[3][2]);
	}
	//くっついているとき
	else
	{
		m_rb.m_pos = Vector3::FromDxLibVector(MV1GetFramePosition(m_bossModelHandle, m_rootFrame));
	}

}

void BossArm::Draw()
{
	if (m_isVanished)return;

	//モデル全体はボスと同じ行列にして、背骨だけボスの付け根＋ずらしに上書きする
	
	//ボスにここで合わせている
	MV1SetMatrix(m_modelHandle, MV1GetMatrix(m_bossModelHandle));
	if (m_armSpineFrame >= 0)
	{
		MV1SetFrameUserLocalWorldMatrix(m_modelHandle, m_armSpineFrame, CalcDrawSpineMatrix());
	}
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);//血殺中は黒く描く
}

void BossArm::DebugDraw()
{
	if (m_isVanished)return;

	const unsigned int color = GetColor(0, 255, 128);
	const Vector3 rootPos = Vector3::FromDxLibVector(MV1GetFramePosition(m_bossModelHandle, m_rootFrame));
	DrawSphere3D(rootPos.ToDxLibVector(), kDebugSphereRadius, kDebugSphereDivNum, color, color, true);

	const Vector3 textPos = rootPos + Vector3(0, kDebugTextHeight, 0);
	VECTOR screenPos = ConvWorldPosToScreenPos(textPos.ToDxLibVector());
	if (screenPos.z <= 0.0f || screenPos.z >= 1.0f)return;
	char text[64];
	snprintf(text, sizeof(text), "BossArm %s %s", m_side == Side::Left ? "L" : "R",
		m_currentState ? m_currentState->GetName() : "-");
	DrawString(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), text, color);
}

void BossArm::DrawEditor()
{
	const char* label = m_side == Side::Left ? "Left Arm" : "Right Arm";
	if (!ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen))return;

	ImGui::PushID(label);

	//付け根のフレーム
	const int frameNum = MV1GetFrameNum(m_bossModelHandle);
	ImGui::InputInt("Root Frame", &m_rootFrame);
	if (m_rootFrame < 0)m_rootFrame = 0;
	if (m_rootFrame >= frameNum)m_rootFrame = frameNum - 1;
	ImGui::Text("Frame Name: %s", MV1GetFrameName(m_bossModelHandle, m_rootFrame));

	//ずれ
	ImGui::DragFloat3("Pos", &m_offsetPos.x, 0.5f);
	ImGui::DragFloat3("Rot (deg)", &m_offsetRotDeg.x, 1.0f, -360.0f, 360.0f);
	ImGui::DragFloat("Scale", &m_scale, 0.01f, 0.1f, 5.0f, "%.3f");

	//定数に書き写す用
	if (ImGui::Button("Copy Constants"))
	{
		char text[512];
		snprintf(text, sizeof(text),
			"//Root Frame: %d (%s)\n"
			"constexpr float kOffsetPosX = %.2ff;\n"
			"constexpr float kOffsetPosY = %.2ff;\n"
			"constexpr float kOffsetPosZ = %.2ff;\n"
			"constexpr float kOffsetRotXDeg = %.2ff;\n"
			"constexpr float kOffsetRotYDeg = %.2ff;\n"
			"constexpr float kOffsetRotZDeg = %.2ff;\n"
			"constexpr float kArmScale = %.3ff;\n",
			m_rootFrame, MV1GetFrameName(m_bossModelHandle, m_rootFrame),
			m_offsetPos.x, m_offsetPos.y, m_offsetPos.z,
			m_offsetRotDeg.x, m_offsetRotDeg.y, m_offsetRotDeg.z, m_scale);
		ImGui::SetClipboardText(text);
	}

	//アニメ//ボスのアニメから選んで腕だけに流す
	const int animNum = MV1GetAnimNum(m_bossModelHandle);
	if (animNum > 0)
	{
		if (m_debugAnimIndex >= animNum)m_debugAnimIndex = 0;
		if (ImGui::BeginCombo("Anim", MV1GetAnimName(m_bossModelHandle, m_debugAnimIndex)))
		{
			for (int i = 0; i < animNum; ++i)
			{
				const bool isSelected = i == m_debugAnimIndex;
				if (ImGui::Selectable(MV1GetAnimName(m_bossModelHandle, i), isSelected))m_debugAnimIndex = i;
				if (isSelected)ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::Checkbox("Loop", &m_isDebugAnimLoop);
		ImGui::SameLine();
		if (ImGui::Button("Play"))
		{
			m_anim.ChangeAnim(MV1GetAnimName(m_bossModelHandle, m_debugAnimIndex), m_isDebugAnimLoop);
		}
		ImGui::SameLine();
		if (ImGui::Button("Idle"))
		{
			ReturnToIdle();//切り離し中なら背中に戻す
		}
		ImGui::Text("Frame: %.1f / %.1f", m_anim.GetNowAnimFrame(), m_anim.GetAnimEndFrame());
	}

	ImGui::PopID();
}

void BossArm::SetMirrorOffset(const BossArm& other)
{
	m_rootFrame = other.m_rootFrame;
	m_offsetPos = Vector3(-other.m_offsetPos.x, other.m_offsetPos.y, other.m_offsetPos.z);
	m_offsetRotDeg = Vector3(other.m_offsetRotDeg.x, -other.m_offsetRotDeg.y, -other.m_offsetRotDeg.z);
	m_scale = other.m_scale;
}

std::shared_ptr<EnemyStateBase> BossArm::NextAfterIdle()
{
	return std::make_shared<EnemyIdle>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> BossArm::NextAfterRecover()
{
	return std::make_shared<EnemyIdle>(GetWeakPtr());
}

void BossArm::PlayAnim(const std::string& key, bool isLoop, float timeScale)
{
	m_anim.ChangeAnim(GetAnimName(key), isLoop, timeScale * GetAnimSpeed(key));
}

void BossArm::ReturnToIdle()
{
	m_isDetached = false;
	m_returnRate = 0.0f;
	//EnterでIdleのアニメが流れる
	ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));
}

void BossArm::Detach()
{
	//もう上がっているならそのまま
	if (std::dynamic_pointer_cast<BossArmDetached>(m_currentState))return;

	//今の形から切り離す(戻り中なら途中の形から)
	const MATRIX now = CalcDrawSpineMatrix();
	ChangeState(std::make_shared<BossArmDetached>(GetWeakPtr()));
	m_detachSpineMatrix = now;
	m_detachMove = Vector3();
	m_detachSpinDeg = 0.0f;
	m_returnRate = 0.0f;
	m_isDetached = true;
}

void BossArm::RequestReturn()
{
	if (!m_isDetached)return;
	ChangeState(std::make_shared<BossArmReturn>(GetWeakPtr()));
}

MATRIX BossArm::CalcSpineMatrix() const
{
	MATRIX rootWorld = MV1GetFrameLocalWorldMatrix(m_bossModelHandle, m_rootFrame);

	//ボスの体の向き(大きさを取り除いた回転)＋付け根の位置//ずらしはこの軸で行う
	MATRIX body = MV1GetMatrix(m_bossModelHandle);
	for (int i = 0; i < 3; ++i)
	{
		VECTOR axis = VNorm(VGet(body.m[i][0], body.m[i][1], body.m[i][2]));
		body.m[i][0] = axis.x;
		body.m[i][1] = axis.y;
		body.m[i][2] = axis.z;
	}
	body.m[3][0] = rootWorld.m[3][0];
	body.m[3][1] = rootWorld.m[3][1];
	body.m[3][2] = rootWorld.m[3][2];

	//付け根を中心に、大きさ→回転→ずらし
	MATRIX offset = MGetScale(VGet(m_scale, m_scale, m_scale));
	offset = MMult(offset, MGetRotX(m_offsetRotDeg.x * kDegToRad));
	offset = MMult(offset, MGetRotY(m_offsetRotDeg.y * kDegToRad));
	offset = MMult(offset, MGetRotZ(m_offsetRotDeg.z * kDegToRad));
	offset = MMult(offset, MGetTranslate(m_offsetPos.ToDxLibVector()));

	//付け根のワールド行列を、体の軸の空間に移してからずらして戻す
	return MMult(MMult(MMult(rootWorld, MInverse(body)), offset), body);
}

MATRIX BossArm::CalcDetachedSpineMatrix() const
{
	//付け根を原点に戻して回してから、移動分ずらす
	const VECTOR root = VGet(m_detachSpineMatrix.m[3][0], m_detachSpineMatrix.m[3][1], m_detachSpineMatrix.m[3][2]);

	MATRIX mat = MMult(m_detachSpineMatrix, MGetTranslate(VScale(root, -1.0f)));
	//デタッチ中の回転を足す
	mat = MMult(mat, MGetRotY(m_detachSpinDeg * kDegToRad));

	//デタッチ中の移動を足す
	return MMult(mat, MGetTranslate(VAdd(root, m_detachMove.ToDxLibVector())));
}

MATRIX BossArm::CalcDrawSpineMatrix() const
{
	if (!m_isDetached)return CalcSpineMatrix();
	if (m_returnRate <= 0.0f)return CalcDetachedSpineMatrix();

	//戻り中//ボスは動いているので、付いた形は毎フレーム計算する
	return LerpMatrix(CalcDetachedSpineMatrix(), CalcSpineMatrix(), m_returnRate);
}

MATRIX BossArm::LerpMatrix(const MATRIX& a, const MATRIX& b, float t) const
{
	MATRIX out = MGetIdent();
	//軸ごとに向きと長さ(大きさ)を分けて補間する
	for (int i = 0; i < 3; ++i)
	{
		const VECTOR axisA = VGet(a.m[i][0], a.m[i][1], a.m[i][2]);
		const VECTOR axisB = VGet(b.m[i][0], b.m[i][1], b.m[i][2]);
		const float length = VSize(axisA) + (VSize(axisB) - VSize(axisA)) * t;
		const VECTOR axis = VScale(VNorm(VAdd(VScale(axisA, 1.0f - t), VScale(axisB, t))), length);
		out.m[i][0] = axis.x;
		out.m[i][1] = axis.y;
		out.m[i][2] = axis.z;
	}
	//位置
	for (int i = 0; i < 3; ++i)
	{
		out.m[3][i] = a.m[3][i] + (b.m[3][i] - a.m[3][i]) * t;
	}
	return out;
}
