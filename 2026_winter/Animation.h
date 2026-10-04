#pragma once
#include <string>
#include "Math/Vector3.h"

/// <summary>
/// ルートモーションをどう見た目から消すか
/// どちらも「基準の高さより上に行った分を消す」のは同じで、基準をどのフレームから取るかが違う
/// </summary>
enum class RootMotionCancel
{
	None,//消さない
	Up,//上昇を消す//基準は最初のフレーム(地上から飛び上がるアニメーション)
	Down,//下降を消す//基準は最後のフレーム(空中から着地するアニメーション)
};

/// <summary>
/// Animationクラス(アニメーションさせるものに持たせる)
/// </summary>
class Animation
{
public:
	Animation();
	virtual ~Animation();
	/// <summary>
	/// handleとnameを受け取ってアニメーションの初期化を行う
	/// </summary>
	/// <param name="modelHandle"></param>
	/// <param name="name"></param>
	/// <param name="endFrame">アニメーションの最終フレーム(負の値なら総フレーム数を使う)</param>
	/// <param name="startFrame">アニメーションの再生を始めるフレーム</param>
	void Init(int modelHandle,std::string name,bool isRoop,float timescale = 1.0f,float endFrame = -1.0f,float startFrame = 0.0f);
	
	void Update(float ownTimeScale = 1.0f);//ここにtimeScaleを引数にして渡して、することで、プレイヤーでもエネミーでも使うと一緒に使える
	void AnimBlend(float ownTimeScale = 1.0f);//アニメーションのブレンドを行う

	void SetAnim(bool isRoop);//アニメーションのループ再生を設定する//m_isEndを初期化
	//startFrameはいろいろ完成ではないので、使い方に注意
	void ChangeAnim(std::string name,bool isRoop = true,float timescale = 1.0f,float endFrame = -1.0f,float startFrame = 0.0f);//アニメーションを切り替える//nameはアニメーションの名前//endFrameは最終フレーム(負の値なら総フレーム数)//startFrameは再生を始めるフレーム

	/// <summary>
	/// モデルを考慮したアニメーション切換え
	/// </summary>
	void ChangeAnimWithModelHandle(int modelHandle,std::string name,bool isRoop,float timescale = 1.0f,float endFrame = -1.0f,float startFrame = 0.0f);
	//モデルハンドルの取得//確認用
	int GetModelHandleForCheck() { return m_modelHandle; }
	//アニメーションを再生するのを止める
	void StopAnim();



	bool GetAnimEndFlag() { return m_isEndAnim;}//アニメーションが終わったかどうかのフラグを返す
	float GetAnimRate();//アニメーションの進行率を返す//0から1の値を返す
	float GetAnimTotalFrame(const std::string& name);//指定したアニメーションの総フレーム数を返す
	float GetNowAnimFrame();//現在のアニメーションのフレーム数を返す
	float GetNowAnimFrame(const std::string& name);//指定したアニメーションの現在のフレーム数を返す
	float GetAnimEndFrame();//現在のアニメーションの実効の最終フレームを返す//endFrameの指定がなければ総フレーム数
	float GetAnimRemainFrame();//最終フレームまでの残りフレーム数を返す
	bool IsAnimFrameOver(float frame);//現在のフレームが指定フレーム以上かどうか
	bool IsAnimFrameBetween(float startFrame, float endFrame);//現在のフレームが範囲内(start以上end未満)かどうか

	/// <summary>
	/// 現在のアニメーションのルートモーション(上下方向の移動)を見た目から消す
	/// 現在のアニメーションがデタッチされる(ブレンドが終わる)と自動で無効になる
	/// ChangeAnimの後に呼ぶこと
	/// </summary>
	/// <param name="mode">Up:上昇を消す Down:下降を消す None:解除</param>
	/// <param name="frameName">ルートモーションが入っているフレーム名(このモデルはpelvis)</param>
	void SetRootMotionEnable(RootMotionCancel mode, const char* frameName = "pelvis");
	Vector3 GetRootMotionDelta();//見た目から消した上方向の移動量の、前フレームからの差分を返す(モデル空間)
	MATRIX GetRootRotationDelta();//ルートモーションの回転量を返す//前フレームと現在のフレームのルート回転の差分を返す

	//実際に使う最終フレームを返す//endFrameが負なら総フレーム数、総フレーム数を超える場合は総フレーム数に収める
	float ResolveEndFrame(int modelHandle, int attachHandle, float endFrame);

private:
	void ApplyRootMotionCancel();//ルートフレームが基準の高さより上に行かないように上書きする
	MATRIX GetParentChainMatrix(int frameIndex);//親フレームをたどって、フレームのローカル座標をモデル空間に変換する行列を返す

	int m_modelHandle;//モデルのハンドル
	int m_currentAnimHandle;//現在のアニメーションのハンドル
	int m_prevAnimHandle;//前のアニメーションのハンドル

	float m_currentAnimCount;//現在のアニメーションのフレーム数
	float m_prevAnimCount;//前のアニメーションのフレーム数

	float m_endFrame = -1.0f;//現在のアニメーションの最終フレーム(負の値なら総フレーム数)
	float m_prevEndFrame = -1.0f;//前のアニメーションの最終フレーム

	float m_animChangeFrame;//アニメーションを切り替えるフレーム数

	bool m_isRoop;//アニメーションをループさせるかどうかのフラグ
	bool m_prevRoop;//前のアニメーションがループしているかどうかのフラグ
	bool m_isEndAnim;//アニメーションが終わったかどうかのフラグ
	bool m_isStop;

	float m_animtimeScale = 1.0f;//アニメーションの再生速度を管理するための変数//1.0fが通常の速度で、0.5fなら半分の速度、2.0fなら倍の速度になる
	float m_prevAnimTimeScale = 1.0f;//前のアニメーションの再生速度を管理するための変数

	MATRIX m_prevRootMatrix;//前フレームのルート行列
	int m_rootFrameIndex = -1;//ルートフレームのインデックス
	bool m_enableRootMotion = false;//ルートモーション有効フラグ
	int m_rootMotionAnimHandle = -1;//ルートモーションを消す対象のアニメーションのアタッチハンドル
	float m_rootBaseY = 0.0f;//基準にするルートフレームの高さ(モデル空間)//Upは最初のフレーム、Downは最後のフレーム
	float m_prevRemovedY = 0.0f;//前フレームに見た目から消した上方向の移動量
	Vector3 m_rootMotionDelta;//前フレームから消した量の差分

};

