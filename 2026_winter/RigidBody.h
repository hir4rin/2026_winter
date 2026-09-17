#pragma once
#include "Math/Vector3.h"
class RigidBody
{
public:
	RigidBody();
	virtual ~RigidBody();

	//座標
	Vector3 GetPos()const { return m_pos; }
	void SetPos(Vector3 pos) { m_pos = pos;}
	void AddPos(Vector3 pos) { m_pos += pos; }

	//速度
	Vector3 GetVel()const { return m_vel; }
	Vector3 GetHorizonVel()const { return Vector3(m_vel.x, 0, m_vel.z); }//水平方向の速度を返す
	void SetVel(Vector3 vel) { m_vel = vel; }
	void AddVel(Vector3 vel) { m_vel += vel; }
	void ResetVel() { m_vel = Vector3(0, 0, 0);}

	//加速度
	Vector3 GetAccel()const { return m_accel; }
	Vector3 GetHorizonAccel()const { return Vector3(m_accel.x, 0, m_accel.z); }//水平方向の加速度を返す
	void SetAccel(Vector3 accel) { m_accel = accel; }
	void AddAccel(Vector3 accel) { m_accel += accel; }
	void ResetAccel() { m_accel = Vector3(0, 0, 0); }

private:

	Vector3 m_pos = Vector3();//座標

	Vector3 m_vel = Vector3();//速度
	Vector3 m_accel = Vector3();//加速度

};

