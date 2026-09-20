#pragma once
#include <memory>
#include <list>
#include <vector>
#include "../Math/Vector3.h"
#include "Camera.h"

class Input;
class Player;
class EnemyBase;
class MainCamera;
class Stage;
class LockOnManager;
class CameraStateBase;

struct CameraContext
{
	std::weak_ptr<Player> m_player;
	//std::weak_ptr<EnemyBase> m_targetEnemyNoLockOn;
	bool m_isUltimate = false;
};

class CameraManager : public std::enable_shared_from_this<CameraManager>
{
public:
	enum class CameraStateName
	{
		PlayerCaemra,
		LockOnCamera,
		UltCamera,
		TitleCamera,
		FinishingFirstCamera,
		FinishingSecondCamera,
		ResultCamera

	};

public:
	CameraManager();
	virtual ~CameraManager();
	//Playerのweak_ptrを渡す
	void Init(std::weak_ptr<Player> player, std::weak_ptr<Stage> stage = {});

	/// <summary>
	/// 
	/// </summary>
	/// <param name="pos">targetの座標</param>
	/// <param name="pos2">補助的な座標</param>
	void Update(Vector3 pos, Vector3 pos2 = Vector3());
	void Draw();

	/// <summary>
	/// レンダーターゲットごとに変わってしまうので、カメラの設定を反映させる
	/// </summary>
	void ApplyCameraSettings();

	//RefWeakptr用
	void SetWeakRef(std::weak_ptr<Player> m_player, std::weak_ptr<EnemyBase> m_enemy = {});
	//ターゲットのEnemyを取得する
	std::shared_ptr<EnemyBase> GetTargetEnemy()const;
	//ロックオンカメラをゲットする
	std::weak_ptr<LockOnManager> GetLockOnManager() { return m_lockOnManager; }
	void SetLockOnCamera(std::weak_ptr<LockOnManager> lockonMgr) {  m_lockOnManager = lockonMgr;}

	//CameraContextのゲット
	std::shared_ptr<CameraContext> GetContext() { return m_context; }
	//現在アクティブステートのカメラをゲット
	std::shared_ptr<CameraStateBase> GetActiveCamera() { return m_currentState; }
	//カメラシェイク
	void StartCameraShake(float power, float time);
	//カメラシェイクのUpdate
	Vector3 CameraShakeUpdate();

	//ロックオン
	void SetLockOn(bool isLockOn) { m_isLockOn = isLockOn; }
	bool GetIsLockOn()const { return m_isLockOn; }

	//フォトモード中、入力からカメラを動かす
	void UpdatePhotoCamera();
	void SetPhotoCamera();
	Vector3 GetPhotoCameraPos() { return m_photoCamPos; }
	Vector3 GetPhotoCameraTarget() { return m_photoCamTarget; }


public:
	void ChangeState(std::shared_ptr<CameraStateBase> newState);
	void InitState(std::shared_ptr<CameraStateBase> newState);
	//外部からステートを変えるとき
	void ChangeStateFromScene(CameraStateName stateName);
private:
	//必要な情報
	std::shared_ptr<CameraContext> m_context;

	//ロックオンマネージャー//実体はGameScene(またはシーン側)が持つため、弱参照で持つ
	std::weak_ptr<LockOnManager> m_lockOnManager;

	//カメラのステート
	std::shared_ptr<CameraStateBase> m_currentState;

	//カメラ揺れ用
	float m_shakePower = 0.0f;
	float m_shakeTimer = 0.0f;
	float m_shakeTimerMax = 0.0f;//減衰用のコピー
	bool m_isShaking = false;//今カメラが揺れているかどうか
	Vector3 m_renderPos = Vector3();//カメラ描画用の座標

	bool m_isLockOn = false;//ロックオンしているかどうか

	//フォトモード用のフリーカメラ
	Vector3 m_photoCamPos = Vector3();
	Vector3 m_photoCamTarget = Vector3();
	float m_photoAngleH = 0.0f;
	float m_photoAngleV = 0.0f;
	float m_kToTargetDistance = 500.0f;


};

