#pragma once
#include "../Collider/Collider.h"
#include "StageObject.h"
#include "WallZone.h"
#include "../System.h"
#include <memory>
#include <vector>

class Stage : public Collider
{
public:
	Stage();
	~Stage();
	void Init();
	void TitleInit();
	/// <summary>ゲーム用の初期化。modelにはSystemで読み込んだステージモデルの種類を渡す</summary>
	void GameInit(AsyncData model);

	void Update();
	void Draw()const;
	void OnCollision(Collider& other)override;
	int GetStageModelHandle()const { return m_stageModelHandle;}

	/// <summary>
	/// ステージ制作モード(StageEditScene)で作ったCSVから、ステージ番号を指定してBOX配置を読み込む
	/// ステージを増やす場合は、この関数を呼ぶstageNumberを変えるだけでよい
	/// </summary>
	void LoadStageObjects(int stageNumber);

	//壁ゾーン(WallKickZone/WallRunZone)----------------------------------------------
	/// <summary>指定したステージ番号の壁ゾーンCSVを読み込み、今あるゾーンと入れ替える</summary>
	void LoadWallZones(int stageNumber);
	/// <summary>今のゾーンを指定したステージ番号のCSVに書き出す</summary>
	/// <returns>書き込みに成功したらtrue</returns>
	bool SaveWallZones(int stageNumber)const;

	/// <summary>
	/// 指定した役割(WallKickZone/WallRunZone)の有効なゾーンのどれかに、コライダーが重なっているか
	/// </summary>
	bool IsInWallZone(Collider::ColRole role, const Collider& col)const;

	//編集画面用
	int GetWallZoneNum()const { return static_cast<int>(m_wallZones.size()); }
	const WallZoneData& GetWallZoneData(int index)const;
	void SetWallZoneData(int index, const WallZoneData& data);
	/// <returns>追加したゾーンのindex</returns>
	int AddWallZone(const WallZoneData& data);
	void RemoveWallZone(int index);
	/// <summary>全ゾーンのデータをまとめて取得する(編集画面のUndo用)</summary>
	std::vector<WallZoneData> GetAllWallZoneData()const;
	/// <summary>全ゾーンをまとめて入れ替える(編集画面のUndo用)</summary>
	void SetAllWallZoneData(const std::vector<WallZoneData>& zones);
	/// <summary>指定したindexのゾーンにコライダーが重なっているか(編集画面の表示用)</summary>
	bool IsWallZoneOverlap(int index, const Collider& col)const;
	/// <summary>壁ゾーンの線枠を描画する(selectedIndexのゾーンは白で目立たせる)</summary>
	void DrawWallZones(int selectedIndex)const;
	//------------------------------------------------------------------------------
private:
	void ApplyPos() override {};//座標の更新はしない

	/// <summary>読み込み済みのBOXを全て破棄する(CollisionManagerからも解除する)</summary>
	void ClearStageObjects();

	Vector3 m_pos_graphic;

	int m_stageModelHandle;
	int m_stageViewHandle;


	//CSVから読み込んだ、当たり判定のみのBOX配置(ステージ制作モードで作成したもの)
	std::vector<std::shared_ptr<StageObject>> m_stageObjects;

	/// <summary>読み込み済みの壁ゾーンを全て破棄する(CollisionManagerからも解除する)</summary>
	void ClearWallZones();
	//壁キック/壁走りできるゾーン(ステージ編集で配置したもの)
	std::vector<std::shared_ptr<WallZone>> m_wallZones;
};

