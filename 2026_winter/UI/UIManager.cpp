#include "UIManager.h"
#include "DxLib.h"
#include "KessatsuUI.h"
#include "PlayerHUD.h"
#include "FinisherUI.h"
#include "AssasinTargetUI.h"
#include "PartBrokenKillUI.h"
#include "GuardBreakChanceUI.h"
#include "ScreenBloodUI.h"

void UIManager::TitleInit()
{
	//前の場面のUIが残らないようにする
	Clear();

	//タイトルで使うUIをここで生成して登録する
	//例: Add(std::make_shared<TitleLogo>());

	//登録したUIのハンドルを生成する
	InitAll();
}

void UIManager::InGameInit(std::weak_ptr<CameraManager> cameraManager, std::weak_ptr<Player> player)
{
	//前の場面のUIが残らないようにする
	Clear();

	//インゲームで使うUIをここで生成して登録する
	//必殺技中の「血殺」の文字
	Add(std::make_shared<KessatsuUI>());
	//画面に付く血(HUDより先に描く)
	Add(std::make_shared<ScreenBloodUI>());
	//HP・スキル・必殺技ゲージ
	Add(std::make_shared<PlayerHUD>(player));
	//暗殺対象の頭の上のUI
	Add(std::make_shared<AssasinTargetUI>(player));
	//確殺対象の体の中心のUI
	Add(std::make_shared<PartBrokenKillUI>(player));
	//ガードブレイクのチャンス中のUI
	Add(std::make_shared<GuardBreakChanceUI>(player));
	//トドメのムービーの黒帯・フラッシュ
	Add(std::make_shared<FinisherUI>());

	//登録したUIのハンドルを生成する
	InitAll();
}

void UIManager::Add(std::shared_ptr<UIBase> ui)
{
	//UIを登録する
	//shared_ptrで所有権を共有する(呼び出し側でも参照を保持できる)
	m_uiList.push_back(std::move(ui));
}

void UIManager::Clear()
{
	m_uiList.clear();
}

void UIManager::InitAll()
{
	//登録されたUIのInitをまとめて呼ぶ
	for (auto& ui : m_uiList)
	{
		ui->Init();
	}
}

void UIManager::Update()
{
	//登録されたUIのUpdateをまとめて呼ぶ
	for (auto& ui : m_uiList)
	{
		ui->Update();
	}
}
void UIManager::DrawBack()
{
	DrawLayer(UILayer::Back);
}

void UIManager::Draw()
{
	DrawLayer(UILayer::Front);
}

void UIManager::DrawLayer(UILayer layer)
{
	//登録されたUIのうち、指定したレイヤーのものだけDrawをまとめて呼ぶ
	for (auto& ui : m_uiList)
	{
		if (ui->GetLayer() != layer) continue;
		ui->Draw();
	}
}
