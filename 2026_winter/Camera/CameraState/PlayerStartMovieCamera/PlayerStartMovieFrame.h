#pragma once
#include "Math/Vector3.h"
#include "DxLib.h"
#include <cmath>

//プレイヤーの登場演出の基準//位置はみんな着地点と正面からの相対
struct PlayerStartMovieFrame
{
	//カットの長さ(実フレーム)
	static constexpr float kCut1Frame = 90.0f;//屋上を遠くから
	static constexpr float kCut2Frame = 45.0f;//アップ
	static constexpr float kCut3Frame = 30.0f;//飛び降りる
	static constexpr float kCut4Frame = 40.0f;//落下(上から)
	static constexpr float kCut5Frame = 25.0f;//落下(下から)→着地
	static constexpr float kLandInCut5 = 18.0f;//C5の何フレーム目に着地するか
	static constexpr float kLandPoseFrame = 50.0f;//着地のポーズ
	static constexpr float kTurnFrame = 45.0f;//背中へ回り込む

	static constexpr float kJumpStartFrame = kCut1Frame + kCut2Frame;
	static constexpr float kCutTotalFrame = kJumpStartFrame + kCut3Frame + kCut4Frame + kCut5Frame;
	static constexpr float kLandFrame = kJumpStartFrame + kCut3Frame + kCut4Frame + kLandInCut5;
	static constexpr float kTotalFrame = kCutTotalFrame + kLandPoseFrame + kTurnFrame;

	//屋上の立ち位置(横・高さ・前)//ステージ1のスタートの後ろのビルの縁
	static constexpr float kRoofSide = 175.0f;
	static constexpr float kRoofHeight = 1816.0f;
	static constexpr float kRoofFront = -655.0f;

	Vector3 origin;//着地点
	Vector3 front = Vector3(0.0f, 0.0f, 1.0f);//水平・正規化済み

	Vector3 Right()const { return Vector3(front.z, 0.0f, -front.x); }
	//横・高さ・前で指定した点
	Vector3 Point(float side, float height, float forward)const
	{
		return origin + Right() * side + Vector3(0.0f, height, 0.0f) + front * forward;
	}
	Vector3 Point(const Vector3& local)const { return Point(local.x, local.y, local.z); }
	//向きだけ(横・高さ・前)
	Vector3 Dir(const Vector3& local)const
	{
		return Right() * local.x + Vector3(0.0f, local.y, 0.0f) + front * local.z;
	}
	//着地点の正面から angleDeg 回った所(+で右回り)
	Vector3 Orbit(float angleDeg, float dist, float height)const
	{
		const float rad = angleDeg * DX_PI_F / 180.0f;
		return origin + (front * cosf(rad) + Right() * sinf(rad)) * dist + Vector3(0.0f, height, 0.0f);
	}
	Vector3 RoofPos()const { return Point(kRoofSide, kRoofHeight, kRoofFront); }
};
