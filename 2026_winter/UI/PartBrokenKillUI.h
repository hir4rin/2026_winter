#pragma once
#include <memory>
#include "UIBase.h"

class Player;
class EnemyBase;

//確殺対象の体の中心に出すUI
class PartBrokenKillUI : public UIBase
{
public:
	PartBrokenKillUI(std::weak_ptr<Player> player);
	~PartBrokenKillUI() override;

	void Init()override;
	void Update(Input& input)override;
	void Draw()override;

private:
	//今Yで確殺できる敵を返す(いなければnullptr)
	std::shared_ptr<EnemyBase> FindTarget() const;
	//UIを消して最初の状態に戻す
	void Hide();

	float GetBladeOpenRate(float delay) const;//刃の伸び具合(0〜1)
	float GetExitAlpha() const;//リアクションの全体の透明度(0〜1)

	//x,yは×の中心(実解像度)
	void DrawBlades(float x, float y, float scale);
	void DrawBladePair(float x, float y, float length, float thick);
	void DrawBlade(float x, float y, float angle, float openRate, float length, float thick);
	void DrawCenter(float x, float y, float scale);

	std::weak_ptr<Player> m_player;
	std::weak_ptr<EnemyBase> m_target;

	int m_frame = 0;//出てからのフレーム

	bool m_isExiting = false;//確殺のリアクション中
	int m_exitFrame = 0;//リアクションが始まってからのフレーム
	bool m_isOnScreen = false;//前のDrawで画面に出せた
	float m_drawX = 0.0f;//×の中心
	float m_drawY = 0.0f;

	int m_bladeHandle = -1;//刃
	int m_buttonHandle = -1;//Yボタン
};
