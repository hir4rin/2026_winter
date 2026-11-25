#pragma once
#include "DxLib.h"
#include <memory>
#include <optional>
#include "Scene/Scene.h"
#include "Battle/RespawnInfo.h"
#include "Stage/StageInfo.h"
#include "Camera/CameraState/PlayerStartMovieCamera/PlayerStartMovieFrame.h"
#include "Render/StageColorGrade.h"
#include "Render/TempTopLight.h"//[TempTopLight]

class Player;
class EnemySwordman;
class CameraManager;
class EnemyManager;
class PhaseManager;
class CheckpointManager;
class BattleManager;
class Camera;
class Stage;
class SkyBox;
class WallZoneEditor;
class PhaseAreaEditor;
class CheckpointEditor;
class UIManager;
class TunnelEnding;
class TutorialFinale;

class SceneMain : public Scene
{
public:
	//respawnがあれば、死ぬ前の続き(チェックポイント・フェーズ・記録)から始める
	SceneMain(SceneController& controller, StageType stageType, std::optional<RespawnInfo> respawn = std::nullopt);
	~SceneMain();

	void Update() override;
	void Draw() override;
private:
	void Init();

	//状態ごとの更新(m_updateFuncで切り替える)
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	//状態ごとの描画(m_drawFuncで切り替える)
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

	void DrawGrid();
	//フェード用の黒い板を描画する
	void DrawFade();
	//デバッグ用//プレイヤーの正面にスキルエフェクトを再生する
	void PlayDebugSkillEffect(int effectHandle, float forwardDistance);

	//チュートリアルを開いてよいか(演出の最中は待つ)
	bool CanOpenTutorial()const;

	//エンディングを始める(トンネルが無いステージはすぐリザルトへ)
	void StartEnding();

	//プレイヤーの登場演出を始める(今の位置に着地する)//Roof:ステージ1 Fall:チュートリアル
	void StartPlayerStartMovie(PlayerStartMovieFrame::Kind kind);

	//死んだときに引き継ぐものをまとめる
	RespawnInfo MakeRespawnInfo()const;

private:
	int m_frameCount;
	int m_fadeFrame;//フェードの経過フレーム

	int m_lightHandle = -1;

	StageType m_stageType;//遊ぶステージ(StageSelectSceneで選んだもの)
	std::optional<RespawnInfo> m_respawn;//リスポーンで作り直したときの引き継ぎ
	std::shared_ptr<Player> m_player;
	std::shared_ptr<EnemyManager> m_enemyManager;
	std::shared_ptr<PhaseManager> m_phaseManager;//フェーズ進行
	std::shared_ptr<CheckpointManager> m_checkpointManager;//チェックポイント(踏んだら起動・最後に踏んだ物を覚える)
	std::shared_ptr<CameraManager> m_cameraManager;
	std::shared_ptr<BattleManager> m_battleManager;
	std::unique_ptr<Camera> m_camera;//Player::Updateに渡すだけ(実際のカメラはCameraManagerが制御する)
	std::shared_ptr<Stage> m_stage;
	std::unique_ptr<SkyBox> m_skyBox;//空(空の画像があるステージだけ)
	std::unique_ptr<WallZoneEditor> m_wallZoneEditor;//ステージ編集モードで壁ゾーンを設置・編集するImGuiウィンドウ
	std::unique_ptr<PhaseAreaEditor> m_phaseAreaEditor;//ステージ編集モードでフェーズ進行のエリアを編集するImGuiウィンドウ
	std::unique_ptr<CheckpointEditor> m_checkpointEditor;//ステージ編集モードでチェックポイントを編集するImGuiウィンドウ
	std::unique_ptr<UIManager> m_uiManager;//このシーンで表示するUIをまとめて管理する
	StageColorGrade m_stageColorGrade;//血殺中に背景とステージの色を変える
	TempTopLight m_tempTopLight;//[TempTopLight]暗くて見づらいので真上から照らす(一時的)
	std::unique_ptr<TunnelEnding> m_ending;//列車ステージのエンディング(始まったら作る)
	std::unique_ptr<TutorialFinale> m_tutorialFinale;//チュートリアルの最後(長い縦穴を自動で登ってリザルトへ)

	bool m_requestScreenshot = false;//次のDrawの最後でスクリーンショットを保存するか
	bool m_wasF4Pressed = false;//前フレームにF4キーが押されていたか(ボスの手前へ飛ぶ)
	bool m_wasF5Pressed = false;//前フレームにF5キーが押されていたか(スキルエフェクト確認用)
	bool m_wasF6Pressed = false;//前フレームにF6キーが押されていたか(スキルエフェクト確認用)
	bool m_wasF9Pressed = false;//前フレームにF9キーが押されていたか(地上弱スキルのエフェクト確認用)
	int m_debugGleamStep = 0;//F9で次に出す地上弱スキルの段(0〜2)
	bool m_wasF7Pressed = false;//前フレームにF7キーが押されていたか(エンディング確認用)
};
