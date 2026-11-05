#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"
#include "Math/Vector3.h"
#include <memory>

class BossEnemyKingmech;
class BossFinisherCamera;
class PlayerStateBossFinisher;

//トドメのムービー//全部自動//ボス・プレイヤー・カメラの流れをここで進める
//補給 → 切り落とし → 投げて胸に刺す → 血殺の一閃 → 納刀 → 真っ二つ → 爆発
class KingmechFinisher :
    public EnemyStateBase
{
public:
    enum class Phase
    {
        Stagger,//膝をつく//白く飛ばして位置を整える
        Supply,//最後のガトリングが落ちてくる
        Cut,//飛び上がって切り落とす
        Throw,//投げる
        Silence,//刺さって一瞬止まる
        Slash,//血殺の一閃
        Sheath,//背を向けて納刀
        Split,//上半身がずれる
        Explode,//誘爆
        Done,//終わり
    };

    KingmechFinisher(std::weak_ptr<EnemyBase> owner);
    virtual ~KingmechFinisher();

    void Enter() override;
    void Update()override;
    void Exit()override;

    const char* GetName()const override { return "KingmechFinisher"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Hit; }

private:
    void ChangePhase(Phase next);
    void SetupStage();//白の間に位置を整える
    void StartSlashHit();//刃が胸を通った瞬間
    void PlayWhiteBlood(float scale);
    void PlayExplosions();
    void Finish();
    void Cleanup();//タイムスケール・血殺などを戻す

    void SetShot(const Vector3& startPos, const Vector3& endPos, const Vector3& startTarget, const Vector3& endTarget,
        float startFov, float endFov, float frame);
    void Shake(float power, float time);
    void FovPunch(float angleDeg, float time);
    void UpdateKneel();//膝をついたらループへ

    Vector3 Point(float front, float side, float height)const;//闘技場の中央から見た位置

    std::shared_ptr<BossEnemyKingmech> GetBoss()const;
    std::shared_ptr<PlayerStateBossFinisher> GetPlayerState()const;
    std::shared_ptr<BossFinisherCamera> GetCamera()const;

private:
    Phase m_phase = Phase::Stagger;
    float m_frame = 0.0f;//今のフェーズの経過時間(実フレーム)
    bool m_isEventDone = false;//フェーズの途中の出来事が済んだか
    bool m_isSecondEventDone = false;
    bool m_isCleaned = false;

    Vector3 m_center;//ムービーの中心(闘技場の中央)
    Vector3 m_front;//ボス→プレイヤーの向き
    Vector3 m_side;//横の向き
    Vector3 m_airPos;//プレイヤーが空中で止まる位置
    Vector3 m_cutPoint;//斬る位置
};
