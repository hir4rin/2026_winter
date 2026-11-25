#include "UIManager.h"
#include "DxLib.h"
#include "UI/Effect/KessatsuUI.h"
#include "UI/HUD/PlayerHUD.h"
#include "UI/Effect/FinisherUI.h"
#include "UI/Effect/EndingUI.h"
#include "UI/Target/AssasinTargetUI.h"
#include "UI/Target/PartBrokenKillUI.h"
#include "UI/Target/GuardBreakChanceUI.h"
#include "UI/Target/LockOnReticleUI.h"
#include "UI/Target/LookNextUI.h"
#include "UI/Effect/LookNextFxUI.h"
#include "UI/Effect/ScreenBloodUI.h"
#include "UI/Effect/LowHpInkUI.h"
#include "UI/Title/TitleBackUI.h"
#include "UI/HUD/BossHpUI.h"

void UIManager::TitleInit()
{
	//前の場面のUIが残らないようにする
	Clear();

	//背景(3Dより先)
	//シーンが合図を出すUIはシーンで作って Add し、最後に InitAll を呼ぶ
	Add(std::make_shared<TitleBackUI>());
}

void UIManager::InGameInit(std::weak_ptr<CameraManager> cameraManager, std::weak_ptr<Player> player, std::weak_ptr<EnemyManager> enemyManager)
{
	//前の場面のUIが残らないようにする
	Clear();

	//インゲームで使うUIをここで生成して登録する
	//必殺技中の「血殺」の文字
	Add(std::make_shared<KessatsuUI>());
	//RTで次の目的地を見ている間の画面効果(画面を写すので、ほかのUIより先に描く)
	Add(std::make_shared<LookNextFxUI>(cameraManager));
	//HPが低いときの墨(ほかのUIより先に描く)
	Add(std::make_shared<LowHpInkUI>(player));
	//画面に付く血(HUDより先に描く)
	Add(std::make_shared<ScreenBloodUI>());
	//HP・スキル・必殺技ゲージ
	Add(std::make_shared<PlayerHUD>(player));
	//ロックオンのレティクル
	Add(std::make_shared<LockOnReticleUI>(player));
	//RTで次の目的地を見ている間の注視点の紋
	Add(std::make_shared<LookNextUI>(cameraManager));
	//暗殺対象の頭の上のUI
	Add(std::make_shared<AssasinTargetUI>(player));
	//確殺対象の体の中心のUI
	Add(std::make_shared<PartBrokenKillUI>(player));
	//ガードブレイクのチャンス中のUI
	Add(std::make_shared<GuardBreakChanceUI>(player));
	//トドメのムービーの黒帯・フラッシュ
	Add(std::make_shared<FinisherUI>());
	//ボスのHPバー(撃破の演出がトドメの黒帯の上に出るように後に描く)
	Add(std::make_shared<BossHpUI>(enemyManager));
	//エンディングのクレジット・ロゴ
	Add(std::make_shared<EndingUI>());

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
