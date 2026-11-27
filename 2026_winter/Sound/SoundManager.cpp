#include "SoundManager.h"
#include "DxLib.h"
#include <cassert>


namespace
{
    //音量変換
    constexpr int kVolumeMax = 255;
    constexpr int kDisplayMax = 100;

    //BGM音量倍率(マスター音量に対する倍率。バランス調整はここを変更する)
    constexpr float kTitleBGMVolumeRate = 1.0f;
    constexpr float kBattleBGMVolumeRate = 1.0f;
    constexpr float kResultBGMVolumeRate = 1.0f;
    constexpr float kGameOverBGMVolumeRate = 1.0f;

    //SE音量倍率(マスター音量に対する倍率。バランス調整はここを変更する)
    constexpr float kAttackSwordSEVolumeRate = 1.5f;
    constexpr float kAttackSwordAirSEVolumeRate = 1.5f;
    constexpr float kAttackSwordHitSEVolumeRate = 1.5f;
    constexpr float kSkillSEVolumeRate = 1.5f;
    constexpr float kSkillAirSEVolumeRate = 1.5f;
    constexpr float kSkillHitSEVolumeRate = 1.5f;
    constexpr float kUltAttackSEVolumeRate = 1.5f;
    constexpr float kUltAttackHitSEVolumeRate = 1.5f;
    constexpr float kJumpUpAndDownVolumeRate = 1.5f;
    constexpr float kKickSEVolumeRate = 1.5f;
    constexpr float kKickAirSEVolumeRate = 1.5f;
    constexpr float kWalkSEVolumeRate = 1.5f;
    constexpr float kCameraSyatterSEVolumeRate = 1.5f;
    constexpr float kMascotHitSEVolumeRate = 1.5f;
    constexpr float kAwayMascotSEVolumeRate = 1.5f;
    constexpr float kWallBreakSEVolumeRate = 1.5f;
    constexpr float kGameClearSEVolumeRate = 1.5f;

    //ボイス系は他のSEより少し音量を小さくする
    constexpr float kZakoVoice1VolumeRate = 1.0f;
    constexpr float kZakoVoice2VolumeRate = 1.0f;

    //ボスのボイス
    constexpr float kBossHadouVoiceVolumeRate = 1.5f;
    constexpr float kBossFireToarchVoiceVolumeRate = 1.5f;
    constexpr float kBossSummonVoiceVolumeRate = 1.5f;
    constexpr float kBossLastVoiceVolumeRate = 1.5f;

    constexpr float kPlayerHitVolumeRate = 1.5f;

    //3重再生用スロットの数(連打されやすい攻撃音を割り当てる)
    constexpr int kAttackSwordSESlotCount = 3;
}

void SoundManager::Init()
{
	//BGMの読み込み
	m_sounds["TitleBGM"] = LoadSoundMem("data/Sound/BGM/TitleBGM.mp3");
	m_sounds["BattleBGM"] = LoadSoundMem("data/Sound/BGM/BattleBGM.mp3");
	m_sounds["ResultBGM"] = LoadSoundMem("data/Sound/BGM/ResultBGM.mp3");
	m_sounds["GameOverBGM"] = LoadSoundMem("data/Sound/BGM/GameOverBGM.mp3");

	//assert(m_sounds["TitleBGM"] >= 0);
	//音量の設定
	m_soundVolumes["TitleBGM"] = m_masterVolume * kTitleBGMVolumeRate;
	m_soundVolumes["BattleBGM"] = m_masterVolume * kBattleBGMVolumeRate;
	m_soundVolumes["ResultBGM"] = m_masterVolume * kResultBGMVolumeRate;
	m_soundVolumes["GameOverBGM"] = m_masterVolume * kGameOverBGMVolumeRate;

	//SEの読み込み(プレイヤー)
	m_sounds["AttackSwordSE"] = LoadSoundMem("data/Sound/SE/Player/AttackSwordSE.mp3");
	m_sounds["AttackSwordAirSE"] = LoadSoundMem("data/Sound/SE/Player/AttackSwordAirSE.mp3");
	m_sounds["AttackSwordHitSE"] = LoadSoundMem("data/Sound/SE/Player/AttackSwordHitSE.mp3");
	m_sounds["SkillSE"] = LoadSoundMem("data/Sound/SE/Player/SkillSE.mp3");
	m_sounds["SkillAirSE"] = LoadSoundMem("data/Sound/SE/Player/SkillAirSE.mp3");
	m_sounds["SkillHitSE"] = LoadSoundMem("data/Sound/SE/Player/SkillHitSE.mp3");
	m_sounds["UltAttackSE"] = LoadSoundMem("data/Sound/SE/Player/UltAttackSE.mp3");
	m_sounds["UltAttackHitSE"] = LoadSoundMem("data/Sound/SE/Player/UltAttackHitSE.mp3");
	m_sounds["JumpUpAndDown"] = LoadSoundMem("data/Sound/SE/Player/JumpUpAndDown.mp3");
	m_sounds["KickSE"] = LoadSoundMem("data/Sound/SE/Player/KickSE.mp3");
	m_sounds["KickAirSE"] = LoadSoundMem("data/Sound/SE/Player/KickAirSE.mp3");
	m_sounds["WalkSE"] = LoadSoundMem("data/Sound/SE/Player/WalkSE.mp3");

	//SEの読み込み(システム)
	m_sounds["CameraSyatterSE"] = LoadSoundMem("data/Sound/SE/System/CameraSyatterSE.mp3");
	m_sounds["MascotHitSE"] = LoadSoundMem("data/Sound/SE/System/MascotHitSE.mp3");
	m_sounds["AwayMascotSE"] = LoadSoundMem("data/Sound/SE/System/AwayMascotSE.mp3");
	m_sounds["WallBreakSE"] = LoadSoundMem("data/Sound/SE/System/WallBreakSE.mp3");
	m_sounds["GameClearSE"] = LoadSoundMem("data/Sound/SE/System/GameClearSE.mp3");

	//SEの読み込み(雑魚敵のボイス)
	m_sounds["ZakoVoice1"] = LoadSoundMem("data/Sound/SE/ZakoEnemy/ZakoVoice1.mp3");
	m_sounds["ZakoVoice2"] = LoadSoundMem("data/Sound/SE/ZakoEnemy/ZakoVoice2.mp3");

	//SEの読み込み(ボスのボイス)
	m_sounds["BossHadouVoice"] = LoadSoundMem("data/Sound/SE/BossEnemy/BossHadouVoice.mp3");
	m_sounds["BossFireToarchVoice"] = LoadSoundMem("data/Sound/SE/BossEnemy/BossFireToarchVoice.mp3");
	m_sounds["BossSummonVoice"] = LoadSoundMem("data/Sound/SE/BossEnemy/BossSummon.mp3");
	m_sounds["BossLastVoice"] = LoadSoundMem("data/Sound/SE/BossEnemy/BossLastVoice.mp3");

	//SEの読み込み(プレイヤーの被弾)
	m_sounds["PlayerHit"] = LoadSoundMem("data/Sound/SE/Player/PlayerHit.mp3");
	//音量の設定
	m_seVolumes["AttackSwordSE"] = m_masterVolume2 * kAttackSwordSEVolumeRate;
	m_seVolumes["AttackSwordAirSE"] = m_masterVolume2 * kAttackSwordAirSEVolumeRate;
	m_seVolumes["AttackSwordHitSE"] = m_masterVolume2 * kAttackSwordHitSEVolumeRate;
	m_seVolumes["SkillSE"] = m_masterVolume2 * kSkillSEVolumeRate;
	m_seVolumes["SkillAirSE"] = m_masterVolume2 * kSkillAirSEVolumeRate;
	m_seVolumes["SkillHitSE"] = m_masterVolume2 * kSkillHitSEVolumeRate;
	m_seVolumes["UltAttackSE"] = m_masterVolume2 * kUltAttackSEVolumeRate;
	m_seVolumes["UltAttackHitSE"] = m_masterVolume2 * kUltAttackHitSEVolumeRate;
	m_seVolumes["JumpUpAndDown"] = m_masterVolume2 * kJumpUpAndDownVolumeRate;
	m_seVolumes["KickSE"] = m_masterVolume2 * kKickSEVolumeRate;
	m_seVolumes["KickAirSE"] = m_masterVolume2 * kKickAirSEVolumeRate;
	m_seVolumes["WalkSE"] = m_masterVolume2 * kWalkSEVolumeRate;
	m_seVolumes["CameraSyatterSE"] = m_masterVolume2 * kCameraSyatterSEVolumeRate;
	m_seVolumes["MascotHitSE"] = m_masterVolume2 * kMascotHitSEVolumeRate;
	m_seVolumes["AwayMascotSE"] = m_masterVolume2 * kAwayMascotSEVolumeRate;
	m_seVolumes["WallBreakSE"] = m_masterVolume2 * kWallBreakSEVolumeRate;
	m_seVolumes["GameClearSE"] = m_masterVolume2 * kGameClearSEVolumeRate;
	m_seVolumes["ZakoVoice1"] = m_masterVolume2 * kZakoVoice1VolumeRate;
	m_seVolumes["ZakoVoice2"] = m_masterVolume2 * kZakoVoice2VolumeRate;
	m_seVolumes["BossHadouVoice"] = m_masterVolume2 * kBossHadouVoiceVolumeRate;
	m_seVolumes["BossFireToarchVoice"] = m_masterVolume2 * kBossFireToarchVoiceVolumeRate;
	m_seVolumes["BossSummonVoice"] = m_masterVolume2 * kBossSummonVoiceVolumeRate;
	m_seVolumes["BossLastVoice"] = m_masterVolume2 * kBossLastVoiceVolumeRate;
	m_seVolumes["PlayerHit"] = m_masterVolume2 * kPlayerHitVolumeRate;

	// サウンド名のリスト
	const std::vector<std::string> soundNames = {
		"TitleBGM",
		"BattleBGM",
		"ResultBGM",
		"GameOverBGM"
	};
	//SEのリスト
	const std::vector<std::string> seNames = {
		"AttackSwordSE",
		"AttackSwordAirSE",
		"AttackSwordHitSE",
		"SkillSE",
		"SkillAirSE",
		"SkillHitSE",
		"UltAttackSE",
		"UltAttackHitSE",
		"JumpUpAndDown",
		"KickSE",
		"KickAirSE",
		"WalkSE",
		"CameraSyatterSE",
		"MascotHitSE",
		"AwayMascotSE",
		"WallBreakSE",
		"GameClearSE",
		"ZakoVoice1",
		"ZakoVoice2",
		"BossHadouVoice",
		"BossFireToarchVoice",
		"BossSummonVoice",
		"BossLastVoice",
		"PlayerHit"
	};

	//BGMの音量設定
	for (const auto& key : soundNames)
	{
		ChangeVolumeSoundMem(m_soundVolumes[key], m_sounds[key]);
	}
	//SEの音量設定
	for (const auto& key : seNames)
	{
		ChangeVolumeSoundMem(m_seVolumes[key], m_sounds[key]);
	}
	//3重再生用スロットの初期化(連打されやすい攻撃音を割り当てる)
	for (int i = 0; i < kAttackSwordSESlotCount; ++i)
	{
		int h = LoadSoundMem("data/Sound/SE/Player/AttackSwordSE.mp3");
		m_seSlots.push_back({ h });
	}

}

void SoundManager::PlaySE(const std::string& name)
{
    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) return;

    if(name == "AttackSwordSE")
    {
        // 3重再生用スロットを探す
        for (auto& slot : m_seSlots)
        {
            if ( CheckSoundMem(slot.handle) == 0)
            {
              //二回目が鳴らない
                ChangeVolumeSoundMem(ApplyMute(m_masterVolume2), slot.handle);
                PlaySoundMem(slot.handle, DX_PLAYTYPE_BACK);
                return;
            }
        }
        // 全てのスロットが再生中の場合、最初のスロットを強制的に再生
        auto& slot = m_seSlots[0];
        StopSoundMem(slot.handle);
        ChangeVolumeSoundMem(ApplyMute(m_masterVolume2), slot.handle);
        PlaySoundMem(slot.handle, DX_PLAYTYPE_BACK);
        return;
	}
    else
    {
        ChangeVolumeSoundMem(ApplyMute(m_seVolumes[name]), it->second);
        PlaySoundMem(it->second, DX_PLAYTYPE_BACK);
    }


 
}

void SoundManager::PlayBgm(const std::string& name)
{
    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) return;

    if (m_currentBgm != -1)
    {
        StopSoundMem(m_currentBgm);
    }

    m_currentBgm = it->second;//ハンドルの番号
    ChangeVolumeSoundMem(ApplyMute(m_soundVolumes[name]), m_currentBgm);
    PlaySoundMem(m_currentBgm, DX_PLAYTYPE_LOOP);
}

void SoundManager::PlaySELoop(const std::string& name)
{
	auto it = m_sounds.find(name);
	if (it == m_sounds.end()) return;
	//すでに再生中なら、再生しない
	if (CheckSoundMem(it->second) == 1)return;
	ChangeVolumeSoundMem(ApplyMute(m_seVolumes[name]), it->second);
	PlaySoundMem(it->second, DX_PLAYTYPE_LOOP);
}

void SoundManager::StopBgm()
{
    if (m_currentBgm != -1)
    {
        StopSoundMem(m_currentBgm);
        m_currentBgm = -1;
    }
}

void SoundManager::FadeOutBgm(int frames)
{
	if (m_currentBgm == -1)return;
	if (CheckSoundMem(m_currentBgm) == 0)return;//鳴っていないなら何もしない

	SeFade fade;
	fade.handle = m_currentBgm;
	fade.startVolume = m_masterVolume;//BGMの音量倍率は現状すべて1.0倍なので、マスター音量をそのまま使う
	fade.elapsedFrame = 0;
	fade.totalFrame = frames > 0 ? frames : 1;
	m_seFades.push_back(fade);

	//フェード側の管理に移すので、現在のBGMハンドルとしては扱わない(直後にPlayBgmで別のBGMを重ねて再生できる)
	m_currentBgm = -1;
}

void SoundManager::StopSE(const std::string& name)
{
    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) return;
    StopSoundMem(it->second);
}

void SoundManager::FadeOutSE(const std::string& name, int frames)
{
	auto it = m_sounds.find(name);
	if (it == m_sounds.end()) return;
	int handle = it->second;
	if (CheckSoundMem(handle) == 0)return;//鳴っていないなら何もしない

	SeFade fade;
	fade.handle = handle;
	fade.startVolume = m_seVolumes[name];//現在設定されている音量から下げていく
	fade.elapsedFrame = 0;
	fade.totalFrame = frames > 0 ? frames : 1;
	m_seFades.push_back(fade);
}

void SoundManager::Update()
{
	//フェードアウト中のSEの音量を毎フレーム下げていく
	for (auto it = m_seFades.begin(); it != m_seFades.end();)
	{
		it->elapsedFrame++;
		float t = static_cast<float>(it->elapsedFrame) / it->totalFrame;
		if (t >= 1.0f)
		{
			StopSoundMem(it->handle);
			it = m_seFades.erase(it);
		}
		else
		{
			int volume = static_cast<int>(it->startVolume * (1.0f - t));
			ChangeVolumeSoundMem(ApplyMute(volume), it->handle);
			++it;
		}
	}
}

void SoundManager::SetBgmVolume(int volume)
{
    m_masterVolume = volume;
    ChangeVolumeSoundMem(ApplyMute(m_masterVolume), m_currentBgm);
}

void SoundManager::SetSEVolume(int volume)
{
    m_masterVolume2 = volume;
    //SEを流す
    PlaySE("cursor");
}

void SoundManager::ToggleMute()
{
	m_isMuted = !m_isMuted;

	//再生中のBGMは、切り替えた瞬間に音量を反映させる
	//(SEは1回鳴らして終わる短い音がほとんどなので、次に再生されたときに自然とミュート状態が反映される)
	if (m_currentBgm != -1)
	{
		ChangeVolumeSoundMem(ApplyMute(m_masterVolume), m_currentBgm);
	}
}

