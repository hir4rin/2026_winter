#pragma once
class IDManager
{
public:
	static int GetNextID();
private:
	IDManager() = delete;
	~IDManager() = delete;
	static int m_nextID;
	//IDManagerはstaticではなくて、CollisionManagerに持たせるという択もある
};

