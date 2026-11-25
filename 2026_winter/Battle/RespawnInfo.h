#pragma once
#include "Battle/ResultRecorder.h"

//死んだあと SceneMain を作り直すときに引き継ぐもの
struct RespawnInfo
{
	int checkpointIndex = -1;//-1なら最初の場所から
	int killCount = 0;//戻ったときの撃破数
	ResultRecorder recorder;//タイム・技・死んだ回数などは続きから
};
