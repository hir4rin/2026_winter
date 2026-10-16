#pragma once
#include "PhaseAreaData.h"
#include <string>
#include <vector>

class Stage;
class PhaseManager;
class Collider;

/// <summary>
/// ステージ編集モードで、フェーズ進行のエリア(EnterArea用の箱)をImGuiで設置・編集するクラス
/// 編集内容はその場でPhaseManagerのエリアに当てはめ、SaveでCSVに書き出す
/// </summary>
class PhaseAreaEditor
{
public:
	PhaseAreaEditor() = default;
	~PhaseAreaEditor() = default;

	/// <summary>編集対象のステージ番号を設定する(Load/Saveで使うCSVの番号)</summary>
	void SetStageNumber(int stageNumber) { m_stageNumber = stageNumber; }

	/// <summary>
	/// 編集用のImGuiウィンドウを組み立てる(この中でImGui::Begin~Endを使っている)
	/// </summary>
	/// <param name="phaseMgr">エリアを持っているPhaseManager</param>
	/// <param name="stage">床の高さを調べるのに使う</param>
	/// <param name="player">プレイヤー(配置の基準に使う。nullでもよい)</param>
	/// <param name="camTarget">今のカメラ注視点</param>
	void DrawWindow(PhaseManager& phaseMgr, const Stage& stage, Collider* player, const Vector3& camTarget);

	/// <summary>エリアの線枠を描画する(選択中のエリアは白)</summary>
	void DrawAreas(const PhaseManager& phaseMgr)const;

private:
	//エリア一覧と追加・複製・削除
	void DrawAreaList(PhaseManager& phaseMgr, Collider* player, const Vector3& camTarget);
	//選択中のエリアの詳細編集
	void DrawSelectedArea(PhaseManager& phaseMgr, const Stage& stage, Collider* player, const Vector3& camTarget);

	//グリッドスナップが有効なら座標を丸める
	Vector3 Snap(const Vector3& pos)const;
	//指定した点の真下の床の高さを求める
	bool FindFloorY(const Stage& stage, const Vector3& pos, float& outFloorY)const;

	void SetStatus(const std::string& message) { m_statusMessage = message; }

	//Undo/Redo(WallZoneEditorと同じ仕組み)--------------------------------------
	struct HistoryEntry
	{
		std::vector<PhaseArea> areas;
		int selectedIndex = -1;
	};
	void ApplyHistoryRequest(PhaseManager& phaseMgr);
	void RecordHistory(const PhaseManager& phaseMgr, const HistoryEntry& before);
	void DrawHistoryButtons();
	//----------------------------------------------------------------------------

	//補助関数--------------------------------------------------------------------
	static bool DragVector3(const char* label, Vector3& value, float speed, float min = 0.0f, float max = 0.0f);
	static bool IsSameVector3(const Vector3& a, const Vector3& b);
	static bool IsSameAreas(const std::vector<PhaseArea>& a, const std::vector<PhaseArea>& b);
	static std::string MakeDefaultName(int index);
	//----------------------------------------------------------------------------

private:
	int m_stageNumber = 1;//Load/Saveするステージ番号
	int m_selectedIndex = -1;//選択中のエリア(-1なら未選択)
	bool m_isDirty = false;//保存していない変更があるか

	Vector3 m_newAreaHalfExtents = Vector3(500.0f, 300.0f, 300.0f);//新しく追加するエリアの半分の大きさ

	float m_dragSpeed = 1.0f;//DragFloatの1ピクセルあたりの変化量
	bool m_useGridSnap = false;//座標をグリッドに合わせるか
	float m_gridSize = 50.0f;//グリッドの間隔

	std::string m_statusMessage;//Load/Saveの結果などを表示する

	std::vector<HistoryEntry> m_undoHistory;
	std::vector<HistoryEntry> m_redoHistory;
	bool m_isContinuousEdit = false;//ドラッグ・文字入力の途中か
	bool m_requestUndo = false;
	bool m_requestRedo = false;
};
