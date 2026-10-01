#pragma once
#include <memory>
#include <list>
#include <vector>
#include "../Math/Vector3.h"

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
		ResultCamera,
		AssasinCamera,
		AssasinCameraStart,
		PartBrokenACamera,
		PartBrokenACameraStart,
		PartBrokenBCamera,
		PartBrokenBCameraStart

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
	void Update();
	void Draw();

	/// <summary>
	/// レンダーターゲットごとに変わってしまうので、カメラの設定を反映させる
	/// </summary>
	void ApplyCameraSettings();

	//CameraContextのゲット
	std::shared_ptr<CameraContext> GetContext() { return m_context; }
	//現在アクティブステートのカメラをゲット
	std::shared_ptr<CameraStateBase> GetActiveCamera() { return m_currentState; }
	//カメラシェイク
	void StartCameraShake(float power, float time);
	//カメラシェイクのUpdate
	Vector3 CameraShakeUpdate();

	//ロックオン//Player経由で読むだけ
	bool IsLockOn()const;
	//ロックオンしていたらロックオンの敵を返し、違ったらnullptrを返す
	std::shared_ptr<EnemyBase> GetLockTarget()const;
	//攻撃の対象(ロックオンor内部ターゲット)を返す、違ったらnullptrを返す
	std::shared_ptr<EnemyBase> GetAttackTarget()const;


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
	//ロックオン状態とカメラのステートがずれていたら遷移する
	void SyncLockOnState();
private:
	//必要な情報
	std::shared_ptr<CameraContext> m_context;


	//カメラのステート
	std::shared_ptr<CameraStateBase> m_currentState;

	//カメラ揺れ用
	float m_shakePower = 0.0f;
	float m_shakeTimer = 0.0f;
	float m_shakeTimerMax = 0.0f;//減衰用のコピー
	bool m_isShaking = false;//今カメラが揺れているかどうか
	Vector3 m_renderPos = Vector3();//カメラ描画用の座標

	//フォトモード用のフリーカメラ
	Vector3 m_photoCamPos = Vector3();
	Vector3 m_photoCamTarget = Vector3();
	float m_photoAngleH = 0.0f;
	float m_photoAngleV = 0.0f;
	float m_kToTargetDistance = 500.0f;


};

