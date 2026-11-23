#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"
#include "Math/Vector3.h"
#include <memory>

class BossEnemyKingmech;
class BossFinisherCamera;
class PlayerStateBossFinisher;

//トドメのムービー//全部自動//ボス・プレイヤー・カメラの流れをここで進める
//膝をつく → 長い刀で構える → 真上へ跳ぶ → 頂点で振りかぶる → 急降下 → ボスの真横に着地して斬る → 血殺 → 左右に割れる
//カメラは揺らさない。切り替えは構え(後ろから)と着地の斬り(低い真横)の2回だけ
class KingmechFinisher :
    public EnemyStateBase
{
public:
    enum class Phase
    {
        Stagger,//膝をつく//白く飛ばして位置を整える
        Stance,//長い刀で構える
        Rise,//真上へ跳ぶ
        Apex,//頂点で振りかぶったまま止まる
        Dive,//急降下
        Strike,//ボスの真横に着地しながら斬る//ここで真横のカメラへ
        Kessatsu,//着地した瞬間に止めて血殺
        Split,//左右に割れる
        Rest,//割れた後の余韻
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
    void StartCut();//真っ二つにする
    void PlayCutBlood(float scale);//切り口に沿って血を出す
    void Finish();
    void Cleanup();//タイムスケール・血殺などを戻す

    void SetShot(const Vector3& startPos, const Vector3& endPos, const Vector3& startTarget, const Vector3& endTarget,
        float startFov, float endFov, float frame);
    void UpdateKneel();//膝をついたらループへ
    void FollowPlayer(float rate);//カメラの注視点でプレイヤーを追う

    Vector3 Point(float front, float side, float height)const;//闘技場の中央から見た位置

    std::shared_ptr<BossEnemyKingmech> GetBoss()const;
    std::shared_ptr<PlayerStateBossFinisher> GetPlayerState()const;
    std::shared_ptr<BossFinisherCamera> GetCamera()const;

private:
    Phase m_phase = Phase::Stagger;
    float m_frame = 0.0f;//今のフェーズの経過時間(実フレーム)
    bool m_isEventDone = false;//フェーズの途中の出来事が済んだか
    bool m_isCleaned = false;

    Vector3 m_center;//ムービーの中心(闘技場の中央)
    Vector3 m_front;//ボス→プレイヤーの向き
    Vector3 m_side;//横の向き
    Vector3 m_landPos;//着地する位置(ボスの真横)
    Vector3 m_landDir;//ボス→着地の水平の向き//着地したプレイヤーはこの向きを向く
    Vector3 m_sideCamPos;//真横のカメラの位置(プレイヤーに寄り)
    Vector3 m_sideCamTarget;//真横のカメラの注視点
    Vector3 m_wideCamPos;//割れるときに引いた先
    Vector3 m_wideCamTarget;
};
