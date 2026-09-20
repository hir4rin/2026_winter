#pragma once
#include <memory>
#include "../../Math/Vector3.h"
#include "DxLib.h"
#include <memory>

class Input;
class CameraManager;
struct CameraContext;
class Stage;


class CameraStateBase
{
public:

	//自分で設定するもの
	struct BlendSetting
	{
		enum class Mode { None, Lerp, Slerp, Chase };
		Mode mode = Mode::None;
		float duration = 0.0f;   //何フレームでtargetに到達させるか
		float easingPower = 1.0f;
		Vector3 pivot = Vector3();//Slerp時の基軸(回転の中心)
	};

	//受け渡し用
	struct CameraData
	{
		Vector3 pos;
		Vector3 target;
		float angleH;
		float angleV;

		BlendSetting overrideSetting = {};
	};
	
	
	enum class Type
	{
		None = -1,
		PlayerCamera = 0,
		Movie1Camera = 1,
		UltCamera = 2,
		LockOnCamera = 3,
		TitleCamera = 4,
		Stage1FazeCamera = 5,
		BossAppearCamera = 6,
		Stage2FazeCamera = 7,
		FinishingFirstCamera = 8,
		FinishingSecondCamera = 9,
		ResultCamera = 10,

		//他のカメラもここに追加していく
	};
public:
	CameraStateBase(std::weak_ptr<CameraManager> owner);
	virtual ~CameraStateBase() = 0;
	

	virtual void Enter(CameraData data) = 0;
	virtual void Update() = 0;
	virtual void Exit() = 0;

	virtual void Draw();
	virtual void FixCameraPos();//カメラの位置を調整する

	Vector3 GetPos()const { return m_pos; }
	Vector3 GetTarget()const { return m_target; }

	virtual Type GetCameraType()const = 0;//各Stateが自分の種類を返す(実装を強制する)

	virtual BlendSetting GetBlendSetting()const = 0;//カメラのブレンドセット
	//BlendModeによって、posとかを変更したりする
	virtual BlendSetting GetOverrideBlendSetting()const { return {}; }//上書き用のブレンドデータ//何もないならいらない




	void SetCameraAngle(float angleH, float angleV) { m_angleH = angleH; m_angleV = angleV; }//引き渡しのときにカメラの角度を設定
	float GetCameraAngleH()const { return m_angleH; }
	float GetCameraAngleV()const { return m_angleV; }

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



	virtual void CameraSetting();

	void SetStage(std::weak_ptr<Stage> stage) { m_stage = stage; }
protected:
	void UpdateBlend(const Vector3& rawPos, const Vector3& rawTarget);//毎フレーム呼ぶ
	void ResetBlend(const Vector3& startPos, const Vector3& startTarget);//Enterの最初に呼ぶ
	// 現在blend中かどうか
	bool IsBlending()const { return m_blendElapsed < m_activeBlend.duration; }
protected:
	//ステートの親
	std::weak_ptr<CameraManager> m_owner;

	bool m_isChangeCamera = false;//カメラを切り変えるかどうかのフラグ

	Vector3 m_target;//注視点//ターゲット座標
	Vector3 m_startTarget;////注視点//初期ターゲット座標
	Vector3 m_goalTarget;//注視点//目的ターゲット座標
	Vector3 m_pos;//カメラの位置//現座標
	Vector3 m_startPos;//カメラの位置//初期座標
	Vector3 m_goalPos;//カメラの位置//目的座標

	float m_angleH = 0.0f;// 水平方向の回転角度
	float m_angleV = 0.0f;// 垂直方向の回転角度
	CameraData m_cameraData;

	float m_blendElapsed = 0.0f;//経過フレーム数
	BlendSetting m_activeBlend;//Enter時に確定させた、この遷移用のブレンド設定

	//カメラ揺れ用
	float m_shakePower = 0.0f;
	float m_shakeTimer = 0.0f;
	float m_shakeTimerMax = 0.0f;//減衰用のコピー
	bool m_isShaking = false;//今カメラが揺れているかどうか

	//blend設定
	//BlendSetting m_blendSetting;

	//ライト
	int m_lightHandle = -1;//ライトのハンドル

	//カメラマネージャーの弱参照

	//ステージの弱参照
	std::weak_ptr<Stage> m_stage;//ステージのハンドルを取得するために使う
};

