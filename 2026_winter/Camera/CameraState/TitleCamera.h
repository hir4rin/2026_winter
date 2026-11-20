#pragma once
#include "CameraStateBase.h"

//タイトルのカメラ//z=0の面を正面から写す。その面で1単位=設計解像度の1ピクセル
class TitleCamera :
	public CameraStateBase
{
public:
	TitleCamera(std::weak_ptr<CameraManager> owner);
	virtual ~TitleCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	//寄った所から引いて出てくるのを飛ばす
	void SkipIntro();

	Type GetCameraType()const override { return Type::TitleCamera; }
	virtual BlendSetting GetBlendSetting()const override { return {}; }

private:
	void UpdatePos();

	int m_frame = 0;
};
