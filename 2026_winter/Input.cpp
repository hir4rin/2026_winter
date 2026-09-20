#include "Input.h"
#include "Dxlib.h"
#include "System.h"
#include <algorithm>

namespace
{
	constexpr int kRightStickDeadZone = 8000;//スティックの傾きがこの値以下の場合は、傾いていないとみなす
	constexpr float kRightStickMaxValue = 32767.0f;//右スティックの入力値の最大値(-1.0〜1.0の割合に変換するのに使う)
}

Input::Input()
{

}
void Input::Init()
{
	//初期化
	m_inputData = {}, m_lastInputData = {}, m_inputTable = {};

	//イベント名を添え字にして、右辺値に実際の入力種別と入力コードの配列をおく
	m_inputTable["A"] = { { PeripheralType::keyboard,KEY_INPUT_Z },//Zキー
						  { PeripheralType::pad1,PAD_INPUT_1 } };//PADのAボタン

	m_inputTable["B"] = { { PeripheralType::keyboard,KEY_INPUT_RETURN },//Enterキー
						  { PeripheralType::pad1,PAD_INPUT_2 } };//PADのBボタン

	m_inputTable["X"] = { { PeripheralType::keyboard,KEY_INPUT_X },//Xキー
						  { PeripheralType::pad1,PAD_INPUT_3 } };//PADのXボタン

	m_inputTable["Y"] = { { PeripheralType::keyboard,KEY_INPUT_C },//Cキー
						  { PeripheralType::pad1,PAD_INPUT_4 } };//PADのYボタン

	m_inputTable["RB"] = { { PeripheralType::keyboard,KEY_INPUT_E },//Eキー
						  { PeripheralType::pad1,PAD_INPUT_6 } };//PADのRBボタン

	// m_inputTable["RT"] = { { PeripheralType::keyboard,KEY_INPUT_R },//Rキー
	  //                     { PeripheralType::pad1,PAD_INPUT_8 } };//PADのRTボタン

	m_inputTable["LB"] = { { PeripheralType::keyboard,KEY_INPUT_W },//Wキー
						  { PeripheralType::pad1,PAD_INPUT_5 } };//PADのLBボタン

	//m_inputTable["LT"] = { { PeripheralType::keyboard,KEY_INPUT_Q },//Qキー
	  //                    { PeripheralType::pad1,PAD_INPUT_7 } };//PADのLTボタン


	m_inputTable["="] = { { PeripheralType::keyboard,KEY_INPUT_V },
						  { PeripheralType::pad1,PAD_INPUT_7 } };//PADのSボタン

	m_inputTable["Start"] = { { PeripheralType::keyboard,KEY_INPUT_P },
						  { PeripheralType::pad1,PAD_INPUT_8 } };//SELECTキー

	m_inputTable["Up"] = { { PeripheralType::keyboard,KEY_INPUT_UP },
						  { PeripheralType::pad1,PAD_INPUT_UP} };
	m_inputTable["Down"] = { { PeripheralType::keyboard,KEY_INPUT_DOWN },
							{ PeripheralType::pad1,PAD_INPUT_DOWN} };
	m_inputTable["Left"] = { { PeripheralType::keyboard,KEY_INPUT_LEFT },
							{ PeripheralType::pad1,PAD_INPUT_LEFT} };
	m_inputTable["Right"] = { { PeripheralType::keyboard,KEY_INPUT_RIGHT },
							 { PeripheralType::pad1,PAD_INPUT_RIGHT} };

	// あらかじめ枠を開けておく
	//ここで枠を開けておかないと、チェックの際にAt関数でクラッシュする可能性がある
	for (const auto& inputInfo : m_inputTable) {
		m_inputData[inputInfo.first] = false;
		m_lastInputData[inputInfo.first] = false;

		m_rawInputData[inputInfo.first] = false;
		m_lastRawInputData[inputInfo.first] = false;
	}
}

bool Input::IsLeftStickInput()
{
	//十字キーのいずれかが押されているか
	if (IsPressed("Up") || IsPressed("Down") || IsPressed("Left") || IsPressed("Right"))
	{
		return true;
	}

	return false;
}

void Input::UpdateRecord()
{

	//最期まで再生終わったらリターン
	if (m_recordIndex >= m_inputRecord.size())
	{
		m_isRecording = false;
		return;
	}

	const auto& entry = m_inputRecord[m_recordIndex];
	for(const auto& name : entry.pressedButtons)
	{
		m_inputData[name] = true;//このフレームだけ終われているとみなすボタンだけtrue
	}

	//Frameを増やす
	if(++m_recordFrameInEntry >= entry.durationFrames)
	{
		m_recordFrameInEntry = 0;
		++m_recordIndex;
	}
}

void Input::Update()
{
	m_lastInputData = m_inputData;//直前のフレームを更新(前のフレーム情報をコピー)
	m_lastRawInputData = m_rawInputData;

	// まず現在の入力情報を取得
	char keyState[256];
	GetHitKeyStateAll(keyState);//生のキーボード情報//この関数が入力を全部とってくる(keyStateに入れてる)
	int padState = GetJoypadInputState(DX_INPUT_PAD1);//生のPAD1情報


	// すべての入力イベントをチェックします
	//ここでInputData_が更新される
	//inputTable_を回して各イベントをチェックする
	for (const auto& inputInfo : m_inputTable)
	{
		auto& inputRaw = m_rawInputData[inputInfo.first];//inputInfo.firstには"ok"等が入っている
		//inputを書き換えると、inputData_のそのイベントが押されているかどうかを
		//書き換えることになる
		//InputStateのベクタを回す
		for (const auto& state : inputInfo.second) {
			//このループはInputState配列のループなので
			//まず、入力種別をチェックします
			switch (state.type)
			{
			case PeripheralType::keyboard://キーボードだったら
				//GethitKeyStateAllでとってきた配列の中身を見て
				//入力されているかどうかをチェックします
				inputRaw = keyState[state.id];
				break;
			case PeripheralType::pad1:
				//GethitKeyStateAllでとってきたビット情報を見て
				//登録されているビット情報と&をとり、そのビットが立っているかどうかをチェック
				inputRaw = (padState & state.id);
				break;
			}
			if (inputRaw) {//必須!
				break;//ここでbreakしないと、最後のチェックで押されていないとfalseになる


			}
		}
	}

	//右スティックの入力を保存
	InputRightStick();

	//Record時
	if (m_isRecording)
	{
		//レコード中は実入力を無視して、配列のみで決める
		for(auto& [name,value] : m_inputData)
		{
			value = false;
		}
		UpdateRecord();
	}
	//イベント時
	else if (System::GetInstance().GetIsEventPlaying())
	{
		//入力を受け付けない
		for (auto& [name, value] : m_inputData)
		{
			value = false;
		}
	}
	//通常時
	else
	{
		//生の入力をそのまま保存する
		m_inputData = m_rawInputData;
	}


}

void Input::InputRightStick()
{
	//前回の右スティックの入力を保存
	m_rightStickInputPrev = m_rightStickInput;

	auto& x = m_rightStickInput.x;
	auto& y = m_rightStickInput.y;

	//毎フレーム初期化
	x = 0;
	y = 0;

	//右スティックの入力を取得する
	//0が返ってきたら成功、-1が返ってきたら失敗
	if (GetJoypadXInputState(DX_INPUT_PAD1, &m_xi) != 0)return;

	//右スティックの範囲は-32768~35767
	short rawX = m_xi.ThumbRX;
	short rawY = m_xi.ThumbRY;

	//デッドゾーンの設定
	if (abs(rawX) < kRightStickDeadZone)rawX = 0;
	if (abs(rawY) < kRightStickDeadZone)rawY = 0;


	//-1.0から1.0の範囲の割合に変える
	x = rawX / kRightStickMaxValue;
	y = rawY / kRightStickMaxValue;
	x = std::clamp(x, -1.0f, 1.0f);
	y = std::clamp(y, -1.0f, 1.0f);

}

bool Input::IsPressed(const char* name) const
{
	//もし「ない」イベントを送られるとクラッシュします
	//クラッシュがいやだったら
	//if (inputData_.contains(name))
	//{
	//    return false;//これで回避できます
	//}

	return m_inputData.at(name);//const関数内部なので[]ではなくatを使用している
}

bool Input::IsTriggered(const char* name) const
{
	return m_inputData.at(name) && !m_lastInputData.at(name);
}

bool Input::IsTriggeredRightStickInputX() const
{
	if(m_rightStickInput.x != 0 && m_rightStickInputPrev.x == 0)
	{
		return true;
	}

	return false;
}

void Input::StartRecord(const InputRecord& record)
{
	m_inputRecord = record;
	m_recordFrameInEntry = 0;
	m_recordIndex = 0;
	m_isRecording = true;
}

void Input::StopRecord()
{
	m_isRecording = false;
}

bool Input::IsRealPressed(const char* name) const
{
	return m_rawInputData.at(name);
}

bool Input::IsRealTriggered(const char* name) const
{
	return m_rawInputData.at(name) && !m_lastRawInputData.at(name);
}
