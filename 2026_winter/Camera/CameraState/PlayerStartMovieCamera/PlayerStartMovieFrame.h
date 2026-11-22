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
	static constexpr float kTurnFrame = 60.0f;//背中へ回り込む

	static constexpr float kJumpStartFrame = kCut1Frame + kCut2Frame;
	static constexpr float kCutTotalFrame = kJumpStartFrame + kCut3Frame + kCut4Frame + kCut5Frame;
	static constexpr float kLandFrame = kJumpStartFrame + kCut3Frame + kCut4Frame + kLandInCut5;
	static constexpr float kTotalFrame = kCutTotalFrame + kLandPoseFrame + kTurnFrame;

	//屋上の立ち位置(横・高さ・前)//ステージ1のスタートの後ろのビルの縁
	static constexpr float kRoofSide = 175.0f;
	static constexpr float kRoofHeight = 1816.0f;
	static constexpr float kRoofFront = -655.0f;
	static constexpr float kRoofApexHeight = 1966.0f;//着地点の上の高さ

	//チュートリアル//列車から落ちてきた続き(C1と屋上は無し)
	static constexpr float kFallCut2Frame = 70.0f;//落ちながらアップ(フェードインを含む)
	static constexpr float kFallCut3Frame = 30.0f;//真横//体勢を立て直して振りかぶる
	static constexpr float kFallApexFrame = kFallCut2Frame + kFallCut3Frame;
	static constexpr float kFallLandFrame = kFallApexFrame + kCut4Frame + kLandInCut5;
	static constexpr float kFallTotalFrame = kFallApexFrame + kCut4Frame + kCut5Frame + kLandPoseFrame + kTurnFrame;
	//落ち始め(横・高さ)//列車から跳んだ向きから斜めに
	static constexpr float kFallStartSide = -350.0f;
	static constexpr float kFallStartHeight = 2200.0f;
	static constexpr float kFallApexHeight = 1400.0f;

	enum class Kind
	{
		Roof,//ステージ1//屋上から飛ぶ
		Fall,//チュートリアル//空から落ちてくる
	};
	Kind kind = Kind::Roof;

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
	Vector3 FallStartPos()const { return Point(kFallStartSide, kFallStartHeight, 0.0f); }

	//種類で変わるもの
	bool IsFall()const { return kind == Kind::Fall; }
	float ApexFrame()const { return IsFall() ? kFallApexFrame : kJumpStartFrame + kCut3Frame; }//着地点の真上に着く
	float LandFrame()const { return IsFall() ? kFallLandFrame : kLandFrame; }
	float TotalFrame()const { return IsFall() ? kFallTotalFrame : kTotalFrame; }
	Vector3 ApexPos()const { return Point(0.0f, IsFall() ? kFallApexHeight : kRoofApexHeight, 0.0f); }
};
