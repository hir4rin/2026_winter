#pragma once
#include "CameraStateBase.h"

class TitleCameraState : public CameraStateBase
{
public:
	//タイトル演出のカット
	enum class Shot
	{
		Fixed,			//定点で映す
		FollowPlayer,	//プレイヤーを追従し続ける
		ZoomOut,
		Opening,
		Finish
	};

	TitleCameraState(std::weak_ptr<CameraManager> owner);
	virtual ~TitleCameraState();

	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void Draw() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::TitleCamera; }
	virtual BlendSetting GetBlendSetting()const override
	{
		return BlendSetting{
			.mode = BlendSetting::Mode::Lerp,
			.duration = kBlendDuration,
			.easingPower = 1.0f,
			.pivot = Vector3()
		};
	}


	void SetShot(Shot shot) { m_shot = shot; }
	Shot GetShot()const { return m_shot; }

	/// <summary>
	/// 前振り演出をスキップして最後のOpeningショットまで一気に進める
	/// (ZoomOutが完了した直後の状態を再現してからOpeningに入る)
	/// </summary>
	/// <param name="playerPos">スキップ後のプレイヤー座標</param>
	void SkipToOpening(const Vector3& playerPos);
private:
	static constexpr float kBlendDuration = 5.0f;//ブレンドにかけるフレーム数

	Shot m_shot = Shot::Fixed;
	int m_count = 0;
	int m_count_zoom = 0;
};

