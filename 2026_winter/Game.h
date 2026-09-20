#pragma once

//ゲーム全体で使用する定数
namespace Game
{
	//画面情報(設計解像度、UI座標の比率計算の基準にする)
	constexpr int kScreenWidth = 1280;
	constexpr int kScreenHeight = 720;
	constexpr int kColorBitNum = 32;

	//現在の実解像度を取得する(ウィンドウ/フルスクリーンの切り替えで変わる)
	int GetScreenWidth();
	int GetScreenHeight();
	//実解像度を変更する(フルスクリーン切り替え時にApplicationから呼ぶ)
	void SetScreenSize(int width, int height);

	//設計解像度(kScreenWidth/kScreenHeight)を基準にした画面比率で座標・サイズを変換する
	float ScaleX(float designX);
	float ScaleY(float designY);
	//縦横共通の拡大率(横方向を基準にする、アスペクト比が同じ前提)
	float GetScale();

	//重力
	constexpr float kGravity = 1.0f;//重力の加速度//この数値を変えることで、ジャンプの高さや落ちる速さを調整できる

	//Playerの移動スピード
	constexpr float kMoveSpeed = 12.0f;//移動速度
	constexpr float kAirMaxSpeed = 6.5f;//空中での最大移動速度


	//敵の移動スピード
	constexpr float kEnemyMoveSpeed = 4.0f;//敵の移動速度
	constexpr float kEnemyBackSpeed = 5.0f;//敵の距離を取るときの移動速度
}



//UIの表示座標一覧
namespace UI
{
	constexpr int kBarScale = 300;//バーの長さ//割合

	/*constexpr int HpBarX = Game::kScreenWidth * 1 / 2;
	constexpr int HpBarY = Game::kScreenHeight / 40;*/
	constexpr int HpBarX = 10;
	constexpr int HpBarY = 20;

	/*constexpr int SkillGaugeBarX = Game::kScreenWidth / 15;
	constexpr int SkillGaugeBarY = Game::kScreenHeight / 20;*/
	constexpr int SkillGaugeBarX = 10;
	constexpr int SkillGaugeBarY = 90;

	/*constexpr int UltGaugeBarX = Game::kScreenWidth / 15;
	constexpr int UltGaugeBarY = Game::kScreenHeight / 10;*/
	constexpr int UltGaugeBarX = 10;
	constexpr int UltGaugeBarY = 160;
}

enum class ComboChainButton : int
{
	X = 0,
	Y = 1,
	A = 2,
	B = 3,
	RB = 4,	
	RT = 5,
	LB = 6,
	LT = 7,
	RStickLR = 8,
	Hold = 9,
	ButtonSize = 10
};

namespace ComboUI
{
	constexpr float kButtonScale = 1.0f;//ボタンの大きさ//倍率

	constexpr int ButtonToButtonDistanceX = 50;//ボタンとボタンの間の距離//横
	constexpr int ButtonToButtonDistanceY = 75;//ボタンとボタンの間の距離//縦(ボタンの高さ64pxに対して、文字と重ならないよう余白を広めにとっている)

	constexpr int kStringDistanceY = 45;//文字とボタンの間の距離//縦(ボタンの高さ64px(半分32px)より広くとって重ならないようにする)

	//コンボ一覧UIの基準位置(すべての項目で共通)
	const int kComboUIStartX = Game::kScreenWidth * 8 / 10;//横位置(画面幅の8割の位置)
	constexpr int kComboUIStartY = 100;//縦位置の基準(ここにButtonToButtonDistanceY*段数を足していく)

	//弱5ボタン
	constexpr ComboChainButton kCombo1[] = { ComboChainButton::X, ComboChainButton::X, ComboChainButton::X,ComboChainButton::X,ComboChainButton::X };
	//開始位置
	const int kCombo1StartX = kComboUIStartX;
	const int kCombo1StartY = kComboUIStartY + ButtonToButtonDistanceY * 1;

	//強2ボタン
	constexpr ComboChainButton kCombo2[] = { ComboChainButton::Y, ComboChainButton::Y };
	//開始位置
	const int kCombo2StartX = kComboUIStartX;
	const int kCombo2StartY = kComboUIStartY + ButtonToButtonDistanceY * 2;
	//打ち上げコンボ
	constexpr ComboChainButton kAirCombo[] = { ComboChainButton::X, ComboChainButton::Y,};
	//開始位置
	const int kAirComboStartX = kComboUIStartX;
	const int kAirComboStartY = kComboUIStartY + ButtonToButtonDistanceY * 3;

	//スキル3コンボボタン
	constexpr ComboChainButton kSkillCombo[] = {ComboChainButton::Hold,ComboChainButton::LB, ComboChainButton::X, ComboChainButton::X, ComboChainButton::X };
	//開始位置
	const int kSkillStartX = kComboUIStartX;
	const int kSkillStartY = kComboUIStartY + ButtonToButtonDistanceY * 4;

	//必殺技ボタン
	constexpr ComboChainButton kUlt[] = { ComboChainButton::Hold,ComboChainButton::LB, ComboChainButton::Y };
	//開始位置
	const int kUltStartX = kComboUIStartX;
	const int kUltStartY = kComboUIStartY + ButtonToButtonDistanceY * 5;

	//ロックオン
	constexpr ComboChainButton kLockOn = ComboChainButton::RB;
	//開始位置
	const int kLockOnStartX = kComboUIStartX;
	const int kLockOnStartY = kComboUIStartY + ButtonToButtonDistanceY * 6;
	//ロックオン切換え
	constexpr ComboChainButton kLockOnChange = ComboChainButton::RStickLR;
	//開始位置
	const int kLockOnChangeStartX = kComboUIStartX;
	const int kLockOnChangeStartY = kComboUIStartY + ButtonToButtonDistanceY * 7;

	
}