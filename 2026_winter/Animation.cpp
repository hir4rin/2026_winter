#include "Animation.h"
#include "DxLib.h"
//#include "../System.h"

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

void Animation::Init(int modelHandle, std::string name, bool isRoop, float timescale)
{
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
	float timeScale = 1.0f;
	//アニメーションの更新
	m_currentAnimCount += 1.0f * timeScale * m_animtimeScale * ownTimeScale;//アニメーションのフレーム数を増やす
	m_prevAnimCount += 1.0f * timeScale * m_prevAnimTimeScale * ownTimeScale;//前のアニメーションのフレーム数を増やす

	//アニメーションのブレンド
	AnimBlend(ownTimeScale);
	//アニメーションのループ再生
	//アタッチしているアニメーションの総フレーム数を取得する
	float totalAnimCount = MV1GetAttachAnimTotalTime(m_modelHandle, m_currentAnimHandle);
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
		float totalPrevAnimCount = MV1GetAttachAnimTotalTime(m_modelHandle, m_prevAnimHandle);
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
		float timeScale = 1.0f;

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



void Animation::ChangeAnim(std::string name, bool isRoop, float timescale)
{
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

void Animation::ChangeAnimWithModelHandle(int modelHandle, std::string name, bool isRoop, float timescale)
{
	if (m_modelHandle == modelHandle)
	{
		//同じモデルなら、ブレンド遷移
		ChangeAnim(name, isRoop, timescale);
	}
	else
	{
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
		Init(modelHandle, name, isRoop, timescale);
	}
}

void Animation::StopAnim()
{
}

float Animation::GetAnimRate()
{
	float totalAnimCount = MV1GetAttachAnimTotalTime(m_modelHandle, m_currentAnimHandle);//アタッチしているアニメーションの総フレーム数を取得する
	if (totalAnimCount <= 0.0f)	return 0.0f;//総フレーム数が0以下のときは、アニメーションの進行率を0にする//0で割るのを防ぐ

	//アニメーションの進行割合
	float rate = m_currentAnimCount / totalAnimCount;//アニメーションのフレーム数をアニメーションの総フレーム数で割ることで、0から1までの値を作る
	return rate;
}

float Animation::GetAnimTotalFrame(const std::string& name)
{
	float totalAnimCount = MV1GetAttachAnimTotalTime(m_modelHandle, MV1GetAnimIndex(m_modelHandle, name.c_str()));//アタッチしているアニメーションの総フレーム数を取得する
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

void Animation::SetRootMotionEnable(bool enable, int rootFrameIndex)
{
	m_enableRootMotion = enable;
	m_rootFrameIndex = rootFrameIndex;
	m_prevRootMatrix = MV1GetFrameLocalMatrix(m_modelHandle, m_rootFrameIndex);//ルートフレームの行列を保存する
}

Vector3 Animation::GetRootMotionDelta()
{
	if (!m_enableRootMotion)return Vector3();//ルートモーションが無効なときは、移動量を0にする

	MATRIX currentRootMatrix = MV1GetFrameLocalMatrix(m_modelHandle, m_rootFrameIndex);//現在のルートフレームの行列を取得する

	//移動量の計算
	float deltaX = currentRootMatrix.m[3][0] - m_prevRootMatrix.m[3][0];//現在のルート位置と前のルート位置の差分を計算する
	float deltaY = currentRootMatrix.m[3][1] - m_prevRootMatrix.m[3][1];
	float deltaZ = currentRootMatrix.m[3][2] - m_prevRootMatrix.m[3][2];

	Vector3 delta = Vector3(deltaX, deltaY, deltaZ);
	m_prevRootMatrix = currentRootMatrix;//現在のルート行列を保存する

	return delta;
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
