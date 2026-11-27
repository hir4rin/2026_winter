#pragma once
#include "Scene.h"
#include "UI/Menu/TrainingMenuUI.h"
#include "Battle/TrainingManager.h"
#include <memory>

/// <summary>
/// トレーニングの設定メニュー(トレーニングではポーズの代わりに Start で積む)
/// 開いている間はゲームを止め、設定を変えて閉じたら暗くしてSceneMainを作り直す
/// </summary>
class TrainingMenuScene :
	public Scene
{
public:
	TrainingMenuScene(SceneController& controller, std::shared_ptr<TrainingManager> training);
	virtual ~TrainingMenuScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

private:
	//メニューの項目(並びは画面の順)
	enum class MenuItem
	{
		EnemyType,//敵の種類
		EnemyCount,//敵の数
		EnemyAi,//敵の動き
		Respawn,//倒したら
		EnemyHp,//敵のHP
		PartBreak,//部位破壊
		PlayerHp,//自分のHP
		Gauge,//スキル・必殺技
		Quit,//トレーニングをやめる
		Num
	};

	std::shared_ptr<TrainingManager> m_training;
	TrainingManager::Settings m_settings;//メニューで選んでいる値(閉じたときに反映する)
	int m_cursor = 0;
	TrainingMenuUI m_ui;
	MenuItem m_decideItem = MenuItem::Quit;//決定の演出が終わったら実行する項目
	int m_decideFrame = 0;
	int m_resetFrame = 0;//作り直す前に暗くしているフレーム
	//長押しで進める(↑↓←→の押している長さ)
	int m_holdUp = 0, m_holdDown = 0, m_holdLeft = 0, m_holdRight = 0;

	//押した瞬間か、長押しで繰り返すフレームなら true(isFirst: 押した瞬間か)
	static bool IsRepeat(const char* name, int& holdFrame, bool& isFirst);
	//設定の項目の値の場所(実行する項目なら nullptr)
	static int* ValueOf(TrainingManager::Settings& settings, MenuItem item);
	//設定の項目の選択肢の数
	static int ChoiceNum(MenuItem item);
	//値を d だけ回す
	void ChangeValue(int d);
	//値の文字・説明をUIに渡す
	void ApplyToUI();
	//メニューを閉じる(変えた設定があれば暗くして作り直す)
	void Close();
	//暗くしてから新しい設定でSceneMainを作り直す
	//SceneMainの中から作り直すと、古いシーンの後片付けが新しいシーンのInitより後に走るので、ここから作り直す
	void ResetUpdate();
	void ResetDraw();
	//実行する項目を実行する
	void Decide(MenuItem item);
};
