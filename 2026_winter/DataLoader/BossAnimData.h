#pragma once
#include <string>
#include <unordered_map>
#include <cassert>
struct BossAnimData
{
	std::unordered_map<std::string, std::string> animNames;//アニメーションの名前を管理するためのマップ//Stateとアニメーションの名前を対応させる

	const std::string& GetAnimName(const std::string& key)const
	{
		auto it = animNames.find(key);
		assert(it != animNames.end() && "BossAnimData:指定されたキーが見つかりません");
		return it->second;
	}
};
