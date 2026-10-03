#include "WallZoneEditor.h"
#include "Stage.h"
#include "../DataLoader/DataManager.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
	constexpr float kRayLength = 20000.0f;//カメラから飛ばすレイの長さ
	constexpr float kFloorCheckUp = 50.0f;//床を調べる線分の開始位置(中心からの上方向)
	constexpr float kFloorCheckDown = 10000.0f;//床を調べる線分の長さ
	constexpr float kMinFloorNormalY = 0.5f;//これ以上なら床
	constexpr float kMinHalfExtent = 1.0f;//Boxの半分の大きさの最小値
	constexpr float kMaxDragSpeed = 100.0f;
	constexpr int kNameBufferSize = 64;//名前の最大文字数(終端含む)
	constexpr float kListHeight = 180.0f;//ゾーン一覧の高さ
	constexpr float kDuplicateOffset = 50.0f;//複製したときにずらす量(X方向)

	const char* const kTypeNames[] = { "WallKickZone", "WallRunZone" };

	//ImGuiのコンボボックスのindexとColRoleの変換
	int TypeToIndex(Collider::ColRole type)
	{
		return (type == Collider::ColRole::WallRunZone) ? 1 : 0;
	}
	Collider::ColRole IndexToType(int index)
	{
		return (index == 1) ? Collider::ColRole::WallRunZone : Collider::ColRole::WallKickZone;
	}
	const char* TypeShortName(Collider::ColRole type)
	{
		return (type == Collider::ColRole::WallRunZone) ? "Run " : "Kick";
	}

	//ImGuiでVector3をまとめて編集するための変換
	bool DragVector3(const char* label, Vector3& value, float speed, float min = 0.0f, float max = 0.0f)
	{
		float v[3] = { value.x, value.y, value.z };
		if (ImGui::DragFloat3(label, v, speed, min, max, "%.1f"))
		{
			value = Vector3(v[0], v[1], v[2]);
			return true;
		}
		return false;
	}

	constexpr size_t kMaxHistoryNum = 100;//Undoで戻れる最大回数

	bool IsSameVector3(const Vector3& a, const Vector3& b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	//2つのゾーン一覧が全く同じか(Undo履歴を積むかどうかの判定に使う)
	bool IsSameZones(const std::vector<WallZoneData>& a, const std::vector<WallZoneData>& b)
	{
		if (a.size() != b.size())return false;
		for (size_t i = 0; i < a.size(); ++i)
		{
			if (a[i].type != b[i].type ||
				a[i].name != b[i].name ||
				!IsSameVector3(a[i].position, b[i].position) ||
				!IsSameVector3(a[i].halfExtents, b[i].halfExtents) ||
				a[i].rotYDeg != b[i].rotYDeg ||
				a[i].isActive != b[i].isActive)
			{
				return false;
			}
		}
		return true;
	}

	//ゾーンの名前を自動で付ける
	std::string MakeDefaultName(Collider::ColRole type, int index)
	{
		char buf[kNameBufferSize];
		snprintf(buf, sizeof(buf), "%s_%d", (type == Collider::ColRole::WallRunZone) ? "Run" : "Kick", index);
		return buf;
	}
}

void WallZoneEditor::DrawWindow(Stage& stage, const Collider* player, const Vector3& camPos, const Vector3& camTarget)
{
	//前のフレームで頼まれたUndo/Redoを先に済ませる
	ApplyHistoryRequest(stage);

	//ゾーンが消えていたら選択を外す(Loadし直したときなど)
	if (m_selectedIndex >= stage.GetWallZoneNum())
	{
		m_selectedIndex = stage.GetWallZoneNum() - 1;
	}

	//このフレームで編集する前の状態を覚えておく(変わっていたらこれを履歴に積む)
	const HistoryEntry before = { stage.GetAllWallZoneData(), m_selectedIndex };

	ImGui::SetNextWindowSize(ImVec2(420, 640), ImGuiCond_FirstUseEver);
	ImGui::Begin("Wall Zone Editor");

	ImGui::TextDisabled("Camera: Arrow=move RB/LB=up/down RStick=look");
	ImGui::TextDisabled("Start(P)=pause menu (return to game from there)");
	ImGui::Separator();

	DrawHistoryButtons();

	//---- 保存・読み込み ----
	ImGui::SetNextItemWidth(100);
	ImGui::InputInt("Stage No.", &m_stageNumber);
	m_stageNumber = (std::max)(m_stageNumber, 1);
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		stage.LoadWallZones(m_stageNumber);
		m_selectedIndex = -1;
		m_isDirty = false;
		SetStatus("Loaded " + DataManager::GetWallZoneFilePath(m_stageNumber));
	}
	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		if (stage.SaveWallZones(m_stageNumber))
		{
			m_isDirty = false;
			SetStatus("Saved " + DataManager::GetWallZoneFilePath(m_stageNumber));
		}
		else
		{
			SetStatus("Save FAILED: " + DataManager::GetWallZoneFilePath(m_stageNumber));
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
	DrawZoneList(stage, player, camPos, camTarget);

	ImGui::Separator();
	DrawSelectedZone(stage, player, camPos, camTarget);

	//ドラッグ中かどうかを見るので、End()より前に呼ぶ
	RecordHistory(stage, before);

	ImGui::End();
}

void WallZoneEditor::ApplyHistoryRequest(Stage& stage)
{
	const bool isUndo = m_requestUndo;
	const bool isRedo = m_requestRedo;
	m_requestUndo = false;
	m_requestRedo = false;

	//取り出す側と、今の状態を積む側
	auto& from = isUndo ? m_undoHistory : m_redoHistory;
	auto& to = isUndo ? m_redoHistory : m_undoHistory;
	if ((!isUndo && !isRedo) || from.empty())return;

	//今の状態を反対側に積んでから、履歴の状態に戻す
	to.push_back({ stage.GetAllWallZoneData(), m_selectedIndex });
	const HistoryEntry entry = from.back();
	from.pop_back();

	stage.SetAllWallZoneData(entry.zones);
	m_selectedIndex = entry.selectedIndex;
	m_isContinuousEdit = false;
	m_isDirty = true;
	SetStatus(isUndo ? "Undo" : "Redo");
}

void WallZoneEditor::RecordHistory(const Stage& stage, const HistoryEntry& before)
{
	const bool isAnyItemActive = ImGui::IsAnyItemActive();

	if (IsSameZones(before.zones, stage.GetAllWallZoneData()))
	{
		//変更が無く、何も触っていなければ、ひと続きの編集は終わり
		if (!isAnyItemActive)m_isContinuousEdit = false;
		return;
	}

	//ドラッグ・文字入力の途中の変更は、触り始めた時の1回だけ積む
	if (!m_isContinuousEdit)
	{
		m_undoHistory.push_back(before);
		if (m_undoHistory.size() > kMaxHistoryNum)
		{
			m_undoHistory.erase(m_undoHistory.begin());//古いものから捨てる
		}
		m_redoHistory.clear();//新しく編集したら、やり直しはできなくなる
	}
	m_isContinuousEdit = isAnyItemActive;
}

void WallZoneEditor::DrawHistoryButtons()
{
	ImGui::BeginDisabled(m_undoHistory.empty());
	if (ImGui::Button("Undo (Ctrl+Z)"))m_requestUndo = true;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(m_redoHistory.empty());
	if (ImGui::Button("Redo (Ctrl+Y)"))m_requestRedo = true;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextDisabled("history: %d", static_cast<int>(m_undoHistory.size()));

	//文字入力中のCtrl+Zはテキストボックス側のUndoに任せる
	if (!ImGui::GetIO().WantTextInput)
	{
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z))m_requestUndo = true;
		if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y))m_requestRedo = true;
	}
}

void WallZoneEditor::DrawZoneList(Stage& stage, const Collider* player, const Vector3& camPos, const Vector3& camTarget)
{
	//---- 追加 ----
	int newTypeIndex = TypeToIndex(m_newZoneType);
	ImGui::SetNextItemWidth(160);
	if (ImGui::Combo("New Type", &newTypeIndex, kTypeNames, IM_ARRAYSIZE(kTypeNames)))
	{
		m_newZoneType = IndexToType(newTypeIndex);
	}
	DragVector3("New Half Size", m_newZoneHalfExtents, m_dragSpeed, kMinHalfExtent, 100000.0f);

	//指定した位置に新しいゾーンを追加して選択する
	auto addZone = [&](const Vector3& pos, float rotYDeg)
		{
			WallZoneData data;
			data.type = m_newZoneType;
			data.name = MakeDefaultName(m_newZoneType, stage.GetWallZoneNum());
			data.position = Snap(pos);
			data.halfExtents = m_newZoneHalfExtents;
			data.rotYDeg = rotYDeg;
			m_selectedIndex = stage.AddWallZone(data);
			m_isDirty = true;
		};

	if (ImGui::Button("Add at Camera Target"))
	{
		addZone(camTarget, 0.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("Add at Player") && player)
	{
		addZone(player->GetWorldPos(), 0.0f);
	}
	if (ImGui::Button("Add on Wall (camera ray)"))
	{
		Vector3 hitPos, normal;
		if (RaycastFromCamera(stage, camPos, camTarget, hitPos, normal))
		{
			//ローカルの+Zが壁の法線を向くように回す
			addZone(hitPos, std::atan2(normal.x, normal.z) * 180.0f / DX_PI_F);
		}
		else
		{
			SetStatus("Camera ray did not hit a wall");
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Casts a ray from the camera to the screen center.\nThe zone is placed on the hit wall, rotated so its local Z faces out of the wall.");
	}

	//---- 一覧 ----
	ImGui::Text("Zones: %d", stage.GetWallZoneNum());
	ImGui::BeginChild("ZoneList", ImVec2(0, kListHeight), true);
	for (int i = 0; i < stage.GetWallZoneNum(); ++i)
	{
		const WallZoneData& data = stage.GetWallZoneData(i);
		const bool isPlayerInside = player && stage.IsWallZoneOverlap(i, *player);

		char label[128];
		snprintf(label, sizeof(label), "%2d [%s] %s%s%s##zone%d", i, TypeShortName(data.type), data.name.c_str(),
			data.isActive ? "" : " (off)", isPlayerInside ? "  <- Player" : "", i);
		if (ImGui::Selectable(label, i == m_selectedIndex))
		{
			m_selectedIndex = i;
		}
	}
	ImGui::EndChild();

	//---- 複製・削除 ----
	const bool hasSelection = (m_selectedIndex >= 0 && m_selectedIndex < stage.GetWallZoneNum());
	ImGui::BeginDisabled(!hasSelection);
	if (ImGui::Button("Duplicate"))
	{
		WallZoneData data = stage.GetWallZoneData(m_selectedIndex);
		data.name += "_copy";
		data.position.x += kDuplicateOffset;
		m_selectedIndex = stage.AddWallZone(data);
		m_isDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		stage.RemoveWallZone(m_selectedIndex);
		m_selectedIndex = (std::min)(m_selectedIndex, stage.GetWallZoneNum() - 1);
		m_isDirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Deselect"))
	{
		m_selectedIndex = -1;
	}
	ImGui::EndDisabled();
}

void WallZoneEditor::DrawSelectedZone(Stage& stage, const Collider* player, const Vector3& camPos, const Vector3& camTarget)
{
	if (m_selectedIndex < 0 || m_selectedIndex >= stage.GetWallZoneNum())
	{
		ImGui::TextDisabled("No zone selected");
		return;
	}

	WallZoneData data = stage.GetWallZoneData(m_selectedIndex);
	bool isChanged = false;

	ImGui::Text("Selected: %d", m_selectedIndex);

	//名前
	char nameBuf[kNameBufferSize];
	strncpy_s(nameBuf, data.name.c_str(), _TRUNCATE);
	if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
	{
		data.name = nameBuf;
		isChanged = true;
	}

	//種類(タグ)
	int typeIndex = TypeToIndex(data.type);
	if (ImGui::Combo("Type", &typeIndex, kTypeNames, IM_ARRAYSIZE(kTypeNames)))
	{
		data.type = IndexToType(typeIndex);
		isChanged = true;
	}

	//有効/無効
	isChanged |= ImGui::Checkbox("Active", &data.isActive);

	//---- 位置 ----
	ImGui::SeparatorText("Position");
	if (DragVector3("Position", data.position, m_dragSpeed))
	{
		data.position = Snap(data.position);
		isChanged = true;
	}
	if (ImGui::Button("To Camera Target"))
	{
		data.position = Snap(camTarget);
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("To Player") && player)
	{
		data.position = Snap(player->GetWorldPos());
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Drop to Floor"))
	{
		//底面を真下の床に合わせる
		float floorY = 0.0f;
		if (FindFloorY(stage, data.position, floorY))
		{
			data.position.y = floorY + data.halfExtents.y;
			isChanged = true;
		}
		else
		{
			SetStatus("No floor found below the zone");
		}
	}
	if (ImGui::Button("Align to Wall (camera ray)"))
	{
		//カメラの向きの壁に移動し、ローカルの+Zが壁の法線を向くように回す
		Vector3 hitPos, normal;
		if (RaycastFromCamera(stage, camPos, camTarget, hitPos, normal))
		{
			data.position = Snap(hitPos);
			data.rotYDeg = std::atan2(normal.x, normal.z) * 180.0f / DX_PI_F;
			isChanged = true;
		}
		else
		{
			SetStatus("Camera ray did not hit a wall");
		}
	}

	//---- 大きさ ----
	ImGui::SeparatorText("Size");
	if (DragVector3("Half Size", data.halfExtents, m_dragSpeed, kMinHalfExtent, 100000.0f))
	{
		data.halfExtents.x = (std::max)(data.halfExtents.x, kMinHalfExtent);
		data.halfExtents.y = (std::max)(data.halfExtents.y, kMinHalfExtent);
		data.halfExtents.z = (std::max)(data.halfExtents.z, kMinHalfExtent);
		isChanged = true;
	}
	ImGui::TextDisabled("Full Size: %.1f x %.1f x %.1f  (X:along wall Y:height Z:depth)",
		data.halfExtents.x * 2.0f, data.halfExtents.y * 2.0f, data.halfExtents.z * 2.0f);

	//---- 回転 ----
	ImGui::SeparatorText("Rotation");
	if (ImGui::SliderFloat("Rot Y (deg)", &data.rotYDeg, -180.0f, 180.0f, "%.1f"))
	{
		isChanged = true;
	}
	auto addRot = [&](float deg)
		{
			data.rotYDeg += deg;
			//-180~180に収める
			while (data.rotYDeg > 180.0f)data.rotYDeg -= 360.0f;
			while (data.rotYDeg < -180.0f)data.rotYDeg += 360.0f;
			isChanged = true;
		};
	if (ImGui::Button("-90")) addRot(-90.0f);
	ImGui::SameLine();
	if (ImGui::Button("-15")) addRot(-15.0f);
	ImGui::SameLine();
	if (ImGui::Button("0"))
	{
		data.rotYDeg = 0.0f;
		isChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("+15")) addRot(15.0f);
	ImGui::SameLine();
	if (ImGui::Button("+90")) addRot(90.0f);

	//変更があったらその場でゾーン(コライダー)に当てはめる
	if (isChanged)
	{
		stage.SetWallZoneData(m_selectedIndex, data);
		m_isDirty = true;
	}
}

void WallZoneEditor::DrawZones(const Stage& stage) const
{
	stage.DrawWallZones(m_selectedIndex);
}

Vector3 WallZoneEditor::Snap(const Vector3& pos) const
{
	if (!m_useGridSnap)return pos;
	auto snap = [&](float v) { return std::round(v / m_gridSize) * m_gridSize; };
	return Vector3(snap(pos.x), snap(pos.y), snap(pos.z));
}

bool WallZoneEditor::RaycastFromCamera(const Stage& stage, const Vector3& camPos, const Vector3& camTarget,
	Vector3& outHitPos, Vector3& outNormal) const
{
	const Vector3 dir = (camTarget - camPos).Normalize();
	const Vector3 end = camPos + dir * kRayLength;

	auto hit = MV1CollCheck_Line(stage.GetStageModelHandle(), -1, camPos.ToDxLibVector(), end.ToDxLibVector());
	if (!hit.HitFlag)return false;

	//壁として使うので、水平方向の法線にする
	Vector3 normal = Vector3::FromDxLibVector(hit.Normal);
	normal.y = 0.0f;
	if (normal.sqMagnitude() <= 0.0001f)return false;//床や天井に当たった

	outHitPos = Vector3::FromDxLibVector(hit.HitPosition);
	outNormal = normal.Normalize();
	return true;
}

bool WallZoneEditor::FindFloorY(const Stage& stage, const Vector3& pos, float& outFloorY) const
{
	const Vector3 start = pos + Vector3(0.0f, kFloorCheckUp, 0.0f);
	const Vector3 end = pos - Vector3(0.0f, kFloorCheckDown, 0.0f);

	auto hit = MV1CollCheck_Line(stage.GetStageModelHandle(), -1, start.ToDxLibVector(), end.ToDxLibVector());
	if (!hit.HitFlag)return false;
	if (hit.Normal.y < kMinFloorNormalY)return false;

	outFloorY = hit.HitPosition.y;
	return true;
}
