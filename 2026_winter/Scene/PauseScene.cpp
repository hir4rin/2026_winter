#include "PauseScene.h"
#include "SceneController.h"
#include "DxLib.h"
#include "Main/Input.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "StageSelectScene.h"
#include <iterator>

namespace
{
	constexpr int kDecideWaitFrame = 14;//決定の演出(白く光って右へ抜ける)を見せる長さ
}

bool PauseScene::s_isDebugItemOpen = false;

PauseScene::PauseScene(SceneController& controller, const PauseUI::Record& record) :Scene(controller)
{
	m_updateFunc = static_cast<UpdateFunc_t>(&PauseScene::NormalUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&PauseScene::NormalDraw);

	m_ui.Init();
	m_ui.SetTitle("ポーズ");
	m_ui.SetListHeader("メニュー");
	m_ui.SetRecord(record);
	RebuildItems();
}

PauseScene::~PauseScene()
{
}

void PauseScene::RebuildItems()
{
	const char* const names[] = { "ゲームに戻る", "フォトモード", "ステージ編集", "ステージセレクトへ" };
	static_assert(std::size(names) == static_cast<size_t>(MenuItem::Num), "項目名の数をMenuItemと合わせる");

	const MenuItem current = m_items.empty() ? MenuItem::ReturnGame : m_items[m_cursor];
	m_items.clear();
	std::vector<std::string> itemNames;
	for (int i = 0; i < static_cast<int>(MenuItem::Num); ++i)
	{
		const MenuItem item = static_cast<MenuItem>(i);
		const bool isDebugItem = (item == MenuItem::PhotoMode || item == MenuItem::StageEdit);
		if (isDebugItem && !s_isDebugItemOpen) continue;
		if (item == current) m_cursor = static_cast<int>(m_items.size());
		m_items.push_back(item);
		itemNames.push_back(names[i]);
	}
	m_ui.SetItems(itemNames);
	ApplyCursor();
}

void PauseScene::ApplyCursor()
{
	const char* const texts[] =
	{
		"任務に戻る。Start・B ボタンでもすぐに戻れる。",
		"カメラを自由に動かして撮影する。(開発用・カメラデバッグ)",
		"壁キック・壁走りのゾーンを置き直す。(開発用)",
		"任務を中断してステージセレクトに戻る。ここまでの進み具合は残らない。",
	};
	static_assert(std::size(texts) == static_cast<size_t>(MenuItem::Num), "説明の数をMenuItemと合わせる");
	m_ui.SetCursor(m_cursor);
	m_ui.SetDetail("", "", texts[static_cast<int>(m_items[m_cursor])]);
}

void PauseScene::Update()
{
	m_ui.Update();
	(this->*m_updateFunc)();
}

void PauseScene::FadeInUpdate()
{
}

void PauseScene::NormalUpdate()
{
	//下のSceneMainはUpdateされないので、ここで入力を更新する
	auto& input = Input::GetInstance();
	input.Update();

	//裏コマンド: L3+R3の同時押しで開発用の項目を出す
	const bool isDebugCommand = input.IsPressed("L3") && input.IsPressed("R3") &&
		(input.IsTriggered("L3") || input.IsTriggered("R3"));
	if (isDebugCommand && !s_isDebugItemOpen)
	{
		s_isDebugItemOpen = true;
		RebuildItems();
	}

	const int itemNum = static_cast<int>(m_items.size());

	//上下で項目を選ぶ(端まで行ったら反対側に回る)
	if (input.IsTriggered("Up"))
	{
		m_cursor = (m_cursor + itemNum - 1) % itemNum;
	}
	if (input.IsTriggered("Down"))
	{
		m_cursor = (m_cursor + 1) % itemNum;
	}
	ApplyCursor();

	//Aで決定(演出を見せてから実行する)
	if (input.IsTriggered("A"))
	{
		m_decideItem = m_items[m_cursor];
		m_decideFrame = 0;
		m_ui.StartDecide();
		m_updateFunc = static_cast<UpdateFunc_t>(&PauseScene::FadeOutUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&PauseScene::FadeOutDraw);
		return;
	}

	//Start・Bはいつでもゲームに戻る
	if (input.IsTriggered("Start") || input.IsTriggered("B"))
	{
		Decide(MenuItem::ReturnGame);
		return;
	}
}

void PauseScene::Decide(MenuItem item)
{
	auto battleMgr = System::GetInstance().GetBattleMgr();

	switch (item)
	{
	case MenuItem::ReturnGame:
		//フォトモード・ステージ編集を解除してSceneMainに戻る
		battleMgr->SetPhotoMode(false);
		battleMgr->SetStageEditMode(false);
		break;
	case MenuItem::PhotoMode:
		battleMgr->SetPhotoMode(true);
		battleMgr->SetStageEditMode(false);
		break;
	case MenuItem::StageEdit:
		battleMgr->SetPhotoMode(false);
		battleMgr->SetStageEditMode(true);
		break;
	case MenuItem::StageSelect:
		battleMgr->SetPhotoMode(false);
		battleMgr->SetStageEditMode(false);
		//下のSceneMainごと破棄してステージセレクトに戻る(このシーンも破棄されるので、呼んだ後は何もしない)
		m_controller.ResetScene<StageSelectScene>();
		return;
	default:
		return;
	}
	m_controller.PopScene();
}

void PauseScene::FadeOutUpdate()
{
	//決定の演出を待つ
	m_decideFrame++;
	if (m_decideFrame >= kDecideWaitFrame)
	{
		Decide(m_decideItem);
	}
}

void PauseScene::Draw()
{
	(this->*m_drawFunc)();
}

void PauseScene::FadeInDraw()
{
}

void PauseScene::NormalDraw()
{
	//下に積まれたSceneMainはSceneController::Drawで先に描画される
	m_ui.Draw();
}

void PauseScene::FadeOutDraw()
{
	m_ui.Draw();
}
