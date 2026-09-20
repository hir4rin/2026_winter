#pragma once
#include <vector>
#include <functional>
#include <memory>
#include <memory>
#include "FixNextPosition.h"//unique_ptrはデストラクタでdeleteするためヘッダに書く必要あり
#include "CollisionChecker.h"

class Collider;
class Stage;

//衝突情報
struct CollisionInfo
{
	std::shared_ptr<Collider> colliderA;//衝突したコライダーA
	std::shared_ptr<Collider> colliderB;//衝突したコライダーB
};
//衝突時のコールバック関数の型//????//わからないから使わない
using CollisionCallback = std::function<void(const CollisionInfo&)>;

class CollisionManager
{
public:
	//シングルトンインスタンスを取得
	static CollisionManager& GetInstance()
	{
		static CollisionManager instance;
		return instance;
	}
	//コライダーの登録・解除
	void RegisterCollider(std::shared_ptr<Collider> collider);//shared_ptrを参照で渡すと、参照カウントが増えない
	void ReleaseCollider(std::shared_ptr<Collider> collider);
	//初期化
	void Init();
	void SetStage(std::weak_ptr<Stage> stage);

	//すべてのコライダーをクリア
	void Terminate();
	//当たり判定の一括チェック
	void Update();
	//デバッグ描画
	void DebugDraw()const;
	//指定したidのコライダーを取得する
	std::shared_ptr<Collider> GetColliderById(int id)const;
private:
	//隠し関数
	bool ContainsCollider(const std::vector<std::weak_ptr<Collider>>& list, const std::shared_ptr<Collider>& target);

private:
	//コンストラクタとデストラクタをプライベートにして、シングルトンパターンを実装
	CollisionManager() = default;
	virtual ~CollisionManager() = default;
	//コピーコンストラクタと代入演算子を削除して、シングルトンのインスタンスが複製されないようにする
	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;

	//すべてのコライダー
	std::vector<std::shared_ptr<Collider>> m_colliders;
	//std::vector<std::pair<Collider::Tags, CollisionCallback>> m_callbacks;//衝突時のコールバック関数のリスト//????//わからないから使わない

	//後々、コライダーを停止、再開する機能を追加するかも

	void ApplyAdjustments();
	/// <summary>
	/// コライダーにtimeScaleを考慮した速度を加算する
	/// </summary>
	void AddVelocity();
private:
	//当たり判定のチェックを行うクラス
	std::unique_ptr<CollisionChecker> m_collisionChecker;
	std::unique_ptr<FixNextPosition> m_fixNextPositioner;
private:
	std::weak_ptr<Stage> m_stage;//ステージへの弱参照//ステージのポリゴンの当たり判定をするために必要
};

