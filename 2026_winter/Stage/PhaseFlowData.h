#pragma once
#include <string>

//進む条件
enum class PhaseConditionType : int
{
	Start,//すぐ(from=0用)
	AllDead,//全滅
	KillCount,//N体倒した
	Timer,//Nフレーム経った
	EnterArea,//エリアに入った
};

//進んだときのイベント
enum class PhaseEvent : int
{
	None,
	BossAppear,
	GameClear,
};

//遷移表の1行
struct PhaseTransition
{
	int from = 0;
	PhaseConditionType condition = PhaseConditionType::AllDead;
	std::string param;//AllDeadのグループ名など
	int intParam = 0;//Timer/KillCount用
	int areaIndex = -1;//EnterArea用
	int to = -1;//-1なら終わり
	PhaseEvent event = PhaseEvent::None;
};
