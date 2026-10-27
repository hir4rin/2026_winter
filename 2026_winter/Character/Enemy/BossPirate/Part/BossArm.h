#pragma once
#include "Character/Enemy/EnemyBase.h"
#include <memory>

//ボスの背中から生える腕
//EnemyBaseを継承して、被弾・打ち上げなどの敵のステートを使えるようにする
//EnemyManagerが持ち、ボスはweak_ptrで持つ
//モデルはボスから切り出したもので、フレームの並びがボスと同じ//アニメはボスのモデルのものを当てる
class BossArm :
    public EnemyBase
{
public:
    enum class Side
    {
        Left,
        Right,
    };

    BossArm(std::weak_ptr<Player> player, std::weak_ptr<EnemyBase> boss, int bossModelHandle, Side side);
    virtual ~BossArm();

    void Init()override;
    void Update()override;
    void Draw()override;
    void DebugDraw()override;

    bool IsTargetable()const override { return false; }//今は当たり判定が無いので対象にしない

    //ImGuiで付け根・ずらし・アニメを調整する(Begin〜Endの中で呼ぶ)
    void DrawEditor();
    //もう片方の腕の値を左右反転して入れる
    void SetMirrorOffset(const BossArm& other);

    //ボスから頼まれたアニメを流す//keyはBossArmAnim.csvのkey
    void PlayAnim(const std::string& key, bool isLoop, float timeScale = 1.0f);
    void ReturnToIdle();//普段のアニメに戻す

    void Detach();//ボスから切り離す
    void RequestReturn();//切り離した腕を戻す
    bool IsDetached()const { return m_isDetached; }

protected:
    //腕は歩かないので、Idleのままにする
    std::shared_ptr<EnemyStateBase> NextAfterIdle()override;
    std::shared_ptr<EnemyStateBase> NextAfterRecover()override;

private:
    MATRIX CalcSpineMatrix()const;//腕のモデルの背骨(spine_03)のワールド行列//ボスの付け根のフレーム＋ずらし

    MATRIX CalcDetachedSpineMatrix()const;//切り離した腕の背骨//覚えた行列を付け根中心に回してずらす
    MATRIX CalcDrawSpineMatrix()const;//描く背骨//付いている・切り離し中・戻り中で変わる
    MATRIX LerpMatrix(const MATRIX& a, const MATRIX& b, float t)const;//向き・大きさ・位置を補間する

private:
    std::weak_ptr<EnemyBase> m_boss;//持ち主のボス
    int m_bossModelHandle = -1;//ボスのモデル//付け根のフレーム・アニメを使う
    Side m_side = Side::Left;

    int m_rootFrame = -1;//付け根にするボスのフレーム(背骨)
    int m_armSpineFrame = -1;//腕のモデルの背骨//ここを上書きすると、鎖骨から先がついてくる

    //付け根からのずれ//向きはボスの体の向き//付け根を中心に回す・大きくする
    Vector3 m_offsetPos;
    Vector3 m_offsetRotDeg;
    float m_scale = 1.0f;

    //デバッグ用//ImGuiで選んだアニメ
    int m_debugAnimIndex = 0;
    bool m_isDebugAnimLoop = true;

    //腕切り離し、引き戻しよう
    friend class BossArmDetached;
    friend class BossArmReturn;

    //切り離し
    bool m_isDetached = false;
    MATRIX m_detachSpineMatrix = MGetIdent();//切り離した瞬間の背骨
    Vector3 m_detachMove;//切り離した位置からの移動
    float m_detachSpinDeg = 0.0f;//付け根を中心にした回転(Y軸)
    float m_returnRate = 0.0f;//戻り具合(0:切り離した形 1:ボスに付いた形)
};
