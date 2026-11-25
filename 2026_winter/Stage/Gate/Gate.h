#pragma once
#include "Collision/Collider.h"
#include "GateData.h"
#include <vector>

/// <summary>
/// フェーズで開け閉めする扉(ゲート)
/// 閉まっている間だけ当たり判定の箱(PolygonShape)で通れなくする
/// </summary>
class Gate : public Collider
{
public:
	Gate() = default;
	virtual ~Gate();

	/// <summary>モデルを読み込み、当たり判定を登録する。offsetはステージをずらした分</summary>
	void Init(const GateData& data, const Vector3& offset);
	/// <summary>今のフェーズに合わせて開け閉めする</summary>
	void Update(int currentPhase);
	/// <summary>動かさずに、そのフェーズの開け閉めの状態にする(リスポーン用)</summary>
	void Snap(int currentPhase);
	void Draw()const;

	void OnCollision(Collider& other) override {}
	const GateData& GetData()const { return m_data; }
	float GetOpenRate()const { return m_openRate; }
	bool IsOpenTarget()const { return m_isOpenTarget; }
	/// <summary>当たり判定の箱の線枠(デバッグ用)</summary>
	void DrawCol(unsigned int color)const;
	/// <summary>カメラ用の箱の線枠(デバッグ用)</summary>
	void DrawCameraBox(unsigned int color)const;
	/// <summary>カメラ用の箱と線分の当たり判定(当たったら hit に入れて true)</summary>
	bool CollCheckCameraLine(const Vector3& start, const Vector3& end, MV1_COLL_RESULT_POLY& hit)const;

private:
	void ApplyPos() override {}
	bool IsOpenPhase(int phase)const;
	void ApplyModelMatrix();
	/// <summary>ランプの色(閉=赤の明滅、開=緑)</summary>
	void ApplyLampColor();
	/// <summary>
	/// カメラ用の箱を置き直す。プレイヤー用の箱(高さ15m)と見た目の範囲が重なった部分
	/// (見た目だけの行は見た目の範囲、箱だけの行は箱)。シャッターが巻き上がると一緒に縮む
	/// </summary>
	void UpdateCameraBox();
	/// <summary>扉の向き基準の箱(中心・半分の大きさ)を線枠で描く</summary>
	void DrawBoxLine(const Vector3& localCenter, const Vector3& half, unsigned int color)const;

	GateData m_data;
	Vector3 m_pos;//ステージのずれを足した位置
	int m_modelHandle = -1;
	int m_colHandle = -1;//当たり判定の箱
	float m_openRate = 0.0f;//0:閉 1:開
	bool m_isOpenTarget = false;//開く方へ動いているか
	float m_waitFrame = 0.0f;//動き出すまで
	std::vector<int> m_lampMaterials;//ランプのマテリアル(自発光が赤いもの)
	float m_lampFrame = 0.0f;//明滅用

	//今の大きさ・移動(扉の向き基準。ApplyModelMatrix で決める)
	Vector3 m_curScale = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 m_curMove;
	//見た目のモデルの範囲(モデルの座標)
	Vector3 m_modelMin, m_modelMax;
	//カメラ用の箱
	int m_camHandle = -1;
	bool m_isCamBoxActive = false;
	bool m_isCamBoxPlaced = false;
	Vector3 m_camMin, m_camMax;//扉の向き基準
	Vector3 m_camCenter;//ワールド
	float m_camRadius = 0.0f;
};
