#pragma once
#include "../Math/Vector3.h"
#include "DxLib.h"
#include <memory>
class Input;
class CameraManager;
struct CameraContext;
class Stage;

struct CameraData
{
	Vector3 pos;
	Vector3 target;
	float angleH;
	float angleV;
};

//保管するカメラかどうかのセットアップ
struct CameraSetUp
{
	bool DoLerp = false;
	bool DoSlerp = false;
};

class Camera
{
protected:
public:
	enum class Type
	{
		None = -1,
		PlayerCamera = 0,
		Movie1Camera = 1,
		UltCamera = 2,
		LockOnCamera = 3,
		TitleCamera = 4,
		ResultCamera = 5,

		//他のカメラもここに追加していく
	};
public:
	Camera();
	virtual ~Camera();


	virtual void Init();
	/// <summary>
	/// 
	/// </summary>
	/// <param name="pos">プレイヤーの座標</param>
	virtual void Update(Vector3 pos,Vector3 pos2 = Vector3());
	virtual void Draw();
	virtual void FixCameraPos();//カメラの位置を調整する

	Vector3 GetCameraPos()const { return m_pos; }
	Vector3 GetCameraTarget()const { return m_target; }

	int GetPriority()const { return m_priority; }
	void SetPriority(int priority) { m_priority = priority; }

	Type GetCameraType()const { return m_type; }


	void SetCameraAngle(float angleH, float angleV) { m_angleH = angleH; m_angleV = angleV; }//引き渡しのときにカメラの角度を設定
	float GetCameraAngleH()const { return m_angleH; }
	float GetCameraAngleV()const { return m_angleV; }

	CameraSetUp& GetCameraSetUp() { return m_cameraSetUp; }

	//カメラデータを渡す//この時、場合によっては初期化
	virtual void SetCameraData(const CameraData& data) { m_cameraData = data; }
	const CameraData& GetCameraData()const { return m_cameraData; }

	/// <summary>
	/// カメラを揺らす
	/// </summary>
	/// <param name="camera">カメラのポインタ</param>
	/// <param name="power">揺れの強さ</param>
	/// <param name="time">揺らす時間</param>
	void StartCameraShake(float power, float time);
	Vector3 CameraShakeUpdate();
	void UpdateLight();//ライトの位置を更新
	void SetCameraManager(std::weak_ptr<CameraManager> cameraManager) { m_cameraManager = cameraManager; }

	virtual void CameraSetting();

	void SetCameraContext(std::shared_ptr<CameraContext> cameraContext) { m_cameraContext = cameraContext; }
	
	void SetStage(std::weak_ptr<Stage> stage) { m_stage = stage; }
protected:
	void InputRightStick();//右スティックの入力を処理する
protected:
	int m_priority = 0;//カメラの優先度//複数のカメラがあるときに、どのカメラを優先するかを決めるためのもの//数値が大きいほど優先される
	bool m_isChangeCamera = false;//カメラを切り変えるかどうかのフラグ

	Vector3 m_target;//注視点
	Vector3 m_pos;//カメラの位置
	Type m_type = Type::None;//カメラの種類
	
	float m_angleH = 0.0f;// 水平方向の回転角度
	float m_angleV = 0.0f;// 垂直方向の回転角度
	CameraData m_cameraData;
	CameraSetUp m_cameraSetUp;

	//カメラ揺れ用
	float m_shakePower = 0.0f;
	float m_shakeTimer = 0.0f;
	float m_shakeTimerMax = 0.0f;//減衰用のコピー
	bool m_isShaking = false;//今カメラが揺れているかどうか
	//ライト
	int m_lightHandle = -1;//ライトのハンドル

	//カメラマネージャーの弱参照
	std::weak_ptr<CameraManager> m_cameraManager;
	std::shared_ptr<CameraContext> m_cameraContext;

	//ステージの弱参照
	std::weak_ptr<Stage> m_stage;//ステージのハンドルを取得するために使う
};

