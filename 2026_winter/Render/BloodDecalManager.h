#pragma once
#include "DxLib.h"
#include "Math/Vector3.h"
#include <array>

//床に残る血の量
enum class BloodAmount : int
{
	Small,//部位破壊
	Large,//暗殺・確殺
};

//床に残る血の跡をまとめて管理する(床に貼った板ポリを1回で描く)//BattleManagerが持つ
class BloodDecalManager
{
public:
	BloodDecalManager() = default;
	~BloodDecalManager() = default;
	BloodDecalManager(const BloodDecalManager&) = delete;
	BloodDecalManager& operator=(const BloodDecalManager&) = delete;

	/// <summary>シーンの始めに呼ぶ</summary>
	/// <param name="stageColHandle">ステージの当たり判定のモデル</param>
	/// <param name="stageViewHandle">ステージの見た目のモデル</param>
	void Init(int stageColHandle, int stageViewHandle);
	//シーンの終わりに呼ぶ
	void Terminate();

	/// <summary>足元に血だまり、dirの向きに飛沫を出す</summary>
	/// <param name="dir">血が飛ぶ向き(水平)</param>
	void Spawn(const Vector3& footPos, const Vector3& dir, BloodAmount amount);

	//時計を進める
	void Update();
	//ステージの後、キャラの前に描く
	void Draw();

private:
	static constexpr int kMaxDecalNum = 256;//血の最大数
	static constexpr int kVertexPerDecal = 4;//1枚の頂点の数
	static constexpr int kIndexPerDecal = 6;//1枚の番号の数

	//血1枚分のメモ
	struct Decal
	{
		Vector3 center;//床の位置
		Vector3 normal;//床の向き
		Vector3 right;//床に沿った横
		Vector3 forward;//床に沿った前
		float halfSize = 0.0f;//大きさの半分
		float stretch = 1.0f;//飛ぶ向きの長さの倍率
		float age = 0.0f;//経過フレーム
		float delay = 0.0f;//出るまでのフレーム
		float growFrame = 1.0f;//広がるフレーム
		int cellX = 0;//画像のマス
		int cellY = 0;
		bool isActive = false;//使っているか
	};

	//飛沫1種類分の出し方
	struct SplatParam
	{
		int num = 0;//数
		float sizeMin = 0.0f;//大きさの最小
		float sizeMax = 0.0f;//大きさの最大
		float distMin = 0.0f;//距離の最小
		float distMax = 0.0f;//距離の最大
		float spreadAngle = 0.0f;//広がる角度
		float stretchMax = 1.0f;//長さの最大の倍率
	};

	//線を飛ばして見た目の床を探す
	bool FindFloor(const Vector3& start, const Vector3& end, Vector3& outPos, Vector3& outNormal)const;
	//四隅の下に見た目の床があるか
	bool CanFit(const Decal& decal, float halfSize)const;
	//飛沫をまとめて出す
	void AddSplats(const Vector3& ground, float baseAngle, float fallDelay, const SplatParam& param);
	//床に合わせて置く//はみ出すなら小さくする
	void AddDecal(Decal decal, const Vector3& dir);
	//minからmaxの乱数
	float RandRange(float min, float max)const;

	int m_graphHandle = -1;//血の画像
	int m_stageColHandle = -1;//当たり判定のモデル
	int m_stageViewHandle = -1;//見た目のモデル
	int m_nextIndex = 0;//次に上書きする番号
	std::array<Decal, kMaxDecalNum> m_decals;//血のメモ帳
	std::array<VERTEX3D, kMaxDecalNum * kVertexPerDecal> m_vertices;//描く頂点
	std::array<unsigned short, kMaxDecalNum * kIndexPerDecal> m_indices;//三角形の番号
};
