#pragma once
#include <memory>

class Player;
class Weapon
{
public:
	Weapon(std::weak_ptr<Player> owner);
	virtual ~Weapon();

	void Update();
	void TitleUpdate();
	void Draw();
	void TitleDraw();
private:
	std::weak_ptr<Player> m_owner;//持ち主のプレイヤー
	int m_ownerHandle = -1;//持ち主のハンドル
	int m_modelHandle = -1;//剣のモデルのハンドル
	int slotIndex = 58;//装備しているスロットのインデックス

};

