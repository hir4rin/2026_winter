#pragma once

//============================================================================
// 血殺(確殺)演出のお試し版：背景を赤で塗り、ステージを赤いフォグに沈める
//
// ・下の BLOOD_KILL_FOG_ENABLE を 0 にすると、全関数が何もしなくなる(まとめてオフ)
// ・完全に消すときは、このファイルと.cppを削除して「[BloodKillFog]」で検索した呼び出しを消す
//============================================================================
#define BLOOD_KILL_FOG_ENABLE 1

class BloodKillFog
{
public:
	/// <summary>毎フレーム呼ぶ。演出中ならフェードイン、終わったらフェードアウトする</summary>
	void Update(bool isActive);

	/// <summary>背景を赤で塗る(3Dを描く前に呼ぶ)</summary>
	void DrawBackground()const;

	/// <summary>ステージを描く直前に呼ぶ(フォグとカラースケールをかける)</summary>
	void BeginStage(int stageViewHandle)const;

	/// <summary>ステージを描いた直後に呼ぶ(フォグとカラースケールを戻す)</summary>
	void EndStage(int stageViewHandle)const;

private:
	float m_rate = 0.0f;//演出のかかり具合(0:通常～1:最大)
};
