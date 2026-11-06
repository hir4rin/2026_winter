#pragma once
#include <memory>
#include "UIBase.h"

class Player;
class EnemyBase;

//暗殺対象の頭の上に出すUI
class AssasinTargetUI : public UIBase
{
public:
	AssasinTargetUI(std::weak_ptr<Player> player);
	~AssasinTargetUI() override;

	void Init()override;
	void Update(Input& input)override;
	void Draw()override;

private:
	//今Yで暗殺できる敵を返す(いなければnullptr)
	std::shared_ptr<EnemyBase> FindTarget() const;
	//UIを消して最初の状態に戻す
	void Hide();

	float GetIconPop() const;//アイコンの大きさの倍率
	float GetExitAlpha() const;//リアクションの全体の透明度(0〜1)

	//x,yは丸の中心(実解像度)
	void DrawBand(float x, float y, float scale);
	void DrawIcon(float x, float y, float scale);
	void DrawLabel(float x, float y, float scale);
	void DrawFlash(float x, float y, float scale);

	std::weak_ptr<Player> m_player;
	std::weak_ptr<EnemyBase> m_target;

	int m_frame = 0;//出てからのフレーム

	bool m_isExiting = false;//暗殺のリアクション中
	int m_exitFrame = 0;//リアクションが始まってからのフレーム
	bool m_isOnScreen = false;//前のDrawで画面に出せた
	float m_drawX = 0.0f;//丸の中心
	float m_drawY = 0.0f;

	int m_iconHandle = -1;//丸いアイコン
	int m_buttonHandle = -1;//Yボタン
	int m_bandHandle = -1;//赤い帯
	int m_fontHandle = -1;//「暗殺」の文字
};
