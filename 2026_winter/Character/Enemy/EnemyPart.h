#pragma once
#include "../../Collider/Collider.h"
#include "../../Math/Matrix4x4.h"

//部位破壊で切り離されたパーツ//ステージのポリゴンと当たってその場で止まる、簡易な物理挙動を持つ
class EnemyPart :
    public Collider
{
public:
    EnemyPart(int modelHandle);
    virtual ~EnemyPart();

    void OnCollision(Collider& other) override;
    void ApplyPos() override;

    //切り離した瞬間の初期化//向き・大きさを含む行列、開始座標、初速度を渡す
    void Break(const MATRIX& baseMat, const Vector3& startPos, const Vector3& initialVel);

    void Update();//重力・回転の更新(EnemySwordman側から毎フレーム呼ぶ)
    void Draw();

private:
    int m_modelHandle = -1;//表示するパーツのモデル//読み込み・解放は所有者(EnemySwordman等)が行う

    MATRIX m_baseMat = {};//切り離した瞬間の向き・大きさ

    Vector3 m_vel;//自前で持つ速度//RigidBodyのvelはCollisionManagerに書き換えられるため、こちらを正として毎フレーム渡す
    Vector3 m_spinAxis;//回転軸//少しずつ向きを変えていく
    float m_spinAngle = 0.0f;//回転角度

    bool m_isLanded = false;//地面に着いたか
};
