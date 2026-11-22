#pragma once
#include "Character/Player/PlayerState.h"
#include "Camera/CameraState/PlayerStartMovieCamera/PlayerStartMovieFrame.h"

//登場演出//屋上から飛ぶ(ステージ1)か空から落ちてきて(チュートリアル)、空中強攻撃で着地点へ突き下ろして立ち上がる(全部自動)
class PlayerStateStartMovie :
    public PlayerState
{
public:
    PlayerStateStartMovie(std::weak_ptr<Player> player, const PlayerStartMovieFrame& frame);
    virtual ~PlayerStateStartMovie();

    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw()override;
    bool IsInvincible()const override { return true; }

private:
    enum class Phase
    {
        Roof,//屋上で立つ
        Leap,//踏み切って着地点の上へ
        Fall,//空から着地点の上へ落ちてくる
        Plunge,//真下へ突き下ろす
        Land,//着地のポーズ
        Stand,//立つ
    };

    void UpdateLeap(Player& player);
    void UpdateFall(Player& player);
    void StartAttackAnim(Player& player);
    void UpdatePlunge(Player& player);
    void Land(Player& player);
    void SetPos(Player& player, const Vector3& pos);
    void UpdateFallTrail(Player& player);
    void StopFallTrail();
    void PlayLandEffect();

private:
    PlayerStartMovieFrame m_frame;
    Phase m_phase = Phase::Roof;
    float m_frameCount = 0.0f;
    bool m_isAttackAnim = false;
    int m_fallTrailEffect = -1;
    int m_fallTrailPlaying = -1;
    int m_landEffect = -1;
};
