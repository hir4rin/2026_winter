#pragma once
#include "../Collider/Collider.h"
#include "StageObject.h"
#include <memory>
#include <vector>

class Stage : public Collider
{
public:
	Stage();
	~Stage();
	void Init();
	void TitleInit();
	void GameInit();

	void Update();
	void Draw()const;
	void OnCollision(Collider& other)override;
	int GetStageModelHandle()const { return m_stageModelHandle;}

	void SetHitDim(MV1_COLL_RESULT_POLY_DIM& dim) { m_hitDim = dim; }
	MV1_COLL_RESULT_POLY_DIM& GetHitDim() { return m_hitDim; }

	/// <summary>
	/// ステージ制作モード(StageEditScene)で作ったCSVから、ステージ番号を指定してBOX配置を読み込む
	/// ステージを増やす場合は、この関数を呼ぶstageNumberを変えるだけでよい
	/// </summary>
	void LoadStageObjects(int stageNumber);
private:
	void ApplyPos() override {};//座標の更新はしない

	/// <summary>読み込み済みのBOXを全て破棄する(CollisionManagerからも解除する)</summary>
	void ClearStageObjects();

	Vector3 m_pos_graphic;

	int m_stageModelHandle;
	int m_stageViewHandle;
	//当たったポリゴンの情報
	MV1_COLL_RESULT_POLY_DIM m_hitDim;

	//CSVから読み込んだ、当たり判定のみのBOX配置(ステージ制作モードで作成したもの)
	std::vector<std::shared_ptr<StageObject>> m_stageObjects;
};

