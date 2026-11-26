#pragma once

enum class  AttackType
{
	None = -1,
	lightAttack = 0,
	heavyAttack = 1,
	SkillHeavyAttack = 2,//地上強スキル(LB+Y)・空中スキル
	UltAttack = 3,
	GuardBreakAttack = 4,
	ImpactAttack = 5,//着地の衝撃
	ImpactSkillAttack = 6,//スキルの着地の衝撃
	SkillLightAttack = 7,//地上弱スキル(LB+X)
};