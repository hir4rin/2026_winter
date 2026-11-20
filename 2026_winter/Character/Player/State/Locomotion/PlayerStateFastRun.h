#pragma once
#include "Character/Player/PlayerState.h"

class Input;

class PlayerStateFastRun :   public PlayerState
{
public:
    PlayerStateFastRun(std::weak_ptr<Player> player);
    virtual ~PlayerStateFastRun();
    void Enter() override;
    void Update() override;
	void Exit() override;

    void DebugDraw() override;

private:
	void Move(Input& input);//移動処理
	void UpdateRunEffect();//ダッシュ中のループのエフェクトを体に付ける

	int m_runEffectHandle = -1;//肩の帯からこぼれる光の粒(ループ、再生中)
};

