#include "CameraOption.h"

float CameraOption::GetSpeedRate() const
{
	return static_cast<float>(sensitivity) / static_cast<float>(kSensitivityDefault);
}
