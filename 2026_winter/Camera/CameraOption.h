#pragma once

//プレイヤーが変えるカメラの設定(ポーズのカメラ設定)
struct CameraOption
{
	static constexpr int kSensitivityMin = 1;
	static constexpr int kSensitivityMax = 10;
	static constexpr int kSensitivityDefault = 5;//今までの速さ

	int sensitivity = kSensitivityDefault;
	bool isInvertX = false;//左右反転
	bool isInvertY = false;//上下反転

	//回転の速さにかける倍率
	float GetSpeedRate() const;
	//反転の符号(1 か -1)
	float GetSignX() const { return isInvertX ? -1.0f : 1.0f; }
	float GetSignY() const { return isInvertY ? -1.0f : 1.0f; }
};
