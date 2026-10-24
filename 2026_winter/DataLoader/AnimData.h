#pragma once
#include <string>
#include <unordered_map>
#include <cassert>

struct AnimData
{
	std::unordered_map<std::string, std::string> animNames;//アニメーションの名前を管理するためのマップ//Stateとアニメーションの名前を対応させる
	//アニメーションの再生速度(CSVの3列目)//キーはanimNamesと同じkey//空欄の行は入れない(1.0扱い)
	std::unordered_map<std::string, float> animSpeeds;

	const std::string& GetAnimName(const std::string& key)const
	{
		auto it = animNames.find(key);
		assert(it != animNames.end() && "AnimData:指定されたキーが見つかりません");
		return it->second;
	}
};
