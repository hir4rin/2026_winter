#pragma once
#include<array>
#include<map>
#include<string>
#include<vector>
#include<DxLib.h>

/// <summary>
/// 周辺機器種別
/// </summary>
enum class PeripheralType {
	keyboard,
	pad1
};

/// <summary>
/// 入力対応情報
/// </summary>
struct InputState
{
	PeripheralType type;// 入力された機器の種別
	int id;//入力情報が入る(キーボードの場合はインデックス、パッドの場合はビット)
};

//右スティックの入力値
struct RightStickInput
{
	float x = 0;
	float y = 0;
};

//リプレイや、機械的な時用
struct InputRecordData
{
	int durationFrames = 1;//この状態を何フレーム維持するか
	std::vector<std::string> pressedButtons;//押されているボタンの名前
};
using InputRecord = std::vector<InputRecordData>;//入力の記録


/// <summary>
/// 入力を抽象化するためのクラス
/// </summary>
class Input
{
public:
	static Input& GetInstance()
	{
		static Input instance;
		return instance;
	}

	void Init();//入力の初期化//イベントと入力の対応表を作る
	
	bool IsLeftStickInput();
	//何かボタンがされたかのboolとか便利な機能をいろいろ作りたい

private:
	//シングルトンクラス
	Input();
	~Input() = default;
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

private:
	std::map<std::string, std::vector<InputState>>m_inputTable;// イベント名と実際の入力の対応表
	std::map<std::string, bool>m_inputData;//実際に入力されたかどうかのデータ
	std::map<std::string, bool>m_lastInputData;//最後に入力されたかどうかのデータ
	
	std::map<std::string, bool> m_rawInputData;//ユーザーの実入力(レコードの影響をうけない)を受け付けるときに使うやつ
	std::map<std::string, bool> m_lastRawInputData;//ユーザーの実入力(レコードの影響をうけない)を受け付けるときに使うやつ

	//右スティックの入力を取得するための変数
	XINPUT_STATE  m_xi = {};
	RightStickInput m_rightStickInput = {};
	RightStickInput m_rightStickInputPrev = {};

	//Record用の変数
	bool m_isRecording = false;//リプレイ中かどうか
	InputRecord m_inputRecord;//入力の記録
	size_t m_recordIndex = 0;//リプレイ中のインデックス
	int m_recordFrameInEntry = 0;//リプレイ中のフレーム数
	void UpdateRecord();//リプレイ中の入力を更新する

public:
	/// <summary>
	///  これを毎フレーム呼び出すことで
	/// 入力情報が更新されます
	/// </summary>
	void Update();
	/// <summary>
	/// 右スティックの入力を保存する
	/// </summary>
	void InputRightStick();

	// 特定のボタンが押されたか
	/// <param name="name">イベント名</param>
	/// <returns>押されている:true/押されていない:false</returns>
	bool IsPressed(const char*name)const;

	// 特定のボタンが今押されたか
	/// <param name="name">イベント名</param>
	/// <returns>今押された瞬間:true/押されていないor押しっぱなし:false</returns>
	bool IsTriggered(const char* name)const;

	/// <summary>
	/// 右スティックの入力値(-1.0～1.0)を取得
	/// </summary>
	 RightStickInput GetRightStickInput()const { return m_rightStickInput; }
	 /// <summary>
	 /// 右スティックのX軸の入力のトリガーを取得する
	 /// </summary>
	 bool IsTriggeredRightStickInputX()const;

	 //レコード用の関数
	 void StartRecord(const InputRecord& record);//レコードの開始
	 void StopRecord();//レコードの停止
	 bool IsReplaying()const { return m_isRecording; }//リプレイ中かどうか
	 //ユーザーの実入力(レコードの影響をうけない)を受け付けるときに使うやつ
	 bool IsRealPressed(const char* name)const;
	 bool IsRealTriggered(const char* name)const;


};
