#include "CameraOptionScene.h"
#include "SceneController.h"
#include "Main/Input.h"
#include "Main/System.h"
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace
{
	constexpr int kDecideWaitFrame = 14;//決定の演出を見せる長さ(PauseSceneと同じ)
	//長押しで繰り返す(TrainingMenuSceneと同じ)
	constexpr int kRepeatDelay = 20;
	constexpr int kRepeatInterval = 6;

	const char* const kItemNames[] = { "カメラ感度", "左右反転", "上下反転", "戻る" };
	const char* const kOnOff[] = { "オフ", "オン" };
	const char* const kHelps[] =
	{
		"右スティックでカメラを回す速さ。5 が標準。",
		"オンにすると、右スティックの左右とカメラの回る向きが逆になる。",
		"オンにすると、右スティックの上下とカメラの回る向きが逆になる。",
		"ポーズに戻る。",
	};
}

CameraOptionScene::CameraOptionScene(SceneController& controller) :Scene(controller)
{
	static_assert(std::size(kItemNames) == static_cast<size_t>(MenuItem::Num), "項目名の数をMenuItemと合わせる");
	static_assert(std::size(kHelps) == static_cast<size_t>(MenuItem::Num), "説明の数をMenuItemと合わせる");
	m_updateFunc = static_cast<UpdateFunc_t>(&CameraOptionScene::NormalUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&CameraOptionScene::NormalDraw);

	m_ui.Init();
	m_ui.SetTitle("カメラ設定");
	m_ui.SetListHeader("設定");
	m_ui.SetItems(std::vector<std::string>(std::begin(kItemNames), std::end(kItemNames)));
	ApplyToUI();
}

CameraOptionScene::~CameraOptionScene()
{
}

bool CameraOptionScene::IsRepeat(const char* name, int& holdFrame, bool& isFirst)
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

void CameraOptionScene::ChangeValue(int d)
{
	auto& option = System::GetInstance().GetCameraOption();
	switch (static_cast<MenuItem>(m_cursor))
	{
	case MenuItem::Sensitivity:
	{
		//感度は端で止める
		const int next = std::clamp(option.sensitivity + d, CameraOption::kSensitivityMin, CameraOption::kSensitivityMax);
		if (next == option.sensitivity)return;
		m_ui.StartValueChange(m_cursor, d);
		option.sensitivity = next;
		break;
	}
	case MenuItem::InvertX:
		m_ui.StartValueChange(m_cursor, d);
		option.isInvertX = !option.isInvertX;
		break;
	case MenuItem::InvertY:
		m_ui.StartValueChange(m_cursor, d);
		option.isInvertY = !option.isInvertY;
		break;
	default:
		break;
	}
}

void CameraOptionScene::ApplyToUI()
{
	const auto& option = System::GetInstance().GetCameraOption();
	std::vector<TrainingMenuUI::ValueRow> values(static_cast<size_t>(MenuItem::Num));

	auto& sensitivity = values[static_cast<int>(MenuItem::Sensitivity)];
	sensitivity.text = std::to_string(option.sensitivity);
	sensitivity.index = option.sensitivity - CameraOption::kSensitivityMin;
	sensitivity.num = CameraOption::kSensitivityMax - CameraOption::kSensitivityMin + 1;

	const auto setOnOff = [](TrainingMenuUI::ValueRow& row, bool isOn)
		{
			row.text = kOnOff[isOn ? 1 : 0];
			row.index = isOn ? 1 : 0;
			row.num = 2;
		};
	setOnOff(values[static_cast<int>(MenuItem::InvertX)], option.isInvertX);
	setOnOff(values[static_cast<int>(MenuItem::InvertY)], option.isInvertY);

	m_ui.SetValueSlot(kOnOff[0]);//「10」より「オフ」のほうが長い
	m_ui.SetValues(values);
	m_ui.SetCursor(m_cursor);
	m_ui.SetDetail("", "", kHelps[m_cursor]);
}

void CameraOptionScene::Update()
{
	m_ui.Update();
	(this->*m_updateFunc)();
}

void CameraOptionScene::FadeInUpdate()
{
}

void CameraOptionScene::NormalUpdate()
{
	//下のシーンはUpdateされないので、ここで入力を更新する
	auto& input = Input::GetInstance();
	input.Update();

	//↑↓は長押しで進む。端で回るのは押した瞬間だけ
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

	//A: 反転は切り替え、戻るは演出を見せてから戻る
	if (input.IsTriggered("A"))
	{
		const MenuItem item = static_cast<MenuItem>(m_cursor);
		if (item == MenuItem::Back)
		{
			m_decideFrame = 0;
			m_ui.StartDecide();
			m_updateFunc = static_cast<UpdateFunc_t>(&CameraOptionScene::FadeOutUpdate);
			m_drawFunc = static_cast<DrawFunc_t>(&CameraOptionScene::FadeOutDraw);
			ApplyToUI();
			return;
		}
		else if (item != MenuItem::Sensitivity)
		{
			ChangeValue(1);
		}
	}
	ApplyToUI();

	//Start・Bでポーズに戻る
	if (input.IsTriggered("Start") || input.IsTriggered("B"))
	{
		m_controller.PopScene();
		return;
	}
}

void CameraOptionScene::FadeOutUpdate()
{
	//決定の演出を待つ
	m_decideFrame++;
	if (m_decideFrame >= kDecideWaitFrame)
	{
		m_controller.PopScene();
	}
}

void CameraOptionScene::Draw()
{
	(this->*m_drawFunc)();
}

void CameraOptionScene::FadeInDraw()
{
}

void CameraOptionScene::NormalDraw()
{
	m_ui.Draw();
}

void CameraOptionScene::FadeOutDraw()
{
	m_ui.Draw();
}
