#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
	//マスター音量の初期値(255が最大)
	constexpr int kDebugBgmMasterVolumeDefault = 80;
	constexpr int kDebugSEMasterVolumeDefault = 100;
	constexpr int kBgmMasterVolumeDefault = 90;
	constexpr int kSEMasterVolumeDefault = 100;
}

struct SeSlot
{
	int handle;
};

struct SeFade
{
	int handle = -1;//フェード対象の再生ハンドル
	int startVolume = 0;//フェード開始時点の音量
	int elapsedFrame = 0;//フェード開始からの経過フレーム
	int totalFrame = 1;//フェードにかけるフレーム数
};

class SoundManager
{
public:

	void Init();
	void Update();

	void PlaySE(const std::string& name);
	void PlayBgm(const std::string& name);
	void PlaySELoop(const std::string& name);

	void StopBgm();
	void StopSE(const std::string& name);
	void FadeOutSE(const std::string& name, int frames);//指定フレームかけて音量を0にしながら止める
	void FadeOutBgm(int frames);//再生中のBGMを指定フレームかけて音量を0にしながら止める(呼んだ直後からPlayBgmで別のBGMを重ねて再生できる)
	//セット
	void SetBgmVolume(int volume);//0～255
	void SetSEVolume(int volume);//0～255

	//音量設定用
	int GetBgmVolume() { return m_masterVolume; }
	int GetSEVolume() { return m_masterVolume2; }

	//ミュート(全音量を0にする)の切り替え。F3キーに割り当てる想定
	void ToggleMute();
	bool IsMuted()const { return m_isMuted; }

private:
	//ミュート中なら0を、そうでなければvolumeそのものを返す。ChangeVolumeSoundMemに渡す直前に必ずこれを通す
	int ApplyMute(int volume)const { return m_isMuted ? 0 : volume; }


	std::unordered_map<std::string, int> m_sounds;
	std::unordered_map<std::string, int>m_soundVolumes;//名前と音量を紐づける
	std::unordered_map<std::string, int>m_seVolumes;//名前と音量を紐づける
	//内部的なもの
#ifdef _DEBUG
	int m_masterVolume = kDebugBgmMasterVolumeDefault;//255が最大
	int m_masterVolume2 = kDebugSEMasterVolumeDefault;//255が最大
#else
	int m_masterVolume = kBgmMasterVolumeDefault;//255が最大
	int m_masterVolume2 = kSEMasterVolumeDefault;//255が最大
#endif
	//見た目用
	int m_currentBgm = -1;

	bool m_isMuted = false;//ミュート中かどうか(F3キーでトグル)
	
	//3重再生用スロット
	std::vector<SeSlot> m_seSlots;

	//フェードアウト中のSEの一覧
	std::vector<SeFade> m_seFades;
};


