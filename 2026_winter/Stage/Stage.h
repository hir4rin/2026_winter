#pragma once
#include "Collision/Collider.h"
#include "WallZone/WallZone.h"
#include "Main/System.h"
#include "StageInfo.h"
#include "StageLights.h"
#include "TrainLighting.h"
#include "TrainSparks.h"
#include "TrainOutside.h"
#include "Gate/Gate.h"
#include <memory>
#include <vector>

class Stage : public Collider
{
public:
	Stage();
	~Stage();
	void Init();
	void TitleInit();
	/// <summary>ゲーム用の初期化。見た目・当たり判定のモデルやトンネルの有無はinfoから決める</summary>
	void GameInit(const StageInfo& info);

	void Update();
	void Draw()const;
	void OnCollision(Collider& other)override;
	int GetStageModelHandle()const { return m_stageModelHandle;}
	int GetStageViewHandle()const { return m_stageViewHandle; }
	/// <summary>ステージの明かり(プレイヤーに近いものを点ける)</summary>
	StageLights& GetLights() { return m_lights; }
	/// <summary>プレイヤーに近い明かりをシェーダーに渡す(CSV の明かり・列車ステージのライティング)。列車ステージの火花もここで出す</summary>
	void UpdateLights(const Vector3& playerPos);
	/// <summary>明かりの調整の ImGui ウィンドウ</summary>
	void DrawLightDebugWindow();

	/// <summary>扉(ゲート)を今のフェーズに合わせて開け閉めする</summary>
	void UpdateGates(int currentPhase);
	/// <summary>扉の当たり判定の線枠(デバッグ用。閉まっているものは赤)</summary>
	void DrawGateCols()const;
	/// <summary>
	/// カメラ用の線分の当たり判定。ステージの当たり判定のモデルと扉(カメラ用の箱)を調べ、始点に一番近い当たりを返す
	/// </summary>
	MV1_COLL_RESULT_POLY CollCheckCameraLine(const Vector3& start, const Vector3& end)const;

	//エンディング(列車がトンネルを抜ける)-----------------------------------------
	struct FogSetting
	{
		int r = 0;
		int g = 0;
		int b = 0;
		float start = 0.0f;
		float end = 0.0f;
	};
	bool HasTunnel()const { return m_tunnelHandle != -1; }
	float GetTrainForwardZ()const;//列車の進む向き(+1か-1)
	FogSetting GetTrainFog()const;//いつもの列車のフォグ
	/// <summary>baseZ から進む向きに minDistance より先の継ぎ目でトンネルを終わらせる(その先は描かない)</summary>
	/// <returns>出口までの距離</returns>
	float StartTunnelExit(float baseZ, float minDistance);
	void SetTunnelSpeedScale(float scale) { m_tunnelSpeedScale = scale; }//流す速さの倍率
	float GetTunnelSpeed()const;//1フレームに流す距離(倍率込み・正の値)
	/// <summary>z から出口までの距離(進む向きが+。抜けたらマイナス)</summary>
	float GetTunnelExitDistance(float z)const;
	void SetFogOverride(const FogSetting& fog) { m_fogOverride = fog; m_hasFogOverride = true; }
	void SetTunnelExitLight(int r, int g, int b) { m_exitLightR = r; m_exitLightG = g; m_exitLightB = b; }//出口の穴の色
	/// <summary>抜けた後の夜の空・遠くの街を出す割合(0〜1)。空気感も外の夜に寄せる</summary>
	void SetOutsideRate(float rate);
	//------------------------------------------------------------------------------

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

	/// <summary>これより下に落ちたら落下死(当たり判定のモデルの一番下から少し下)</summary>
	float GetKillY()const { return m_killY; }

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

	Vector3 m_pos_graphic;

	int m_stageModelHandle;
	int m_stageViewHandle;

	//列車ステージのトンネル(見た目だけ。列車の進む向きと逆に流し続ける)
	int m_tunnelHandle = -1;
	float m_tunnelScroll = 0.0f;//流した距離(トンネル1本の長さで折り返す)
	float m_tunnelSpeedScale = 1.0f;
	float m_tunnelFrontOffset = 0.0f;//置いた位置から進む向きの端まで
	float m_tunnelHalfWidth = 0.0f;//トンネルの外側の大きさ(出口の穴に使う)
	float m_tunnelMinY = 0.0f;
	float m_tunnelMaxY = 0.0f;
	int m_exitLightR = 0;//出口の穴の色
	int m_exitLightG = 0;
	int m_exitLightB = 0;
	void DrawTunnelExitLight()const;
	void DrawArch(float centerX, float bottom, float spring, float halfWidth, float archHeight, unsigned int color)const;
	bool m_hasTunnelExit = false;//トンネルの終わりがあるか(エンディング)
	float m_tunnelExitZ = 0.0f;//出口のz(トンネルと一緒に流れる)
	bool m_hasFogOverride = false;
	FogSetting m_fogOverride;
	bool m_useNightFog = false;//遠くを夜の色にする(StageInfo::useNightFog)
	float m_killY = -1.0e9f;//落下死の高さ(GameInitで決める)
	StageLights m_lights;
	std::unique_ptr<TrainLighting> m_trainLighting;//列車ステージのライティング(トンネルがあるときだけ)
	std::unique_ptr<TrainSparks> m_trainSparks;//列車ステージの車輪の火花(トンネルがあるときだけ)
	std::unique_ptr<TrainOutside> m_outside;//トンネルを抜けた後の景色(トンネルがあるときだけ)
	std::vector<Vector3> m_trackLampBuffer;
	void DrawTunnel()const;

	//画面に映らないメッシュは描かない(MV1DrawModel は画面の外のメッシュも全部描くため)
	struct MeshBox
	{
		VECTOR min;
		VECTOR max;
	};
	std::vector<MeshBox> m_meshBoxes;//メッシュごとの箱(ワールド座標。添え字はメッシュの番号)
	/// <summary>見た目のモデルを置いた後に1回呼ぶ(ステージは動かないので箱は作り直さない)</summary>
	void BuildMeshBoxes();
	void CullMeshes()const;


	/// <summary>読み込み済みの壁ゾーンを全て破棄する(CollisionManagerからも解除する)</summary>
	void ClearWallZones();
	//壁キック/壁走りできるゾーン(ステージ編集で配置したもの)
	std::vector<std::shared_ptr<WallZone>> m_wallZones;

	//扉(data/Stage/Gates/Stage%03d.csv。番号は敵配置と同じ)
	std::vector<std::shared_ptr<Gate>> m_gates;
	void LoadGates(int stageNumber);
	void ClearGates();
};

