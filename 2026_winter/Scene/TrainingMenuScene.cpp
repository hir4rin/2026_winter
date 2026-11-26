#include "TrainingMenuScene.h"
#include "SceneController.h"
#include "StageSelectScene.h"
#include "Main/Input.h"
#include <iterator>
#include <string>
#include <vector>

namespace
{
	constexpr int kDecideWaitFrame = 14;//決定の演出(白く光って右へ抜ける)を見せる長さ(PauseSceneと同じ)

	const char* const kItemNames[] =
	{
		"敵の種類", "敵の数", "敵の動き", "倒したら", "敵のHP", "自分のHP", "ゲージ",
		"敵を出し直す", "スタートに戻る", "トレーニング終了",
	};
	//選択肢(TrainingManager::Settings の番号の順)//項目名と重ならないように4文字まで
	const std::vector<std::vector<const char*>> kChoices =
	{
		{ "刀", "銃", "バズーカ", "混ぜる", "ボス" },
		{ "1体", "3体", "5体" },
		{ "立つだけ", "ガード", "攻撃する", "ふつう" },
		{ "出し直す", "そのまま" },
		{ "ふつう", "無限" },
		{ "無限", "ふつう" },
		{ "無限", "ふつう" },
	};
	const char* const kHelps[] =
	{
		"練習する相手。混ぜるは刀・銃・バズーカを順に出す。ボスはキングメックを広場の真ん中に1体だけ出す。",
		"同時に出す数。1体は円の真ん中、3体はこちらに開いた弧、5体は円の上に並べる。",
		"",//敵の動きは選んでいる値ごとに変える(kAiHelps)
		"出し直すなら、倒した敵を1秒後に同じ場所へ同じ種類で出す。",
		"無限にすると倒れないので、長いコンボを試せる。",
		"無限なら敵の攻撃を受けても減らない。",
		"無限ならスキルと必殺技がいつでも使える。",
		"今の設定で、敵を全部消して出し直す。",
		"自分をスタートに戻す。",
		"トレーニングをやめて、ステージセレクトに戻る。",
	};
	const char* const kAiHelps[] =
	{
		"立つだけ: その場に立って動かない。",
		"ガード: ガードし続ける。当て続けると崩れる(ガードブレイクの練習)。",
		"攻撃する: その場から動かず、間をあけてランダムな技を出す。寄ってこないので、近づいて受ける(回避の練習)。",
		"ふつう: ステージと同じ動き。",
	};
}

TrainingMenuScene::TrainingMenuScene(SceneController& controller, std::shared_ptr<TrainingManager> training) :
	Scene(controller),
	m_training(training)
{
	static_assert(std::size(kItemNames) == static_cast<size_t>(MenuItem::Num), "項目名の数をMenuItemと合わせる");
	static_assert(std::size(kHelps) == static_cast<size_t>(MenuItem::Num), "説明の数をMenuItemと合わせる");
	m_updateFunc = static_cast<UpdateFunc_t>(&TrainingMenuScene::NormalUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&TrainingMenuScene::NormalDraw);

	if (m_training) m_settings = m_training->GetSettings();
	m_ui.Init();
	m_ui.SetTitle("トレーニング");
	m_ui.SetListHeader("設定");
	m_ui.SetItems(std::vector<std::string>(std::begin(kItemNames), std::end(kItemNames)));
	ApplyToUI();
}

TrainingMenuScene::~TrainingMenuScene()
{
}

int* TrainingMenuScene::ValueOf(MenuItem item)
{
	switch (item)
	{
	case MenuItem::EnemyType: return &m_settings.enemyType;
	case MenuItem::EnemyCount: return &m_settings.enemyCount;
	case MenuItem::EnemyAi: return &m_settings.enemyAi;
	case MenuItem::Respawn: return &m_settings.respawn;
	case MenuItem::EnemyHp: return &m_settings.enemyHp;
	case MenuItem::PlayerHp: return &m_settings.playerHp;
	case MenuItem::Gauge: return &m_settings.gauge;
	default: return nullptr;
	}
}

int TrainingMenuScene::ChoiceNum(MenuItem item)
{
	const int index = static_cast<int>(item);
	if (index < 0 || index >= static_cast<int>(kChoices.size()))return 0;
	return static_cast<int>(kChoices[index].size());
}

void TrainingMenuScene::ChangeValue(int d)
{
	const MenuItem item = static_cast<MenuItem>(m_cursor);
	int* value = ValueOf(item);
	if (!value)return;
	const int num = ChoiceNum(item);
	*value = (*value + d + num) % num;
}

void TrainingMenuScene::ApplyToUI()
{
	std::vector<std::string> values;
	for (int i = 0; i < static_cast<int>(MenuItem::Num); ++i)
	{
		const int* value = ValueOf(static_cast<MenuItem>(i));
		values.push_back(value ? kChoices[i][*value] : "");
	}
	m_ui.SetValues(values);
	m_ui.SetCursor(m_cursor);
	const MenuItem item = static_cast<MenuItem>(m_cursor);
	m_ui.SetDetail("", "", item == MenuItem::EnemyAi ? kAiHelps[m_settings.enemyAi] : kHelps[m_cursor]);
	m_ui.SetDirty(m_training && !(m_settings == m_training->GetSettings()));
}

void TrainingMenuScene::Update()
{
	m_ui.Update();
	(this->*m_updateFunc)();
}

void TrainingMenuScene::FadeInUpdate()
{
}

void TrainingMenuScene::NormalUpdate()
{
	//下のSceneMainはUpdateされないので、ここで入力を更新する
	auto& input = Input::GetInstance();
	input.Update();

	const int itemNum = static_cast<int>(MenuItem::Num);
	if (input.IsTriggered("Up")) m_cursor = (m_cursor + itemNum - 1) % itemNum;
	if (input.IsTriggered("Down")) m_cursor = (m_cursor + 1) % itemNum;
	if (input.IsTriggered("Left")) ChangeValue(-1);
	if (input.IsTriggered("Right")) ChangeValue(1);

	//A: 設定の項目は値を進める、実行する項目は演出を見せてから実行する
	if (input.IsTriggered("A"))
	{
		const MenuItem item = static_cast<MenuItem>(m_cursor);
		if (ValueOf(item))
		{
			ChangeValue(1);
		}
		else
		{
			m_decideItem = item;
			m_decideFrame = 0;
			m_ui.StartDecide();
			m_updateFunc = static_cast<UpdateFunc_t>(&TrainingMenuScene::FadeOutUpdate);
			m_drawFunc = static_cast<DrawFunc_t>(&TrainingMenuScene::FadeOutDraw);
			ApplyToUI();
			return;
		}
	}
	ApplyToUI();

	//Start・Bで閉じる
	if (input.IsTriggered("Start") || input.IsTriggered("B"))
	{
		Close();
		return;
	}
}

void TrainingMenuScene::Close()
{
	if (m_training && !(m_settings == m_training->GetSettings())) m_training->Apply(m_settings);
	m_controller.PopScene();
}

void TrainingMenuScene::Decide(MenuItem item)
{
	switch (item)
	{
	case MenuItem::RespawnNow:
		if (m_training) m_training->Apply(m_settings);
		break;
	case MenuItem::ReturnStart:
		if (m_training)
		{
			if (!(m_settings == m_training->GetSettings())) m_training->Apply(m_settings);
			m_training->ReturnPlayerToStart();
		}
		break;
	case MenuItem::Quit:
		//下のSceneMainごと破棄してステージセレクトに戻る(このシーンも破棄されるので、呼んだ後は何もしない)
		m_controller.ResetScene<StageSelectScene>();
		return;
	default:
		break;
	}
	m_controller.PopScene();
}

void TrainingMenuScene::FadeOutUpdate()
{
	//決定の演出を待つ
	m_decideFrame++;
	if (m_decideFrame >= kDecideWaitFrame)
	{
		Decide(m_decideItem);
	}
}

void TrainingMenuScene::Draw()
{
	(this->*m_drawFunc)();
}

void TrainingMenuScene::FadeInDraw()
{
}

void TrainingMenuScene::NormalDraw()
{
	//下に積まれたSceneMainはSceneController::Drawで先に描画される
	m_ui.Draw();
}

void TrainingMenuScene::FadeOutDraw()
{
	m_ui.Draw();
}
