#pragma once
#include "../Collider/Collider.h"
#include "WallZoneData.h"

/// <summary>
/// 壁キック/壁走りを許可するゾーン(押し戻しをしないトリガーのBOX)
/// タグは{StaticObject, WallKickZone or WallRunZone}
/// プレイヤーがこの中にいるときだけ、ステージモデルへのレイ判定で壁キック/壁走りできる
/// CollisionCheckerの中でやろうとすると、いろいろ不都合が発生する(OnCollisonの呼び出し系)のでこっちでやる
/// </summary>
class WallZone : public Collider
{
public:
	WallZone();
	virtual ~WallZone();

	/// <summary>データから初期化する(この中でCollisionManagerへの登録も行われる)</summary>
	void Init(const WallZoneData& data);

	/// <summary>編集画面で変更したデータを当てはめる(タグ・形状・位置・有効をまとめて更新する)</summary>
	void SetData(const WallZoneData& data);
	const WallZoneData& GetData()const { return m_data; }

	/// <summary>
	/// 指定したコライダー(カプセル・球)がこのゾーンと重なっているか
	/// 有効でないゾーンは常にfalse
	/// </summary>
	bool IsOverlap(const Collider& other)const;

	void OnCollision(Collider& other) override;
	void ApplyPos() override;

	/// <summary>指定した色で線枠を描画する(編集画面で選択中のゾーンを目立たせるのに使う)</summary>
	void DrawWithColor(unsigned int color)const;

private:
	/// <summary>カプセル(線分＋半径)がゾーンと重なっているか</summary>
	bool IsOverlapCapsule(const Vector3& start, const Vector3& end, float radius)const;

	WallZoneData m_data;
};
