#pragma once
#include "../General/EnemyStateBase.h"
#include "../../../CharacterBase.h"
class EnemyPartBrokenKilled :
    public EnemyStateBase
{
public:
    enum class PartBrokenKill
    {
        Start,
        Execute,
        End,
    };
public:
    EnemyPartBrokenKilled(std::weak_ptr<EnemyBase> owner, CharacterBase::PartBrokenPattern pattern);
    virtual ~EnemyPartBrokenKilled();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    void PatternAUpdate();
    void PatternBUpdate();
private:
    PartBrokenKill m_state = PartBrokenKill::Start;
    float m_startTimer = 0.0f;
    float m_excuteTimer = 0.0f;

    CharacterBase::PartBrokenPattern m_pattern;

};

