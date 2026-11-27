#pragma once
#include "PauseUI.h"

//トレーニングの設定メニュー(ポーズと同じ形。項目の右に今の値、右上に説明)
class TrainingMenuUI :
	public PauseUI
{
public:
	//項目ごとの値
	struct ValueRow
	{
		std::string text;//空なら値の無い項目=実行する項目
		int index = 0;//今の選択肢の番号
		int num = 0;//選択肢の数
		bool isChanged = false;//まだ反映していない
	};

	void Update() override;

	void SetValues(const std::vector<ValueRow>& values) { m_values = values; }
	//値の枠の幅(全部の項目で共通。全部の選択肢でいちばん長い文字を渡す)
	void SetValueSlot(const std::string& widest) { m_slotWidest = widest; }
	//変えた設定がまだ反映されていないか(閉じたときに反映する)
	void SetDirty(bool isDirty) { m_isDirty = isDirty; }
	//値が変わった演出(dir: 1で右へ、-1で左へ)。SetValues より前に呼ぶ
	void StartValueChange(int row, int dir);

private:
	void DrawDescription() override;//項目の右に値
	void DrawRecord() override;//右上に説明
	void DrawPauseGuide() override;
	void DrawValue(int row, float y);

	std::vector<ValueRow> m_values;
	std::string m_slotWidest;
	bool m_isDirty = false;
	int m_changeRow = -1;
	int m_changeDir = 1;
	int m_changeFrame = 0;
	std::string m_changeOldText;
};
