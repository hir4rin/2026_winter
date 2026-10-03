#pragma once
#include "WallZoneData.h"
#include <string>
#include <vector>

class Stage;
class Collider;

/// <summary>
/// ステージ編集モードで、壁キック/壁走りゾーンをImGuiで設置・編集するクラス
/// 編集内容はその場でStageのゾーン(コライダー)に当てはめ、SaveでCSVに書き出す
/// </summary>
class WallZoneEditor
{
public:
	WallZoneEditor() = default;
	~WallZoneEditor() = default;

	/// <summary>編集対象のステージ番号を設定する(Load/Saveで使うCSVの番号)</summary>
	void SetStageNumber(int stageNumber) { m_stageNumber = stageNumber; }

	/// <summary>
	/// 編集用のImGuiウィンドウを組み立てる(この中でImGui::Begin~Endを使っている)
	/// </summary>
	/// <param name="stage">編集するステージ</param>
	/// <param name="player">プレイヤー(配置の基準と、ゾーンの中にいるかの表示に使う。nullでもよい)</param>
	/// <param name="camPos">今のカメラ座標</param>
	/// <param name="camTarget">今のカメラ注視点</param>
	void DrawWindow(Stage& stage, const Collider* player, const Vector3& camPos, const Vector3& camTarget);

	/// <summary>ゾーンの線枠を描画する(選択中のゾーンは白)</summary>
	void DrawZones(const Stage& stage)const;

private:
	//ゾーン一覧と追加・複製・削除
	void DrawZoneList(Stage& stage, const Collider* player, const Vector3& camPos, const Vector3& camTarget);
	//選択中のゾーンの詳細編集
	void DrawSelectedZone(Stage& stage, const Collider* player, const Vector3& camPos, const Vector3& camTarget);

	//グリッドスナップが有効なら座標を丸める
	Vector3 Snap(const Vector3& pos)const;

	//カメラの向きにレイを飛ばして、ステージモデルに当たった点と法線を求める
	bool RaycastFromCamera(const Stage& stage, const Vector3& camPos, const Vector3& camTarget,
		Vector3& outHitPos, Vector3& outNormal)const;
	//指定した点の真下の床の高さを求める
	bool FindFloorY(const Stage& stage, const Vector3& pos, float& outFloorY)const;

	void SetStatus(const std::string& message) { m_statusMessage = message; }

	//Undo/Redo-------------------------------------------------------------------
	//履歴1つ分(その時点の全ゾーンと選択中のindex)
	struct HistoryEntry
	{
		std::vector<WallZoneData> zones;
		int selectedIndex = -1;
	};
	//フレームの最初に、前のフレームで頼まれたUndo/Redoを実行する
	//(ウィンドウを組み立てている途中で戻すと、その変更自体が「編集」として履歴に積まれてしまうため)
	void ApplyHistoryRequest(Stage& stage);
	//フレームの最後に、フレームの最初と比べてゾーンが変わっていたら、変わる前を履歴に積む
	void RecordHistory(const Stage& stage, const HistoryEntry& before);
	//Undo/Redoのボタンとショートカット(Ctrl+Z / Ctrl+Y)
	void DrawHistoryButtons();
	//----------------------------------------------------------------------------

private:
	int m_stageNumber = 1;//Load/Saveするステージ番号
	int m_selectedIndex = -1;//選択中のゾーン(-1なら未選択)
	bool m_isDirty = false;//保存していない変更があるか

	Collider::ColRole m_newZoneType = Collider::ColRole::WallKickZone;//新しく追加するゾーンの種類
	Vector3 m_newZoneHalfExtents = Vector3(200.0f, 200.0f, 100.0f);//新しく追加するゾーンの半分の大きさ(X:壁に沿う幅 Y:高さ Z:壁からの厚み)

	float m_dragSpeed = 1.0f;//DragFloatの1ピクセルあたりの変化量
	bool m_useGridSnap = false;//座標をグリッドに合わせるか
	float m_gridSize = 50.0f;//グリッドの間隔

	std::string m_statusMessage;//Load/Saveの結果などを表示する

	std::vector<HistoryEntry> m_undoHistory;//1つ前に戻すための履歴(後ろほど新しい)
	std::vector<HistoryEntry> m_redoHistory;//Undoで戻したものをやり直すための履歴(後ろほど新しい)
	bool m_isContinuousEdit = false;//ドラッグ・文字入力の途中か(触っている間の変更は1回分の履歴にまとめる)
	bool m_requestUndo = false;//次のフレームの最初にUndoする
	bool m_requestRedo = false;//次のフレームの最初にRedoする
};
