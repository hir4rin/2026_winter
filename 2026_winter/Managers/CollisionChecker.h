#pragma once

class Collider;

class CollisionChecker
{
public:
	CollisionChecker();
	virtual ~CollisionChecker();
	/// <summary>
	/// 当たり判定の結果を返す
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool IsCollide(Collider& colA, Collider& colB);
private:

	/// <summary>
	/// 球と球の当たり判定
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool CheckCollSS(Collider& colA, Collider& colB);
	/// <summary>
	/// カプセルと球の当たり判定
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool CheckCollCS(Collider& colA, Collider& colB);

	bool CheckColCB(Collider& colA,Collider& colB){};//今回は省略;
	bool CheckColSB(Collider& colA,Collider& colB);
	bool CheckColPB(Collider& colA,Collider& colB){};//今回は省略
	bool CheckColBB(Collider& colA,Collider& colB){};//今回は省略

	/// <summary>
	/// カプセルとカプセルの当たり判定
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool CheckCollCC(Collider& colA, Collider& colB);
	/// <summary>
	/// カプセルとカプセルの当たり判定(DxLibVer)
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool CheckCollCCVerDxLib(Collider& colA, Collider& colB);
	/// <summary>
	/// 球とポリゴンの当たり判定
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool CheckCollSP(Collider& colA, Collider& colB);
	/// <summary>
	/// カプセルとポリゴンの当たり判定
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool CheckCollCP(Collider& colA, Collider& colB);
	/// <summary>
	/// カプセル同士で平行な場合の当たり判定
	/// </summary>
	/// <param name="colA"></param>
	/// <param name="colB"></param>
	/// <returns></returns>
	bool CheckParallelCC(Collider& colA, Collider& colB);
};

