#pragma once
#include "../Collider/Collider.h"

/// <summary>
/// ステージ制作モードで配置する、当たり判定のみのBOX
/// 見た目のモデルは持たず、線枠(DebugDraw)のみで表示する
/// </summary>
class StageObject : public Collider
{
public:
	StageObject();
	virtual ~StageObject();

	/// <summary>中心座標とBoxの半分の大きさで初期化する(この中でCollisionManagerへの登録も行われる)</summary>
	void Init(const Vector3& pos, const Vector3& halfExtents);

	void OnCollision(Collider& other) override;
	void ApplyPos() override;

	void Draw() const;

	/// <summary>ステージ制作モードで、配置済みのBOXの位置・大きさを調整するために使う</summary>
	void SetTransform(const Vector3& pos, const Vector3& halfExtents);
};
