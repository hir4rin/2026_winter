#pragma once
#include <vector>
#include "../Math/Vector3.h"
#include "../System.h"

//Effekseerのエフェクトの再生と更新をまとめて行う
//エフェクトは再生を始めた持ち主(敵など)が消えても再生され続けるので、ここでまとめて管理する
class EffectManager
{
private:
	//コンストラクタとデストラクタをプライベートにして、シングルトンパターンを実装
	EffectManager() = default;
	~EffectManager() = default;
	EffectManager(const EffectManager&) = delete;
	EffectManager& operator=(const EffectManager&) = delete;
public:
	//シングルトンインスタンスを取得
	static EffectManager& GetInstance()
	{
		static EffectManager instance;
		return instance;
	}

	/// <summary>Systemで読み込んだエフェクトを再生する</summary>
	/// <param name="rotY">Y軸回転(ラジアン)//エフェクトの+Zを向けたい方向に合わせる</param>
	/// <returns>再生中のエフェクトのハンドル</returns>
	int Play(AsyncData type, const Vector3& pos, float rotY = 0.0f, float scale = 1.0f);

	/// <summary>
	/// タイムスケールに関係なく、このエフェクトだけspeedの速さで再生させる(スロー中でも速さを保ちたいエフェクト用)
	/// 再生が終わるまで、Updateで毎フレーム速さを設定し直す
	/// </summary>
	void SetOwnSpeed(int playingHandle, float speed);

	//毎フレーム1回呼ぶ//自分の速さを持つエフェクトの速さを設定し直してから、全エフェクトをタイムスケールに合わせて進める
	void Update();

private:
	//タイムスケールに関係なく、自分の速さで再生するエフェクト
	struct OwnSpeedEffect
	{
		int playingHandle = -1;
		float speed = 1.0f;
	};
	std::vector<OwnSpeedEffect> m_ownSpeedEffects;
};
