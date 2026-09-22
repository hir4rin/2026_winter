#include "IDManager.h"

//変数の実態を定義して、初期値を設定
int IDManager::m_nextID = 0;


int IDManager::GetNextID()
{
	int id = m_nextID;
	m_nextID++;
    return id;
}
