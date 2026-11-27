#include "TrainingMenuScene.h"
#include "SceneController.h"
#include "StageSelectScene.h"
#include "SceneMain.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

namespace
{
	constexpr int kDecideWaitFrame = 14;//決定の演出(白く光って右へ抜ける)を見せる長さ(PauseSceneと同じ)
	constexpr int kResetFadeFrame = 30;//作り直す前に暗くする長さ(SceneMainのフェードと同じ)
	//長押しで繰り返す
	constexpr int kRepeatDelay = 20;
	constexpr int kRepeatInterval = 6;

	const char* const kItemNames[] =
	{
		"敵の種類", "敵の数", "敵の動き", "倒したら", "敵のHP", "敵の欠損", "自分のHP", "ゲージ",
		"トレーニング終了",
	};
	//選択肢(TrainingManager::Settings の番号の順)//項目名と重ならないように4文字まで
	const std::vector<std::vector<const char*>> kChoices =
	{
		{ "刀", "銃", "バズーカ", "混ぜる", "ボス" },
		{ "1体", "3体", "5体" },
		{ "立つだけ", "ガード", "攻撃する", "通常" },
		{ "出し直す", "そのまま" },
		{ "通常", "無限" },
		{ "通常", "必ず欠損", "欠損なし" },
		{ "無限", "通常" },
		{ "無限", "通常" },
	};
	const char* const kHelps[] =
	{
		"練習する相手。混ぜるは刀・銃・バズーカを順に出す。ボスはキングメックを広場の真ん中に1体だけ出す。",
		"同時に出す数。1体は円の真ん中、3体はこちらに開いた弧、5体は円の上に並べる。",
		"",//敵の動きは選んでいる値ごとに変える(kAiHelps)
		"出し直すなら、倒した敵を1秒後に同じ場所へ同じ種類で出す。",
		"無限にすると倒れないので、長いコンボを試せる。",
		"通常は技ごとの確率で腕が取れる。必ず欠損は攻撃が通るとすぐ取れる(確殺の練習)。ボスは対象外。",
		"無限なら敵の攻撃を受けても減らない。",
		"無限ならスキルと必殺技がいつでも使える。",
		"トレーニングをやめて、ステージセレクトに戻る。",
	};
	const char* const kAiHelps[] =
	{
		"立つだけ: その場に立って動かない。",
		"ガード: ガードし続ける。当て続けると崩れる(ガードブレイクの練習)。",
		"攻撃する: その場から動かず、間をあけてランダムな技を出す。寄ってこないので、近づいて受ける(回避の練習)。",
		"通常: ステージと同じ動き。",
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
	//値の枠は全部の選択肢でいちばん長い文字に合わせる
	std::string widest;
	for (const auto& choices : kChoices)
	{
		for (const char* choice : choices)
		{
			if (strlen(choice) > widest.size()) widest = choice;
		}
	}
	m_ui.SetValueSlot(widest);
	m_ui.SetListHeader("設定");
	m_ui.SetItems(std::vector<std::string>(std::begin(kItemNames), std::end(kItemNames)));
	ApplyToUI();
}

TrainingMenuScene::~TrainingMenuScene()
{
}

int* TrainingMenuScene::ValueOf(TrainingManager::Settings& settings, MenuItem item)
{
	switch (item)
	{
	case MenuItem::EnemyType: return &settings.enemyType;
	case MenuItem::EnemyCount: return &settings.enemyCount;
	case MenuItem::EnemyAi: return &settings.enemyAi;
	case MenuItem::Respawn: return &settings.respawn;
	case MenuItem::EnemyHp: return &settings.enemyHp;
	case MenuItem::PartBreak: return &settings.partBreak;
	case MenuItem::PlayerHp: return &settings.playerHp;
	case MenuItem::Gauge: return &settings.gauge;
	default: return nullptr;
	}
}

bool TrainingMenuScene::IsRepeat(const char* name, int& holdFrame, bool& isFirst)
{
	auto& input = Input::GetInstance();
	isFirst = input.IsTriggered(name);
	if (isFirst)
	{
		holdFrame = 0;
		return true;
	}
	if (!input.IsPressed(name))
	{
		holdFrame = 0;
		return false;
	}
	holdFrame++;
	return holdFrame >= kRepeatDelay && (holdFrame - kRepeatDelay) % kRepeatInterval == 0;
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
	int* value = ValueOf(m_settings, item);
	if (!value)return;
	const int num = ChoiceNum(item);
	m_ui.StartValueChange(m_cursor, d);
	*value = (*value + d + num) % num;
}

void TrainingMenuScene::ApplyToUI()
{
	TrainingManager::Settings applied = m_training ? m_training->GetSettings() : m_settings;
	std::vector<TrainingMenuUI::ValueRow> values;
	for (int i = 0; i < static_cast<int>(MenuItem::Num); ++i)
	{
		const MenuItem item = static_cast<MenuItem>(i);
		TrainingMenuUI::ValueRow row;
		const int* value = ValueOf(m_settings, item);
		if (value)
		{
			row.text = kChoices[i][*value];
			row.index = *value;
			row.num = ChoiceNum(item);
			row.isChanged = (*value != *ValueOf(applied, item));
		}
		values.push_back(row);
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

	//↑↓は長押しで進む。端で回るのは押した瞬間だけ(長押しは端で止まる)
	const int itemNum = static_cast<int>(MenuItem::Num);
	bool isFirst = false;
	if (IsRepeat("Up", m_holdUp, isFirst))
	{
		if (m_cursor > 0) m_cursor--;
		else if (isFirst) m_cursor = itemNum - 1;
	}
	if (IsRepeat("Down", m_holdDown, isFirst))
	{
		if (m_cursor < itemNum - 1) m_cursor++;
		else if (isFirst) m_cursor = 0;
	}
	if (IsRepeat("Left", m_holdLeft, isFirst)) ChangeValue(-1);
	if (IsRepeat("Right", m_holdRight, isFirst)) ChangeValue(1);

	//A: 設定の項目は値を進める、実行する項目は演出を見せてから実行する
	if (input.IsTriggered("A"))
	{
		const MenuItem item = static_cast<MenuItem>(m_cursor);
		if (ValueOf(m_settings, item))
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
	//変わっていたら暗くして作り直す
	if (m_training && !(m_settings == m_training->GetSettings()))
	{
		m_resetFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&TrainingMenuScene::ResetUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&TrainingMenuScene::ResetDraw);
		return;
	}
	m_controller.PopScene();
}

void TrainingMenuScene::ResetUpdate()
{
	m_resetFrame++;
	if (m_resetFrame >= kResetFadeFrame)
	{
		//下のSceneMainごと破棄して作り直す(このシーンも破棄されるので、呼んだ後は何もしない)
		const TrainingManager::Settings settings = m_settings;
		m_controller.ResetScene<SceneMain>(StageType::Training, std::nullopt, settings);
	}
}

void TrainingMenuScene::ResetDraw()
{
	//メニューは消して、止まったゲームの画面を暗くしていく
	const float rate = static_cast<float>(m_resetFrame) / static_cast<float>(kResetFadeFrame);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, 0x000000, true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TrainingMenuScene::Decide(MenuItem item)
{
	switch (item)
	{
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
