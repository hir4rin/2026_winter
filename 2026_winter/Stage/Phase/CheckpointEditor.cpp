#include "CheckpointEditor.h"
#include "Stage/Stage.h"
#include "Collision/Collider.h"
#include "DataLoader/DataManager.h"
#include "Battle/CheckpointManager.h"
#include "DxLib.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
	constexpr float kFloorCheckUp = 50.0f;//床を調べる線分の開始位置(上方向)
	constexpr float kFloorCheckDown = 10000.0f;//床を調べる線分の長さ
	constexpr float kMinFloorNormalY = 0.5f;//これ以上なら床
	constexpr float kMinRadius = 10.0f;
	constexpr float kMaxRadius = 5000.0f;
	constexpr float kMaxDragSpeed = 100.0f;
	constexpr int kNameBufferSize = 64;//名前の最大文字数(終端含む)
	constexpr float kListHeight = 180.0f;//一覧の高さ
	constexpr float kDuplicateOffset = 50.0f;//複製したときにずらす量(X方向)
	constexpr size_t kMaxHistoryNum = 100;//Undoで戻れる最大回数

	//描画
	constexpr int kCircleDiv = 32;//円の分割数
	constexpr float kDrawHeight = 200.0f;//筒の高さ
	constexpr float kDrawLift = 2.0f;//床に埋まらないように少し上げる
	constexpr float kArrowLengthRate = 1.5f;//向きの矢印の長さ(半径に対して)
	constexpr float kArrowHeadLength = 40.0f;
	constexpr float kLabelHeight = 230.0f;//名前を出す高さ
}

void CheckpointEditor::DrawWindow(CheckpointManager& checkpointMgr, const Stage& stage, Collider* player, const Vector3& camPos, const Vector3& camTarget)
{
	//このフレームはManagerのデータを写して編集し、最後に変わっていたら戻す
	m_checkpoints = checkpointMgr.GetCheckpoints();

	//前のフレームで頼まれたUndo/Redoを先に済ませる
	ApplyHistoryRequest();

	const int num = static_cast<int>(m_checkpoints.size());
	if (m_selectedIndex >= num)
	{
		m_selectedIndex = num - 1;
	}

	//このフレームで編集する前の状態を覚えておく
	const HistoryEntry before = { m_checkpoints, m_selectedIndex };

	ImGui::SetNextWindowSize(ImVec2(420, 600), ImGuiCond_FirstUseEver);
	ImGui::Begin("Checkpoint Editor");

	DrawHistoryButtons();

	//---- 保存・読み込み ----
	ImGui::SetNextItemWidth(100);
	ImGui::InputInt("Stage No.", &m_stageNumber);
	m_stageNumber = (std::max)(m_stageNumber, 1);
	ImGui::SameLine();
	if (ImGui::Button("Load##cp"))
	{
		DataManager::GetInstance().LoadCheckpoints(m_stageNumber);
		m_checkpoints = DataManager::GetInstance().GetCheckpoints();
		m_selectedIndex = -1;
		m_isDirty = false;
		SetStatus("Loaded " + DataManager::GetCheckpointFilePath(m_stageNumber));
	}
	ImGui::SameLine();
	if (ImGui::Button("Save##cp"))
	{
		if (DataManager::GetInstance().SaveCheckpoints(m_stageNumber, m_checkpoints))
		{
			m_isDirty = false;
			SetStatus("Saved " + DataManager::GetCheckpointFilePath(m_stageNumber));
		}
		else
		{
			SetStatus("Save FAILED: " + DataManager::GetCheckpointFilePath(m_stageNumber));
		}
	}
	if (m_isDirty)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "* Unsaved changes");
	}
	if (!m_statusMessage.empty())
	{
		ImGui::TextWrapped("%s", m_statusMessage.c_str());
	}

	//---- 編集の設定 ----
	if (ImGui::CollapsingHeader("Edit Settings##cp"))
	{
		ImGui::SliderFloat("Drag Speed##cp", &m_dragSpeed, 0.1f, kMaxDragSpeed, "%.1f", ImGuiSliderFlags_Logarithmic);
		ImGui::Checkbox("Grid Snap (position)##cp", &m_useGridSnap);
		if (m_useGridSnap)
		{
			ImGui::DragFloat("Grid Size##cp", &m_gridSize, 1.0f, 1.0f, 1000.0f, "%.0f");
			m_gridSize = (std::max)(m_gridSize, 1.0f);
		}
		ImGui::Checkbox("Drop to Floor on Add##cp", &m_dropOnAdd);
	}

	ImGui::Separator();
	DrawList(checkpointMgr, stage, player, camTarget);

	ImGui::Separator();
	DrawSelected(stage, player, camPos, camTarget);

	//ドラッグ中かどうかを見るので、End()より前に呼ぶ
	RecordHistory(before);

	ImGui::End();

	//変更があったらその場でManagerに当てはめる
	if (!IsSameCheckpoints(m_checkpoints, checkpointMgr.GetCheckpoints()))
	{
		checkpointMgr.SetCheckpoints(m_checkpoints);
	}
}

void CheckpointEditor::ApplyHistoryRequest()
{
	const bool isUndo = m_requestUndo;
	const bool isRedo = m_requestRedo;
	m_requestUndo = false;
	m_requestRedo = false;

	auto& from = isUndo ? m_undoHistory : m_redoHistory;
	auto& to = isUndo ? m_redoHistory : m_undoHistory;
	if ((!isUndo && !isRedo) || from.empty())return;

	//今の状態を反対側に積んでから、履歴の状態に戻す
	to.push_back({ m_checkpoints, m_selectedIndex });
	const HistoryEntry entry = from.back();
	from.pop_back();

	m_checkpoints = entry.checkpoints;
	m_selectedIndex = entry.selectedIndex;
	m_isContinuousEdit = false;
	m_isDirty = true;
	SetStatus(isUndo ? "Undo" : "Redo");
}

void CheckpointEditor::RecordHistory(const HistoryEntry& before)
{
	const bool isAnyItemActive = ImGui::IsAnyItemActive();

	if (IsSameCheckpoints(before.checkpoints, m_checkpoints))
	{
		if (!isAnyItemActive)m_isContinuousEdit = false;
		return;
	}

	//ドラッグ・文字入力の途中の変更は、触り始めた時の1回だけ積む
	if (!m_isContinuousEdit)
	{
		m_undoHistory.push_back(before);
		if (m_undoHistory.size() > kMaxHistoryNum)
		{
			m_undoHistory.erase(m_undoHistory.begin());
		}
		m_redoHistory.clear();
	}
	m_isContinuousEdit = isAnyItemActive;
}

void CheckpointEditor::DrawHistoryButtons()
{
	ImGui::BeginDisabled(m_undoHistory.empty());
	if (ImGui::Button("Undo##cp"))m_requestUndo = true;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(m_redoHistory.empty());
	if (ImGui::Button("Redo##cp"))m_requestRedo = true;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextDisabled("history: %d", static_cast<int>(m_undoHistory.size()));

	//Ctrl+Z/Yは、このウィンドウを触っているときだけ
	if (!ImGui::GetIO().WantTextInput && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z))m_requestUndo = true;
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y))m_requestRedo = true;
	}
}

void CheckpointEditor::DrawList(const CheckpointManager& checkpointMgr, const Stage& stage, Collider* player, const Vector3& camTarget)
{
	//指定した位置に追加して選択する
	auto add = [&](const Vector3& pos)
		{
			CheckpointData cp;
			cp.name = MakeDefaultName(static_cast<int>(m_checkpoints.size()));
			cp.position = Snap(pos);
			float floorY = 0.0f;
			if (m_dropOnAdd && FindFloorY(stage, cp.position, floorY))cp.position.y = floorY;
			//フェーズは最後の続きにしておく
			if (!m_checkpoints.empty())cp.phase = m_checkpoints.back().phase + 1;
			m_checkpoints.push_back(cp);
			m_selectedIndex = static_cast<int>(m_checkpoints.size()) - 1;
			m_isDirty = true;
		};

	if (ImGui::Button("Add at Camera Target##cp"))
	{
		add(camTarget);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add at Player##cp") && player)
	{
		add(player->GetRigidBody().GetPos());//足元
	}

	//---- 一覧 ----
	ImGui::Text("Checkpoints: %d  (last: %d)", static_cast<int>(m_checkpoints.size()), checkpointMgr.GetLastIndex());
	ImGui::BeginChild("CheckpointList", ImVec2(0, kListHeight), true);
	for (int i = 0; i < static_cast<int>(m_checkpoints.size()); ++i)
	{
		//踏んだ物・次の物の印
		const char* mark = checkpointMgr.IsActivated(i) ? "done" : (i == checkpointMgr.GetNextIndex() ? "NEXT" : "    ");
		char label[128];
		snprintf(label, sizeof(label), "%2d %s [ph%2d] %s%s##cp%d", i, mark, m_checkpoints[i].phase, m_checkpoints[i].name.c_str(),
			checkpointMgr.IsPlayerInside(i) ? "  <- Player" : "", i);
		if (ImGui::Selectable(label, i == m_selectedIndex))
		{
			m_selectedIndex = i;
		}
	}
	ImGui::EndChild();

	//---- 並べ替え・複製・削除 ----
	const int num = static_cast<int>(m_checkpoints.size());
	const bool hasSelection = (m_selectedIndex >= 0 && m_selectedIndex < num);
	ImGui::BeginDisabled(!hasSelection);
	ImGui::BeginDisabled(m_selectedIndex <= 0);
	if (ImGui::ArrowButton("##cpUp", ImGuiDir_Up))
	{
		std::swap(m_checkpoints[m_selectedIndex], m_checkpoints[m_selectedIndex - 1]);
		--m_selectedIndex;
		m_isDirty = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(m_selectedIndex >= num - 1);
	if (ImGui::ArrowButton("##cpDown", ImGuiDir_Down))
	{
		std::swap(m_checkpoints[m_selectedIndex], m_checkpoints[m_selectedIndex + 1]);
		++m_selectedIndex;
		m_isDirty = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Duplicate##cp"))
	{
		CheckpointData cp = m_checkpoints[m_selectedIndex];
		cp.name += "_copy";
		cp.position.x += kDuplicateOffset;
		m_checkpoints.push_back(cp);
		m_selectedIndex = static_cast<int>(m_checkpoints.size()) - 1;
		m_isDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete##cp"))
	{
		m_checkpoints.erase(m_checkpoints.begin() + m_selectedIndex);
		m_selectedIndex = (std::min)(m_selectedIndex, static_cast<int>(m_checkpoints.size()) - 1);
		m_isDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Deselect##cp"))
	{
		m_selectedIndex = -1;
	}
	ImGui::EndDisabled();
}

void CheckpointEditor::DrawSelected(const Stage& stage, Collider* player, const Vector3& camPos, const Vector3& camTarget)
{
	if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_checkpoints.size()))
	{
		ImGui::TextDisabled("No checkpoint selected");
		return;
	}

	CheckpointData& cp = m_checkpoints[m_selectedIndex];
	bool isChanged = false;

	ImGui::Text("Selected: %d", m_selectedIndex);

	char nameBuf[kNameBufferSize];
	strncpy_s(nameBuf, cp.name.c_str(), _TRUNCATE);
	if (ImGui::InputText("Name##cp", nameBuf, sizeof(nameBuf)))
	{
		cp.name = nameBuf;
		isChanged = true;
	}
	if (ImGui::InputInt("Phase##cp", &cp.phase))
	{
		cp.phase = (std::max)(cp.phase, 0);
		isChanged = true;
	}

	//---- 位置 ----
	ImGui::SeparatorText("Position (floor center)");
	if (DragVector3("Position##cp", cp.position, m_dragSpeed))
	{
		cp.position = Snap(cp.position);
		isChanged = true;
	}
	if (ImGui::Button("To Camera Target##cp"))
	{
		cp.position = Snap(camTarget);
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("To Player##cp") && player)
	{
		cp.position = Snap(player->GetRigidBody().GetPos());
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Drop to Floor##cp"))
	{
		float floorY = 0.0f;
		if (FindFloorY(stage, cp.position, floorY))
		{
			cp.position.y = floorY;
			isChanged = true;
		}
		else
		{
			SetStatus("No floor found below the checkpoint");
		}
	}

	//---- 範囲 ----
	ImGui::SeparatorText("Activate Range");
	if (ImGui::DragFloat("Radius##cp", &cp.radius, m_dragSpeed, kMinRadius, kMaxRadius, "%.1f"))
	{
		cp.radius = std::clamp(cp.radius, kMinRadius, kMaxRadius);
		isChanged = true;
	}

	//---- 向き(戻ったときに向く方向) ----
	ImGui::SeparatorText("Respawn Facing");
	if (ImGui::SliderFloat("Rot Y (deg)##cp", &cp.rotYDeg, -180.0f, 180.0f, "%.1f"))
	{
		isChanged = true;
	}
	auto addRot = [&](float deg)
		{
			cp.rotYDeg = WrapDeg(cp.rotYDeg + deg);
			isChanged = true;
		};
	if (ImGui::Button("-90##cp")) addRot(-90.0f);
	ImGui::SameLine();
	if (ImGui::Button("-15##cp")) addRot(-15.0f);
	ImGui::SameLine();
	if (ImGui::Button("0##cp"))
	{
		cp.rotYDeg = 0.0f;
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("+15##cp")) addRot(15.0f);
	ImGui::SameLine();
	if (ImGui::Button("+90##cp")) addRot(90.0f);
	if (ImGui::Button("Face Camera Direction##cp"))
	{
		//カメラが見ている向きにする
		const Vector3 dir = camTarget - camPos;
		if (dir.x != 0.0f || dir.z != 0.0f)
		{
			cp.rotYDeg = WrapDeg(std::atan2(dir.x, dir.z) * 180.0f / DX_PI_F);
			isChanged = true;
		}
	}

	//---- 確認 ----
	ImGui::SeparatorText("Test");
	if (ImGui::Button("Warp Player Here##cp") && player)
	{
		player->GetRigidBody().SetPos(cp.position);
		player->GetRigidBody().ResetVel();
	}

	if (isChanged)
	{
		m_isDirty = true;
	}
}

void CheckpointEditor::DrawCheckpoints(const CheckpointManager& checkpointMgr) const
{
	const auto& checkpoints = checkpointMgr.GetCheckpoints();
	for (int i = 0; i < static_cast<int>(checkpoints.size()); i++)
	{
		const CheckpointData& cp = checkpoints[i];
		unsigned int color;
		if (i == m_selectedIndex)
		{
			color = GetColor(255, 255, 255);//選択中は白
		}
		else if (checkpointMgr.IsPlayerInside(i))
		{
			color = GetColor(255, 0, 0);//中にいるときは赤
		}
		else if (checkpointMgr.IsActivated(i))
		{
			color = GetColor(110, 110, 110);//踏んだ物は灰
		}
		else if (i == checkpointMgr.GetNextIndex())
		{
			color = GetColor(120, 200, 255);//次は薄い青
		}
		else
		{
			color = GetColor(40, 90, 200);//その先は濃い青
		}

		//範囲の筒(下の円・上の円・縦の線)
		const Vector3 base = cp.position + Vector3(0.0f, kDrawLift, 0.0f);
		const Vector3 up = Vector3(0.0f, kDrawHeight, 0.0f);
		for (int d = 0; d < kCircleDiv; d++)
		{
			const float a0 = DX_TWO_PI_F * d / kCircleDiv;
			const float a1 = DX_TWO_PI_F * (d + 1) / kCircleDiv;
			const Vector3 p0 = base + Vector3(std::sin(a0), 0.0f, std::cos(a0)) * cp.radius;
			const Vector3 p1 = base + Vector3(std::sin(a1), 0.0f, std::cos(a1)) * cp.radius;
			DrawLine3D(p0.ToDxLibVector(), p1.ToDxLibVector(), color);
			DrawLine3D((p0 + up).ToDxLibVector(), (p1 + up).ToDxLibVector(), color);
			if (d % (kCircleDiv / 4) == 0)DrawLine3D(p0.ToDxLibVector(), (p0 + up).ToDxLibVector(), color);
		}

		//向きの矢印
		const float rad = cp.rotYDeg * DX_PI_F / 180.0f;
		const Vector3 forward(std::sin(rad), 0.0f, std::cos(rad));
		const Vector3 right(forward.z, 0.0f, -forward.x);
		const Vector3 tip = base + forward * (cp.radius * kArrowLengthRate);
		DrawLine3D(base.ToDxLibVector(), tip.ToDxLibVector(), color);
		DrawLine3D(tip.ToDxLibVector(), (tip - forward * kArrowHeadLength + right * kArrowHeadLength * 0.5f).ToDxLibVector(), color);
		DrawLine3D(tip.ToDxLibVector(), (tip - forward * kArrowHeadLength - right * kArrowHeadLength * 0.5f).ToDxLibVector(), color);

		//名前とフェーズ(カメラの前にあるときだけ)
		const VECTOR screen = ConvWorldPosToScreenPos((cp.position + Vector3(0.0f, kLabelHeight, 0.0f)).ToDxLibVector());
		if (screen.z > 0.0f && screen.z < 1.0f)
		{
			DrawFormatString(static_cast<int>(screen.x), static_cast<int>(screen.y), color, "%s (ph%d)", cp.name.c_str(), cp.phase);
		}
	}
}

Vector3 CheckpointEditor::Snap(const Vector3& pos) const
{
	if (!m_useGridSnap)return pos;
	auto snap = [&](float v) { return std::round(v / m_gridSize) * m_gridSize; };
	return Vector3(snap(pos.x), snap(pos.y), snap(pos.z));
}

bool CheckpointEditor::FindFloorY(const Stage& stage, const Vector3& pos, float& outFloorY) const
{
	const Vector3 start = pos + Vector3(0.0f, kFloorCheckUp, 0.0f);
	const Vector3 end = pos - Vector3(0.0f, kFloorCheckDown, 0.0f);

	auto hit = MV1CollCheck_Line(stage.GetStageModelHandle(), -1, start.ToDxLibVector(), end.ToDxLibVector());
	if (!hit.HitFlag)return false;
	if (hit.Normal.y < kMinFloorNormalY)return false;

	outFloorY = hit.HitPosition.y;
	return true;
}

bool CheckpointEditor::DragVector3(const char* label, Vector3& value, float speed)
{
	float v[3] = { value.x, value.y, value.z };
	if (ImGui::DragFloat3(label, v, speed, 0.0f, 0.0f, "%.1f"))
	{
		value = Vector3(v[0], v[1], v[2]);
		return true;
	}
	return false;
}

bool CheckpointEditor::IsSameVector3(const Vector3& a, const Vector3& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool CheckpointEditor::IsSameCheckpoints(const std::vector<CheckpointData>& a, const std::vector<CheckpointData>& b)
{
	if (a.size() != b.size())return false;
	for (size_t i = 0; i < a.size(); ++i)
	{
		if (a[i].name != b[i].name ||
			!IsSameVector3(a[i].position, b[i].position) ||
			a[i].rotYDeg != b[i].rotYDeg ||
			a[i].phase != b[i].phase ||
			a[i].radius != b[i].radius)
		{
			return false;
		}
	}
	return true;
}

float CheckpointEditor::WrapDeg(float deg)
{
	while (deg > 180.0f)deg -= 360.0f;
	while (deg < -180.0f)deg += 360.0f;
	return deg;
}

std::string CheckpointEditor::MakeDefaultName(int index)
{
	char buf[kNameBufferSize];
	snprintf(buf, sizeof(buf), "CP_%d", index);
	return buf;
}
