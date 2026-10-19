#pragma once

enum class  AttackType
{
	None = -1,
	lightAttack = 0,
	heavyAttack = 1,
	SkillAttack = 2,
	UltAttack = 3,
	GuardBreakAttack = 4,
	ImpactAttack = 5,//着地の衝撃
	ImpactSkillAttack = 6,//スキルの着地の衝撃
};