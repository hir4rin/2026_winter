#pragma once
#include "PlayerState.h"
#include "../../CharacterBase.h"
class PlayerStatePartBrokenKill :
    public PlayerState
{
public:
    enum class PartBrokenKill
    {
        Start,
        Execute,
        End,
    };
    //あとで外部化するので適当//外部化の仕方は発生するタイミングや条件をすべてstructやenumでまとめる
    //↓こんな感じ
        //// フェーズ中に発火するイベント（音・エフェクト・カメラなど）
        //enum class KillEventType { PlaySE, SpawnEffect, ChangeCamera, SetTimeScale, HitEnemy };

        //struct KillEvent {
        //    float         frame;   // フェーズ開始から何フレーム目に発火するか
        //    KillEventType type;
        //    std::string   id;      // SE名／エフェクト名／カメラ名
        //    float         value;   // TimeScaleなど、必要なら
        //    bool          fired = false; // 発火済みフラグ（実行時用）
        //};

        //// 1フェーズ = 1アニメ + 移動 + イベント
        //struct KillPhase {
        //    std::string anim;
        //    float       endFrame;      // 次に進むフレーム。負なら「アニメ終了で次へ」
        //    float       moveDistance;  // 敵に近づく量（0なら動かない）
        //    std::vector<KillEvent> events;
        //};

        //struct KillSetup {
        //    std::vector<KillPhase> phases;
        //};
public:
    PlayerStatePartBrokenKill(std::weak_ptr<Player> player);
    virtual ~PlayerStatePartBrokenKill();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;
private:
    void SelectedPattern();

    void PatternAUpdate();
    void PatternBUpdate();
private:
    PartBrokenKill m_state = PartBrokenKill::Start;
    float m_startTimer = 0.0f;
    float m_excuteTimer = 0.0f;

    CharacterBase::PartBrokenPattern m_pattern = CharacterBase::PartBrokenPattern::A;
};

