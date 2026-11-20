#pragma once
#include "Camera/CameraState/CameraStateBase.h"
#include "PlayerStartMovieFrame.h"
#include <vector>

//登場演出のC1〜C5//カットで切り替える部分をまとめて持つ
class PlayerStartMovieCutCamera :
    public CameraStateBase
{
public:
	PlayerStartMovieCutCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame);
	virtual ~PlayerStartMovieCutCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	Type GetCameraType()const override { return Type::PlayerStartMovieCutCamera; }
	virtual BlendSetting GetBlendSetting()const override { return {}; }

private:
	//位置・注視点を何からの相対にするか
	enum class Follow
	{
		None,//着地点
		Target,//注視点だけプレイヤー
		All,//位置も注視点もプレイヤー
	};
	//位置は(横・高さ・前)
	struct Cut
	{
		Vector3 startPos;
		Vector3 endPos;
		Vector3 startTarget;
		Vector3 endTarget;
		float startFov = 60.0f;//度
		float endFov = 60.0f;
		float frame = 1.0f;
		Follow follow = Follow::None;
		EasingMode easing = EasingMode::EaseInOut;
	};

	void BuildCuts();
	void ApplyCut();
	Vector3 ToWorld(const Vector3& local, bool isPlayer)const;

private:
	PlayerStartMovieFrame m_frame;
	std::vector<Cut> m_cuts;
	int m_index = 0;
	float m_cutFrame = 0.0f;
};
