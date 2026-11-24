#pragma once
#include "CheckpointData.h"
#include <string>
#include <vector>

class Stage;
class Collider;
class CheckpointManager;

/// <summary>
/// ステージ編集モードで、チェックポイントをImGuiで設置・編集するクラス
/// 編集内容はその場でCheckpointManagerに当てはめ、SaveでCSVに書き出す
/// </summary>
class CheckpointEditor
{
public:
	CheckpointEditor() = default;
	~CheckpointEditor() = default;

	/// <summary>編集対象のステージ番号を設定する(Load/Saveで使うCSVの番号)</summary>
	void SetStageNumber(int stageNumber) { m_stageNumber = stageNumber; }

	/// <summary>
	/// 編集用のImGuiウィンドウを組み立てる(この中でImGui::Begin~Endを使っている)
	/// </summary>
	/// <param name="checkpointMgr">チェックポイントを持っているCheckpointManager</param>
	/// <param name="stage">床の高さを調べるのに使う</param>
	/// <param name="player">プレイヤー(配置の基準・ワープに使う。nullでもよい)</param>
	/// <param name="camPos">今のカメラ位置(向きを決めるのに使う)</param>
	/// <param name="camTarget">今のカメラ注視点</param>
	void DrawWindow(CheckpointManager& checkpointMgr, const Stage& stage, Collider* player, const Vector3& camPos, const Vector3& camTarget);

	/// <summary>チェックポイントの範囲を描画する(選択中は白、プレイヤーが中にいると赤、次は青、踏んだ物は灰)</summary>
	void DrawCheckpoints(const CheckpointManager& checkpointMgr)const;

private:
	//一覧と追加・複製・削除
	void DrawList(const CheckpointManager& checkpointMgr, const Stage& stage, Collider* player, const Vector3& camTarget);
	//選択中のチェックポイントの詳細編集
	void DrawSelected(const Stage& stage, Collider* player, const Vector3& camPos, const Vector3& camTarget);

	//グリッドスナップが有効なら座標を丸める
	Vector3 Snap(const Vector3& pos)const;
	//指定した点の真下の床の高さを求める
	bool FindFloorY(const Stage& stage, const Vector3& pos, float& outFloorY)const;

	void SetStatus(const std::string& message) { m_statusMessage = message; }

	//Undo/Redo(PhaseAreaEditorと同じ仕組み)-------------------------------------
	struct HistoryEntry
	{
		std::vector<CheckpointData> checkpoints;
		int selectedIndex = -1;
	};
	void ApplyHistoryRequest();
	void RecordHistory(const HistoryEntry& before);
	void DrawHistoryButtons();
	//----------------------------------------------------------------------------

	//補助関数--------------------------------------------------------------------
	static bool DragVector3(const char* label, Vector3& value, float speed);
	static bool IsSameVector3(const Vector3& a, const Vector3& b);
	static bool IsSameCheckpoints(const std::vector<CheckpointData>& a, const std::vector<CheckpointData>& b);
	static float WrapDeg(float deg);//-180~180に収める
	static std::string MakeDefaultName(int index);
	//----------------------------------------------------------------------------

private:
	std::vector<CheckpointData> m_checkpoints;//このフレームで編集するデータ(CheckpointManagerから写す)
	int m_stageNumber = 1;//Load/Saveするステージ番号
	int m_selectedIndex = -1;//選択中(-1なら未選択)
	bool m_isDirty = false;//保存していない変更があるか

	float m_dragSpeed = 1.0f;//DragFloatの1ピクセルあたりの変化量
	bool m_useGridSnap = false;//座標をグリッドに合わせるか
	float m_gridSize = 50.0f;//グリッドの間隔
	bool m_dropOnAdd = true;//追加したら床に下ろすか

	std::string m_statusMessage;//Load/Saveの結果などを表示する

	std::vector<HistoryEntry> m_undoHistory;
	std::vector<HistoryEntry> m_redoHistory;
	bool m_isContinuousEdit = false;//ドラッグ・文字入力の途中か
	bool m_requestUndo = false;
	bool m_requestRedo = false;
};
