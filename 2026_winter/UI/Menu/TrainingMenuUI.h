#pragma once
#include "PauseUI.h"

//トレーニングの設定メニュー(ポーズと同じ形。項目の右に今の値、右上に説明)
class TrainingMenuUI :
	public PauseUI
{
public:
	//項目ごとの今の値(空なら値の無い項目=実行する項目)
	void SetValues(const std::vector<std::string>& values) { m_values = values; }
	//変えた設定がまだ反映されていないか(閉じたときに反映する)
	void SetDirty(bool isDirty) { m_isDirty = isDirty; }

private:
	void DrawDescription() override;//項目の右に値
	void DrawRecord() override;//右上に説明
	void DrawPauseGuide() override;

	std::vector<std::string> m_values;
	bool m_isDirty = false;
};
