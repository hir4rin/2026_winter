#include "PhaseAreaEditor.h"
#include "Battle/PhaseManager.h"
#include "Stage/Stage.h"
#include "Collision/Collider.h"
#include "DataLoader/DataManager.h"
#include "DxLib.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
	constexpr float kFloorCheckUp = 50.0f;//床を調べる線分の開始位置(中心からの上方向)
	constexpr float kFloorCheckDown = 10000.0f;//床を調べる線分の長さ
	constexpr float kMinFloorNormalY = 0.5f;//これ以上なら床
	constexpr float kMinHalfExtent = 1.0f;//Boxの半分の大きさの最小値
	constexpr float kMaxDragSpeed = 100.0f;
	constexpr int kNameBufferSize = 64;//名前の最大文字数(終端含む)
	constexpr float kListHeight = 180.0f;//エリア一覧の高さ
	constexpr float kDuplicateOffset = 50.0f;//複製したときにずらす量(X方向)

	constexpr size_t kMaxHistoryNum = 100;//Undoで戻れる最大回数
}

void PhaseAreaEditor::DrawWindow(PhaseManager& phaseMgr, const Stage& stage, Collider* player, const Vector3& camTarget)
{
	//前のフレームで頼まれたUndo/Redoを先に済ませる
	ApplyHistoryRequest(phaseMgr);

	const int areaNum = static_cast<int>(phaseMgr.GetAreas().size());
	if (m_selectedIndex >= areaNum)
	{
		m_selectedIndex = areaNum - 1;
	}

	//このフレームで編集する前の状態を覚えておく
	const HistoryEntry before = { phaseMgr.GetAreas(), m_selectedIndex };

	ImGui::SetNextWindowSize(ImVec2(420, 560), ImGuiCond_FirstUseEver);
	ImGui::Begin("Phase Area Editor");

	DrawHistoryButtons();

	//---- 保存・読み込み ----
	ImGui::SetNextItemWidth(100);
	ImGui::InputInt("Stage No.", &m_stageNumber);
	m_stageNumber = (std::max)(m_stageNumber, 1);
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		DataManager::GetInstance().LoadPhaseAreas(m_stageNumber);
		phaseMgr.SetAreas(DataManager::GetInstance().GetPhaseAreas());
		m_selectedIndex = -1;
		m_isDirty = false;
		SetStatus("Loaded " + DataManager::GetPhaseAreaFilePath(m_stageNumber));
	}
	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		if (DataManager::GetInstance().SavePhaseAreas(m_stageNumber, phaseMgr.GetAreas()))
		{
			m_isDirty = false;
			SetStatus("Saved " + DataManager::GetPhaseAreaFilePath(m_stageNumber));
		}
		else
		{
			SetStatus("Save FAILED: " + DataManager::GetPhaseAreaFilePath(m_stageNumber));
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

	//遷移表に書いてあるのにエリアが無い名前(名前を変えた・消したとき)
	for (const auto& name : phaseMgr.GetMissingAreaNames())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Missing (used in PhaseFlow): %s", name.c_str());
	}

	//---- 編集の設定 ----
	if (ImGui::CollapsingHeader("Edit Settings"))
	{
		ImGui::SliderFloat("Drag Speed", &m_dragSpeed, 0.1f, kMaxDragSpeed, "%.1f", ImGuiSliderFlags_Logarithmic);
		ImGui::Checkbox("Grid Snap (position)", &m_useGridSnap);
		if (m_useGridSnap)
		{
			ImGui::DragFloat("Grid Size", &m_gridSize, 1.0f, 1.0f, 1000.0f, "%.0f");
			m_gridSize = (std::max)(m_gridSize, 1.0f);
		}
	}

	ImGui::Separator();
	DrawAreaList(phaseMgr, player, camTarget);

	ImGui::Separator();
	DrawSelectedArea(phaseMgr, stage, player, camTarget);

	//ドラッグ中かどうかを見るので、End()より前に呼ぶ
	RecordHistory(phaseMgr, before);

	ImGui::End();
}

void PhaseAreaEditor::ApplyHistoryRequest(PhaseManager& phaseMgr)
{
	const bool isUndo = m_requestUndo;
	const bool isRedo = m_requestRedo;
	m_requestUndo = false;
	m_requestRedo = false;

	auto& from = isUndo ? m_undoHistory : m_redoHistory;
	auto& to = isUndo ? m_redoHistory : m_undoHistory;
	if ((!isUndo && !isRedo) || from.empty())return;

	//今の状態を反対側に積んでから、履歴の状態に戻す
	to.push_back({ phaseMgr.GetAreas(), m_selectedIndex });
	const HistoryEntry entry = from.back();
	from.pop_back();

	phaseMgr.SetAreas(entry.areas);
	m_selectedIndex = entry.selectedIndex;
	m_isContinuousEdit = false;
	m_isDirty = true;
	SetStatus(isUndo ? "Undo" : "Redo");
}

void PhaseAreaEditor::RecordHistory(const PhaseManager& phaseMgr, const HistoryEntry& before)
{
	const bool isAnyItemActive = ImGui::IsAnyItemActive();

	if (IsSameAreas(before.areas, phaseMgr.GetAreas()))
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

void PhaseAreaEditor::DrawHistoryButtons()
{
	ImGui::BeginDisabled(m_undoHistory.empty());
	if (ImGui::Button("Undo##area"))m_requestUndo = true;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(m_redoHistory.empty());
	if (ImGui::Button("Redo##area"))m_requestRedo = true;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextDisabled("history: %d", static_cast<int>(m_undoHistory.size()));

	//Ctrl+Z/Yは、このウィンドウを触っているときだけ(WallZoneEditorと同時に戻らないように)
	if (!ImGui::GetIO().WantTextInput && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z))m_requestUndo = true;
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y))m_requestRedo = true;
	}
}

void PhaseAreaEditor::DrawAreaList(PhaseManager& phaseMgr, Collider* player, const Vector3& camTarget)
{
	std::vector<PhaseArea> areas = phaseMgr.GetAreas();

	//---- 追加 ----
	DragVector3("New Half Size", m_newAreaHalfExtents, m_dragSpeed, kMinHalfExtent, 100000.0f);

	//指定した位置に新しいエリアを追加して選択する
	auto addArea = [&](const Vector3& pos)
		{
			PhaseArea area;
			area.name = MakeDefaultName(static_cast<int>(areas.size()));
			area.position = Snap(pos);
			area.halfExtents = m_newAreaHalfExtents;
			areas.push_back(area);
			phaseMgr.SetAreas(areas);
			m_selectedIndex = static_cast<int>(areas.size()) - 1;
			m_isDirty = true;
		};

	if (ImGui::Button("Add at Camera Target##area"))
	{
		addArea(camTarget);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add at Player##area") && player)
	{
		addArea(player->GetRigidBody().GetPos());//足元
	}

	//---- 一覧 ----
	ImGui::Text("Areas: %d", static_cast<int>(areas.size()));
	ImGui::BeginChild("AreaList", ImVec2(0, kListHeight), true);
	for (int i = 0; i < static_cast<int>(areas.size()); ++i)
	{
		char label[128];
		snprintf(label, sizeof(label), "%2d %s%s##area%d", i, areas[i].name.c_str(),
			phaseMgr.IsPlayerInArea(i) ? "  <- Player" : "", i);
		if (ImGui::Selectable(label, i == m_selectedIndex))
		{
			m_selectedIndex = i;
		}
	}
	ImGui::EndChild();

	//---- 複製・削除 ----
	const bool hasSelection = (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(areas.size()));
	ImGui::BeginDisabled(!hasSelection);
	if (ImGui::Button("Duplicate##area"))
	{
		PhaseArea area = areas[m_selectedIndex];
		area.name += "_copy";
		area.position.x += kDuplicateOffset;
		areas.push_back(area);
		phaseMgr.SetAreas(areas);
		m_selectedIndex = static_cast<int>(areas.size()) - 1;
		m_isDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete##area"))
	{
		areas.erase(areas.begin() + m_selectedIndex);
		phaseMgr.SetAreas(areas);
		m_selectedIndex = (std::min)(m_selectedIndex, static_cast<int>(areas.size()) - 1);
		m_isDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Deselect##area"))
	{
		m_selectedIndex = -1;
	}
	ImGui::EndDisabled();
}

void PhaseAreaEditor::DrawSelectedArea(PhaseManager& phaseMgr, const Stage& stage, Collider* player, const Vector3& camTarget)
{
	std::vector<PhaseArea> areas = phaseMgr.GetAreas();
	if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(areas.size()))
	{
		ImGui::TextDisabled("No area selected");
		return;
	}

	PhaseArea& area = areas[m_selectedIndex];
	bool isChanged = false;

	ImGui::Text("Selected: %d", m_selectedIndex);

	//名前(遷移表のparamに書く名前)
	char nameBuf[kNameBufferSize];
	strncpy_s(nameBuf, area.name.c_str(), _TRUNCATE);
	if (ImGui::InputText("Name##area", nameBuf, sizeof(nameBuf)))
	{
		area.name = nameBuf;
		isChanged = true;
	}

	//---- 位置 ----
	ImGui::SeparatorText("Position");
	if (DragVector3("Position##area", area.position, m_dragSpeed))
	{
		area.position = Snap(area.position);
		isChanged = true;
	}
	if (ImGui::Button("To Camera Target##area"))
	{
		area.position = Snap(camTarget);
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("To Player##area") && player)
	{
		area.position = Snap(player->GetRigidBody().GetPos());
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Drop to Floor##area"))
	{
		//底面を真下の床に合わせる
		float floorY = 0.0f;
		if (FindFloorY(stage, area.position, floorY))
		{
			area.position.y = floorY + area.halfExtents.y;
			isChanged = true;
		}
		else
		{
			SetStatus("No floor found below the area");
		}
	}

	//---- 大きさ ----
	ImGui::SeparatorText("Size");
	if (DragVector3("Half Size##area", area.halfExtents, m_dragSpeed, kMinHalfExtent, 100000.0f))
	{
		area.halfExtents.x = (std::max)(area.halfExtents.x, kMinHalfExtent);
		area.halfExtents.y = (std::max)(area.halfExtents.y, kMinHalfExtent);
		area.halfExtents.z = (std::max)(area.halfExtents.z, kMinHalfExtent);
		isChanged = true;
	}
	ImGui::TextDisabled("Full Size: %.1f x %.1f x %.1f",
		area.halfExtents.x * 2.0f, area.halfExtents.y * 2.0f, area.halfExtents.z * 2.0f);

	//---- 回転 ----
	ImGui::SeparatorText("Rotation");
	if (ImGui::SliderFloat("Rot Y (deg)##area", &area.rotYDeg, -180.0f, 180.0f, "%.1f"))
	{
		isChanged = true;
	}
	auto addRot = [&](float deg)
		{
			area.rotYDeg += deg;
			//-180~180に収める
			while (area.rotYDeg > 180.0f)area.rotYDeg -= 360.0f;
			while (area.rotYDeg < -180.0f)area.rotYDeg += 360.0f;
			isChanged = true;
		};
	if (ImGui::Button("-90##area")) addRot(-90.0f);
	ImGui::SameLine();
	if (ImGui::Button("-15##area")) addRot(-15.0f);
	ImGui::SameLine();
	if (ImGui::Button("0##area"))
	{
		area.rotYDeg = 0.0f;
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("+15##area")) addRot(15.0f);
	ImGui::SameLine();
	if (ImGui::Button("+90##area")) addRot(90.0f);

	//変更があったらその場でPhaseManagerに当てはめる
	if (isChanged)
	{
		phaseMgr.SetAreas(areas);
		m_isDirty = true;
	}
}

void PhaseAreaEditor::DrawAreas(const PhaseManager& phaseMgr) const
{
	phaseMgr.DrawAreas(m_selectedIndex);
}

Vector3 PhaseAreaEditor::Snap(const Vector3& pos) const
{
	if (!m_useGridSnap)return pos;
	auto snap = [&](float v) { return std::round(v / m_gridSize) * m_gridSize; };
	return Vector3(snap(pos.x), snap(pos.y), snap(pos.z));
}

bool PhaseAreaEditor::FindFloorY(const Stage& stage, const Vector3& pos, float& outFloorY) const
{
	const Vector3 start = pos + Vector3(0.0f, kFloorCheckUp, 0.0f);
	const Vector3 end = pos - Vector3(0.0f, kFloorCheckDown, 0.0f);

	auto hit = MV1CollCheck_Line(stage.GetStageModelHandle(), -1, start.ToDxLibVector(), end.ToDxLibVector());
	if (!hit.HitFlag)return false;
	if (hit.Normal.y < kMinFloorNormalY)return false;

	outFloorY = hit.HitPosition.y;
	return true;
}

bool PhaseAreaEditor::DragVector3(const char* label, Vector3& value, float speed, float min, float max)
{
	float v[3] = { value.x, value.y, value.z };
	if (ImGui::DragFloat3(label, v, speed, min, max, "%.1f"))
	{
		value = Vector3(v[0], v[1], v[2]);
		return true;
	}
	return false;
}

bool PhaseAreaEditor::IsSameVector3(const Vector3& a, const Vector3& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool PhaseAreaEditor::IsSameAreas(const std::vector<PhaseArea>& a, const std::vector<PhaseArea>& b)
{
	if (a.size() != b.size())return false;
	for (size_t i = 0; i < a.size(); ++i)
	{
		if (a[i].name != b[i].name ||
			!IsSameVector3(a[i].position, b[i].position) ||
			!IsSameVector3(a[i].halfExtents, b[i].halfExtents) ||
			a[i].rotYDeg != b[i].rotYDeg)
		{
			return false;
		}
	}
	return true;
}

std::string PhaseAreaEditor::MakeDefaultName(int index)
{
	char buf[kNameBufferSize];
	snprintf(buf, sizeof(buf), "Area_%d", index);
	return buf;
}
