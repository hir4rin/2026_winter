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
		PartBrokenACameraEnd,
		PartBrokenBCamera,
		PartBrokenBCameraStart,
		UltStartCamera,
		SkillKessatuCamera,
		AirAssasinCamera,
		AirAssasinCameraStart,
		BattleSmoothCamera

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
	//Fovパンチ
	void StartFovPunch(float angleDeg, float time, float inFrame, float outPower);//+で広げる、-で挟める
	float FovPunchUpdate();//今フレームのずれ(ラジアン)。
	//Fovオフセット//戻すまでずっと足す(+で広げる)
	void SetFovOffset(float angleDeg, float blendFrame);//今の値からblendFrameかけて変える
	void ResetFovOffset(float blendFrame);//0に戻す
	float FovOffsetUpdate();//今フレームのずれ(ラジアン)//Updateで1回だけ呼ぶ


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

	//プレイヤーの攻撃がヒットしたとき//PlayerCamera中だけBattleSmoothCameraに遷移する
	void OnPlayerAttackHit();

public:
	void ChangeState(std::shared_ptr<CameraStateBase> newState);
	void InitState(std::shared_ptr<CameraStateBase> newState);
	//外部からステートを変えるとき
	void ChangeStateFromScene(CameraStateName stateName);


private:
	//ロックオン状態とカメラのステートがずれていたら遷移する
	void SyncLockOnState();
	//ステージに当たらない位置に補正する
	Vector3 FixStageCollision(const Vector3& pos, const Vector3& target);
	//今のカメラがステージとの当たり判定をするか
	bool IsStageCollisionCamera() const;
	//今のカメラにFovオフセットを足すか//演出カメラは足さない
	bool IsFovOffsetCamera() const;
private:
	std::weak_ptr<Stage> m_stage;
	float m_collisionDist = -1.0f;//補正後の注視点からの距離(-1なら補正なし)

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


	//Fov用
	float m_renderFov = DX_PI_F / 3.0f;//カメラ描画用の視野角
	float m_fovPunchAngle = 0.0f;//パンチの強さ(ラジアン)
	float m_fovPunchOffset = 0.0f;//今フレームのずれ
	float m_fovPunchTimer = 0.0f;
	float m_fovPunchMaxTime = 0.0f;
	float m_fovPunchInFrame = 0.0f;//入りのフレーム数
	float m_fovPunchOutPower = 0.0f;//戻りのイージングの指数
	float m_fovPunchCoolTimer = 0.0f;//次のパンチを受け付けるまでの残り

	//Fovオフセット用(ラジアン)
	float m_fovOffset = 0.0f;//今の値
	float m_fovOffsetStart = 0.0f;//変え始めの値
	float m_fovOffsetGoal = 0.0f;//目標の値
	float m_fovOffsetTimer = 0.0f;
	float m_fovOffsetBlendFrame = 0.0f;
	float m_fovOffsetWeight = 1.0f;//効き具合(0~1)//演出カメラ中は0へ

	//フォトモード用のフリーカメラ
	Vector3 m_photoCamPos = Vector3();
	Vector3 m_photoCamTarget = Vector3();
	float m_photoAngleH = 0.0f;
	float m_photoAngleV = 0.0f;
	float m_kToTargetDistance = 500.0f;


};

