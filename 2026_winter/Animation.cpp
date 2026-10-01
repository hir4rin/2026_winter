#include "Animation.h"
#include "DxLib.h"
#include <algorithm>
#include "System.h"

namespace
{
	constexpr int kAnimChangeFrame = 20;//アニメーションを切り替えるフレーム数//ブレンドのフレームもアニメーションごとに変えたい

	
	
}

Animation::Animation() :
	m_modelHandle(-1),
	m_currentAnimHandle(-1),
	m_prevAnimHandle(-1),
	m_currentAnimCount(0.0f),
	m_prevAnimCount(0.0f),
	m_animChangeFrame(0),
	m_isRoop(false),
	m_prevRoop(false),
	m_isEndAnim(false)
{
}

Animation::~Animation()
{
	//モデルをメモリから解放する
	MV1DeleteModel(m_modelHandle);
}

void Animation::Init(int modelHandle, std::string name, bool isRoop, float timescale, float endFrame)
{
	//古いモデルに残っているルートモーションの上書きを解除する
	SetRootMotionEnable(RootMotionCancel::None);
	// 違うモデルに切り替える前に、古いモデルのアニメーションを全てデタッチする
	if (m_currentAnimHandle != -1)
	{
		MV1DetachAnim(m_modelHandle, m_currentAnimHandle);
		m_currentAnimHandle = -1;
	}
	if (m_prevAnimHandle != -1)
	{
		MV1DetachAnim(m_modelHandle, m_prevAnimHandle);
		m_prevAnimHandle = -1;
	}
	//m_prevAnimCount = 0.0f;
	m_animChangeFrame = 0.0f;

	m_endFrame = endFrame;//最終フレームを保存する
	m_prevEndFrame = -1.0f;
	SetAnim(isRoop);
	//モデルのハンドルを保存する
	m_modelHandle = modelHandle;

	//animation名からアニメーションのインデックスを取得する
	int animNo = MV1GetAnimIndex(modelHandle, name.c_str());

	//m_animHandleに入れることで、アニメーションを変えるときや、ブレンドするときにm_animHandleを使う
	m_currentAnimHandle = MV1AttachAnim(m_modelHandle, animNo, -1, -1);//アニメーションをアタッチする
	m_currentAnimCount = 0.0f;//アニメーションのフレーム数を0にする

	m_prevAnimHandle = -1;//前のアニメーションのハンドルを-1にする
	m_prevAnimCount = 0.0f;//前のアニメーションのフレーム数を0にする
	m_animtimeScale = timescale;//アニメーションの再生速度を設定する
}


void Animation::Update(float ownTimeScale)
{
	//float timeScale = System::GetInstance().GetTimeScale();//時間のスケールを取得する//0から1の値を返す//0.5なら、時間が半分になる
	float timeScale = System::GetInstance().GetTimeScale();
	//アニメーションの更新
	m_currentAnimCount += 1.0f * timeScale * m_animtimeScale * ownTimeScale;//アニメーションのフレーム数を増やす
	m_prevAnimCount += 1.0f * timeScale * m_prevAnimTimeScale * ownTimeScale;//前のアニメーションのフレーム数を増やす

	//アニメーションのブレンド
	AnimBlend(ownTimeScale);
	//アニメーションのループ再生
	//アタッチしているアニメーションの総フレーム数を取得する
	float totalAnimCount = ResolveEndFrame(m_modelHandle, m_currentAnimHandle, m_endFrame);
	if (m_currentAnimCount >= totalAnimCount)
	{
		m_isEndAnim = true;//アニメーションが終わったフラグを立てる
		if (m_isRoop)
		{
			m_currentAnimCount -= totalAnimCount;//アニメーションのフレーム数がアニメーションの総フレーム数を超えたら、0にする
		}
		else
		{
			m_currentAnimCount = totalAnimCount;//止める
		}
	}
	//アニメーションのカウントを更新する
	MV1SetAttachAnimTime(m_modelHandle, m_currentAnimHandle, m_currentAnimCount);
	//前のアニメーションのループ再生
	if (m_prevAnimHandle != -1)
	{
		float totalPrevAnimCount = ResolveEndFrame(m_modelHandle, m_prevAnimHandle, m_prevEndFrame);
		if (m_prevAnimCount >= totalPrevAnimCount)
		{
			if (m_prevRoop)
			{
				m_prevAnimCount -= totalPrevAnimCount;//前のアニメーションのフレーム数が前のアニメーションの総フレーム数を超えたら、0にする
			}
			else
			{
				m_prevAnimCount = totalPrevAnimCount;//止める
			}
		}

		MV1SetAttachAnimTime(m_modelHandle, m_prevAnimHandle, m_prevAnimCount);//前のアニメーションのフレーム数を更新する
	}
	//アニメーションの時間を進めた後に、ルートモーションを見た目から消す
	ApplyRootMotionCancel();
}

void Animation::AnimBlend(float ownTimeScale)
{
	if (m_prevAnimHandle == -1)
	{
		//アニメーションが一つしかないときは、ブレンドしない
		MV1SetAttachAnimBlendRate(m_modelHandle, m_currentAnimHandle, 1.0f);//現在のアニメーションのブレンド率を1にする
	}
	else
	{
		//timeScale
		//float timeScale = System::GetInstance().GetTimeScale();//時間のスケールを取得する//0から1の値を返す//0.5なら、時間が半分になる
		float timeScale = System::GetInstance().GetTimeScale();

		//m_animtimeScaleを足さないといけないと思った//どっちのm_animTimeScaleを足すか不明なのでいったんパス
		m_animChangeFrame += 1.0f * timeScale * ownTimeScale;//アニメーションを切り替えるフレーム数を増やす

		//currentAnimBlendのブレンド率を計算
		float rate = m_animChangeFrame / kAnimChangeFrame;//アニメーションを切り替えるフレーム数で割ることで、0から1までの値を作る
		if (rate > 1.0f)
		{
			rate = 1.0f;//ブレンド率が1を超えないようにする
			//ブレンド完了
			MV1DetachAnim(m_modelHandle, m_prevAnimHandle);
			m_prevAnimHandle = -1;
			m_prevAnimCount = 0.0f;
		}

		MV1SetAttachAnimBlendRate(m_modelHandle, m_currentAnimHandle, rate);//現在のアニメーションのブレンド率を設定する
		if (m_prevAnimHandle != -1)//prevが有効な時だけ呼ぶ//prevが-1の時にSetAttachAnimBlendRateを呼ぶと、そのハンドルのブレンド率が0になってしまうので、呼ばないようにする
		{
			MV1SetAttachAnimBlendRate(m_modelHandle, m_prevAnimHandle, 1.0f - rate);//前のアニメーションのブレンド率を設定する
		}
	}
}


void Animation::SetAnim(bool isRoop)
{
	m_prevRoop = m_isRoop;
	//ループするかどうかの保存
	m_isRoop = isRoop;
	//アニメーションの終了フラグを初期化
	m_isEndAnim = false;
}



void Animation::ChangeAnim(std::string name, bool isRoop, float timescale, float endFrame)
{
	m_prevEndFrame = m_endFrame;//現在の最終フレームを前のアニメーション用に保存する
	m_endFrame = endFrame;
	m_prevAnimTimeScale = m_animtimeScale;//前のアニメーションの再生速度を保存する
	m_animtimeScale = timescale;//アニメーションの再生速度を設定する
	//m_animtimeScale = 1.0f;

	//アニメーションの切り替え
	//攻撃終了の旗、ループするかどうかの旗を初期化
	SetAnim(isRoop);
	//新しくアニメーションをセットしようとしたとき
	//古いアニメーションが残っている場合、それをデタッチ
	if (m_prevAnimHandle != -1)
	{
		MV1DetachAnim(m_modelHandle, m_prevAnimHandle);//前のアニメーションをデタッチする
		m_prevAnimHandle = -1;//前のアニメーションのハンドルを-1にする
		m_prevAnimCount = 0.0f;//前のアニメーションのフレーム数を0にする
	}
	//現在再生中のアニメーションを一つ前のアニメーションとする
	m_prevAnimHandle = m_currentAnimHandle;//現在のアニメーションを保存する
	m_prevAnimCount = m_currentAnimCount;//現在のアニメーションのフレーム数を保存する
	//デタッチしたら消えてしまうので、デタッチせず、新しいアニメーションをアタッチする
	int animNo = MV1GetAnimIndex(m_modelHandle, name.c_str());//アニメーション名からアニメーションのインデックスを取得する
	m_currentAnimHandle = MV1AttachAnim(m_modelHandle, animNo, -1, -1);//新しいアニメーションをアタッチする



	MV1SetAttachAnimBlendRate(m_modelHandle, m_currentAnimHandle, 0.0f); // 新アニメを非表示から開始
	MV1SetAttachAnimBlendRate(m_modelHandle, m_prevAnimHandle, 1.0f);    // 旧アニメを完全表示に明示
	m_currentAnimCount = 0.0f;//新しいアニメーションのフレーム数を0にする
	m_animChangeFrame = 0.0f;//アニメーションを切り替えるフレーム数を0にする


}

void Animation::ChangeAnimWithModelHandle(int modelHandle, std::string name, bool isRoop, float timescale, float endFrame)
{
	if (m_modelHandle == modelHandle)
	{
		//同じモデルなら、ブレンド遷移
		ChangeAnim(name, isRoop, timescale, endFrame);
	}
	else
	{
		//古いモデルに残っているルートモーションの上書きを解除する
		SetRootMotionEnable(RootMotionCancel::None);
		// 違うモデルに切り替える前に、古いモデルのアニメーションを全てデタッチする
		if (m_currentAnimHandle != -1)
		{
			MV1DetachAnim(m_modelHandle, m_currentAnimHandle);
			m_currentAnimHandle = -1;
		}
		if (m_prevAnimHandle != -1)
		{
			MV1DetachAnim(m_modelHandle, m_prevAnimHandle);
			m_prevAnimHandle = -1;
		}
		//m_prevAnimCount = 0.0f;
		m_animChangeFrame = 0.0f;

		//違うモデルなら、Initで初期化//ブレンドなし	
		Init(modelHandle, name, isRoop, timescale, endFrame);
	}
}

void Animation::StopAnim()
{
}

float Animation::GetAnimRate()
{
	float totalAnimCount = ResolveEndFrame(m_modelHandle, m_currentAnimHandle, m_endFrame);//最終フレーム(指定がなければ総フレーム数)を取得する
	if (totalAnimCount <= 0.0f)	return 0.0f;//総フレーム数が0以下のときは、アニメーションの進行率を0にする//0で割るのを防ぐ

	//アニメーションの進行割合
	float rate = m_currentAnimCount / totalAnimCount;//アニメーションのフレーム数をアニメーションの総フレーム数で割ることで、0から1までの値を作る
	return rate;
}

float Animation::GetAnimTotalFrame(const std::string& name)
{
	//MV1GetAttachAnimTotalTimeはアタッチ番号を受け取るので、アニメーション番号を渡すと別のアニメ(or -1)の値になる
	//アニメーション番号から直接総フレーム数を取る
	int animIndex = MV1GetAnimIndex(m_modelHandle, name.c_str());
	if (animIndex == -1) return 0.0f;//アニメーションが存在しない場合は0を返す
	float totalAnimCount = MV1GetAnimTotalTime(m_modelHandle, animIndex);
	return totalAnimCount;
}

float Animation::GetNowAnimFrame()
{
	return m_currentAnimCount;
}

float Animation::GetNowAnimFrame(const std::string& name)
{
	int animIndex = MV1GetAnimIndex(m_modelHandle, name.c_str());
	if (animIndex == -1) return 0.0f;//アニメーションが存在しない場合は0を返す
	float animFrame = MV1GetAttachAnimTime(m_modelHandle, animIndex);//アタッチしているアニメーションの現在のフレーム数を取得する

	return animFrame;
}

float Animation::GetAnimEndFrame()
{
	return ResolveEndFrame(m_modelHandle, m_currentAnimHandle, m_endFrame);
}

float Animation::GetAnimRemainFrame()
{
	return (std::max)(0.0f, GetAnimEndFrame() - m_currentAnimCount);
}

bool Animation::IsAnimFrameOver(float frame)
{
	return m_currentAnimCount >= frame;
}

bool Animation::IsAnimFrameBetween(float startFrame, float endFrame)
{
	return m_currentAnimCount >= startFrame && m_currentAnimCount < endFrame;
}

void Animation::SetRootMotionEnable(RootMotionCancel mode, const char* frameName)
{
	//前の上書きが残っていたら解除する
	if (m_enableRootMotion && m_rootFrameIndex >= 0)
	{
		MV1ResetFrameUserLocalMatrix(m_modelHandle, m_rootFrameIndex);
	}
	m_enableRootMotion = false;
	m_rootMotionDelta = Vector3();
	m_prevRemovedY = 0.0f;
	if (mode == RootMotionCancel::None)return;

	m_rootFrameIndex = MV1SearchFrame(m_modelHandle, frameName);
	if (m_rootFrameIndex < 0)return;//フレームが見つからなければ何もしない

	m_enableRootMotion = true;
	m_rootMotionAnimHandle = m_currentAnimHandle;//今アタッチしているアニメーションが対象

	//基準にするフレーム//Up:最初のフレーム(立っている高さ) Down:最後のフレーム(着地して立った高さ)
	//どちらも基準より下の動き(しゃがみ込み、着地の沈み込みなど)は残す
	float baseFrame = 0.0f;
	if (mode == RootMotionCancel::Down)
	{
		baseFrame = ResolveEndFrame(m_modelHandle, m_currentAnimHandle, m_endFrame);
		//endFrameの指定がないときは総フレーム数になるが、最後のキーはループ用に先頭の姿勢に戻っていることがあるので1フレーム手前を使う
		if (m_endFrame < 0.0f)baseFrame = (std::max)(0.0f, baseFrame - 1.0f);
	}

	//アニメーションの時間を一瞬だけ基準フレームにして高さを取り、元に戻す
	float nowFrame = MV1GetAttachAnimTime(m_modelHandle, m_currentAnimHandle);
	MV1SetAttachAnimTime(m_modelHandle, m_currentAnimHandle, baseFrame);
	VECTOR baseLocalPos = MV1GetAttachAnimFrameLocalPosition(m_modelHandle, m_currentAnimHandle, m_rootFrameIndex);
	MV1SetAttachAnimTime(m_modelHandle, m_currentAnimHandle, nowFrame);

	m_rootBaseY = VTransform(baseLocalPos, GetParentChainMatrix(m_rootFrameIndex)).y;
}

Vector3 Animation::GetRootMotionDelta()
{
	return m_rootMotionDelta;
}

void Animation::ApplyRootMotionCancel()
{
	if (!m_enableRootMotion)return;

	//対象のアニメーションがデタッチされた(ブレンドも終わった)ら無効にする
	if (m_rootMotionAnimHandle != m_currentAnimHandle && m_rootMotionAnimHandle != m_prevAnimHandle)
	{
		SetRootMotionEnable(RootMotionCancel::None);
		return;
	}

	//上書きを一旦外して、ブレンド込みのアニメーションの行列を取得する
	MV1ResetFrameUserLocalMatrix(m_modelHandle, m_rootFrameIndex);
	MATRIX localMat = MV1GetFrameLocalMatrix(m_modelHandle, m_rootFrameIndex);

	//ローカル座標をモデル空間に変換して、上方向(Y)を見る//FBX(Z上)からの軸の違いをここで吸収する
	MATRIX parentMat = GetParentChainMatrix(m_rootFrameIndex);
	VECTOR modelPos = VTransform(VGet(localMat.m[3][0], localMat.m[3][1], localMat.m[3][2]), parentMat);

	//基準より上に行った分だけ消す
	float removedY = (std::max)(0.0f, modelPos.y - m_rootBaseY);
	modelPos.y -= removedY;

	//モデル空間からローカル座標に戻して、行列の移動成分を差し替え
	VECTOR localPos = VTransform(modelPos, MInverse(parentMat));
	localMat.m[3][0] = localPos.x;
	localMat.m[3][1] = localPos.y;
	localMat.m[3][2] = localPos.z;
	MV1SetFrameUserLocalMatrix(m_modelHandle, m_rootFrameIndex, localMat);

	m_rootMotionDelta = Vector3(0.0f, removedY - m_prevRemovedY, 0.0f);
	m_prevRemovedY = removedY;
}

MATRIX Animation::GetParentChainMatrix(int frameIndex)
{
	MATRIX mat = MGetIdent();
	//DxLibは行ベクトルなので、子→親の順に掛ける
	for (int parent = MV1GetFrameParent(m_modelHandle, frameIndex); parent >= 0; parent = MV1GetFrameParent(m_modelHandle, parent))
	{
		mat = MMult(mat, MV1GetFrameLocalMatrix(m_modelHandle, parent));
	}
	return mat;
}

MATRIX Animation::GetRootRotationDelta()
{
	if (!m_enableRootMotion)return MGetIdent();//ルートモーションが無効なときは、回転量を0にする

	MATRIX currentRotationMatrix = MV1GetFrameLocalMatrix(m_modelHandle, m_rootFrameIndex);//現在のルートフレームの行列を取得する	

	//回転量の計算
	//前フレームの逆行列 * 現在行列 = 差分回転
	MATRIX delta = MMult(MInverse(m_prevRootMatrix), currentRotationMatrix);

	return delta;
}

float Animation::ResolveEndFrame(int modelHandle, int attachHandle, float endFrame)
{
	float total = MV1GetAttachAnimTotalTime(modelHandle, attachHandle);
	if (endFrame < 0.0f) return total;
	return (std::min)(endFrame, total);	
}
