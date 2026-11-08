#include "BloodDecalManager.h"
#include "Main/System.h"
#include <algorithm>
#include <cmath>

namespace
{
	const char* const kGraphPath = "data/Decal/BloodDecal.png";
	constexpr int kCellNum = 2;//画像の1辺のマス数
	constexpr int kPoolCellY = 0;//血だまりの段
	constexpr int kSplatCellY = 1;//飛沫の段

	const Vector3 kUp = Vector3(0.0f, 1.0f, 0.0f);
	constexpr float kFloorNormalMinY = 0.5f;//これより立っている面は床ではない
	constexpr float kRayUp = 100.0f;//足元のどれだけ上から調べるか
	constexpr float kPoolRayDown = 3000.0f;//空中でも床を探せる長さ
	constexpr float kSplatRayDown = 150.0f;//飛沫が下の段に落ちない長さ
	constexpr float kViewRayRange = 45.0f;//当たり判定の床から見た目の床を探す範囲
	constexpr int kViewCollDivX = 32;//見た目のモデルの当たり判定の分割数
	constexpr int kViewCollDivY = 8;
	constexpr int kViewCollDivZ = 32;
	constexpr float kFitCornerRate = 0.75f;//画像の端は透明なので四隅より内側を調べる
	constexpr float kFitRayRange = 15.0f;//床の段差の許容
	constexpr float kFitScaleRate = 0.7f;//はみ出したときの縮小率
	constexpr int kFitTryNum = 3;//縮小を試す回数
	constexpr float kFitSkipSize = 30.0f;//これより小さい粒ははみ出しを調べない

	//血だまり
	constexpr float kPoolSizeMin = 120.0f;//大きさの最小
	constexpr float kPoolSizeMax = 165.0f;//大きさの最大
	constexpr float kPoolDelay = 2.0f;//出るまで
	constexpr float kPoolGrowFrame = 10.0f;//広がる時間

	//飛沫(飛ぶ向きに細長い)
	constexpr int kSplatNum = 10;//数
	constexpr float kSplatSizeMin = 45.0f;//大きさの最小
	constexpr float kSplatSizeMax = 115.0f;//大きさの最大
	constexpr float kSplatDistMin = 30.0f;//飛ぶ距離の最小
	constexpr float kSplatDistMax = 400.0f;//飛ぶ距離の最大
	constexpr float kSplatSpreadAngle = DX_PI_F * 0.28f;//飛ぶ向きの広がり
	constexpr float kSplatStretchMax = 2.4f;//一番遠い飛沫の長さの倍率

	//細かい粒
	constexpr int kDropNum = 14;//数
	constexpr float kDropSizeMin = 9.0f;//大きさの最小
	constexpr float kDropSizeMax = 26.0f;//大きさの最大
	constexpr float kDropDistMin = 40.0f;//飛ぶ距離の最小
	constexpr float kDropDistMax = 560.0f;//飛ぶ距離の最大
	constexpr float kDropSpreadAngle = DX_PI_F * 0.42f;//飛ぶ向きの広がり
	constexpr float kDropStretchMax = 2.6f;//一番遠い粒の長さの倍率

	//少量
	constexpr float kSmallSizeRate = 0.45f;//大きさの倍率
	constexpr float kSmallDistRate = 0.6f;//距離の倍率
	constexpr int kSmallSplatNum = 3;//飛沫の数
	constexpr int kSmallDropNum = 6;//粒の数

	//動き
	constexpr float kFlySpeed = 75.0f;//床を飛ぶ速さ(遠い飛沫ほど遅れて着く)
	constexpr float kFallSpeed = 60.0f;//空中から床へ落ちる速さ
	constexpr float kSplatGrowFrame = 3.0f;//飛沫が広がる時間
	constexpr float kStartScale = 0.6f;//出た瞬間の大きさ

	//見た目
	constexpr float kLifeFrame = 1200.0f;//出てから消えるまで
	constexpr float kFadeFrame = 180.0f;//薄くなる時間
	constexpr float kDryFrame = 150.0f;//新しい血の色から乾いた色になるまで
	constexpr float kAlpha = 0.95f;//濃さ
	constexpr float kFreshR = 190.0f;//新しい血の色
	constexpr float kFreshG = 12.0f;
	constexpr float kFreshB = 18.0f;
	constexpr float kDryR = 88.0f;//乾いた血の色
	constexpr float kDryG = 5.0f;
	constexpr float kDryB = 10.0f;

	constexpr float kDepthBias = 0.00024f;//床とちらつかないようにカメラへ寄せる深度の量
}

void BloodDecalManager::Init(int stageColHandle, int stageViewHandle)
{
	//ステージのモデルを覚える
	m_stageColHandle = stageColHandle;
	m_stageViewHandle = stageViewHandle;
	//見た目のモデルに線を飛ばせるようにする
	MV1SetupCollInfo(m_stageViewHandle, -1, kViewCollDivX, kViewCollDivY, kViewCollDivZ);
	//画像の読み込み
	m_graphHandle = LoadGraph(kGraphPath);

	//三角形の並びは変わらない
	for (int i = 0; i < kMaxDecalNum; i++)
	{
		const unsigned short base = static_cast<unsigned short>(i * kVertexPerDecal);//この血の最初の頂点
		const int top = i * kIndexPerDecal;//この血の最初の番号
		//1枚目の三角形
		m_indices[top + 0] = base;
		m_indices[top + 1] = base + 1;
		m_indices[top + 2] = base + 2;
		//2枚目の三角形
		m_indices[top + 3] = base + 2;
		m_indices[top + 4] = base + 1;
		m_indices[top + 5] = base + 3;
	}
}

void BloodDecalManager::Terminate()
{
	//画像の解放
	if (m_graphHandle != -1)
	{
		DeleteGraph(m_graphHandle);
		m_graphHandle = -1;
	}
	//線を飛ばす準備の解放
	if (m_stageViewHandle != -1)
	{
		MV1TerminateCollInfo(m_stageViewHandle, -1);
		m_stageViewHandle = -1;
	}
	m_stageColHandle = -1;
}

void BloodDecalManager::Spawn(const Vector3& footPos, const Vector3& dir, BloodAmount amount)
{
	//準備前は何もしない
	if (m_stageColHandle == -1)return;

	//足元の床を探す
	Decal pool;
	if (!FindFloor(footPos + kUp * kRayUp, footPos - kUp * kPoolRayDown, pool.center, pool.normal))return;

	//量の倍率
	const bool isSmall = amount == BloodAmount::Small;
	const float sizeRate = isSmall ? kSmallSizeRate : 1.0f;
	const float distRate = isSmall ? kSmallDistRate : 1.0f;

	//空中なら落ちる分だけ遅らせる
	const Vector3 ground = pool.center;
	const float fallDelay = (std::max)(footPos.y - ground.y, 0.0f) / kFallSpeed;

	//血だまり
	const float poolAngle = RandRange(0.0f, DX_TWO_PI_F);//向きはランダム
	pool.halfSize = RandRange(kPoolSizeMin, kPoolSizeMax) * sizeRate;
	pool.delay = fallDelay + kPoolDelay;
	pool.growFrame = kPoolGrowFrame;
	pool.cellX = GetRand(kCellNum - 1);//形はランダム
	pool.cellY = kPoolCellY;
	AddDecal(pool, Vector3(sinf(poolAngle), 0.0f, cosf(poolAngle)));

	//血が飛ぶ向きの角度
	const float baseAngle = atan2f(dir.x, dir.z);

	//飛沫
	SplatParam splat;
	splat.num = isSmall ? kSmallSplatNum : kSplatNum;
	splat.sizeMin = kSplatSizeMin * sizeRate;
	splat.sizeMax = kSplatSizeMax * sizeRate;
	splat.distMin = kSplatDistMin * distRate;
	splat.distMax = kSplatDistMax * distRate;
	splat.spreadAngle = kSplatSpreadAngle;
	splat.stretchMax = kSplatStretchMax;
	AddSplats(ground, baseAngle, fallDelay, splat);

	//細かい粒
	SplatParam drop;
	drop.num = isSmall ? kSmallDropNum : kDropNum;
	drop.sizeMin = kDropSizeMin;
	drop.sizeMax = kDropSizeMax;
	drop.distMin = kDropDistMin * distRate;
	drop.distMax = kDropDistMax * distRate;
	drop.spreadAngle = kDropSpreadAngle;
	drop.stretchMax = kDropStretchMax;
	AddSplats(ground, baseAngle, fallDelay, drop);
}

void BloodDecalManager::Update()
{
	const float timeScale = System::GetInstance().GetTimeScale();
	for (auto& decal : m_decals)
	{
		if (!decal.isActive)continue;
		//時計を進める
		decal.age += timeScale;
		//寿命が来たら消す
		if (decal.age >= decal.delay + kLifeFrame)
		{
			decal.isActive = false;
		}
	}
}

void BloodDecalManager::Draw()
{
	if (m_graphHandle == -1)return;

	const Vector3 cameraPos = Vector3::FromDxLibVector(GetCameraPosition());
	const float depthRate = kDepthBias / GetCameraNear();//カメラへ寄せる割合
	const float cellUV = 1.0f / kCellNum;//1マスのUVの幅

	int drawNum = 0;//描く枚数
	for (const auto& decal : m_decals)
	{
		//出番の前は描かない
		if (!decal.isActive || decal.age < decal.delay)continue;

		//出てからの時間
		const float time = decal.age - decal.delay;
		//広がり具合(最初に一気に広がる)
		const float growRate = (std::min)(time / decal.growFrame, 1.0f);
		const float invRate = 1.0f - growRate;
		const float easedRate = 1.0f - invRate * invRate * invRate;
		//横の大きさ
		const float sizeX = decal.halfSize * (kStartScale + (1.0f - kStartScale) * easedRate);
		//飛ぶ向きの大きさ
		const float sizeZ = sizeX * (1.0f + (decal.stretch - 1.0f) * easedRate);

		//最後に薄くする
		const float fadeTime = time - (kLifeFrame - kFadeFrame);
		const float alpha = kAlpha * (1.0f - (std::clamp)(fadeTime / kFadeFrame, 0.0f, 1.0f));
		//だんだん乾いた色にする
		const float dryRate = (std::min)(time / kDryFrame, 1.0f);
		const COLOR_U8 color = GetColorU8(
			static_cast<int>(kFreshR + (kDryR - kFreshR) * dryRate),
			static_cast<int>(kFreshG + (kDryG - kFreshG) * dryRate),
			static_cast<int>(kFreshB + (kDryB - kFreshB) * dryRate),
			static_cast<int>(255 * alpha));
		const COLOR_U8 specular = GetColorU8(0, 0, 0, 0);//反射なし

		//四隅の頂点を作る
		for (int i = 0; i < kVertexPerDecal; i++)
		{
			const int sideX = (i % 2 == 0) ? -1 : 1;//左か右か
			const int sideZ = (i < 2) ? 1 : -1;//奥か手前か
			//床に沿った頂点の位置
			const Vector3 pos = decal.center + decal.right * (sideX * sizeX) + decal.forward * (sideZ * sizeZ);
			//視線に沿って寄せるので画面の位置は変わらない
			const Vector3 toCamera = cameraPos - pos;

			VERTEX3D& vertex = m_vertices[drawNum * kVertexPerDecal + i];
			vertex.pos = (pos + toCamera * (depthRate * toCamera.Magnitude())).ToDxLibVector();//位置
			vertex.norm = decal.normal.ToDxLibVector();//法線
			vertex.dif = color;//色
			vertex.spc = specular;//反射
			//ここでどの画像を使うかを決める
			vertex.u = (decal.cellX + (sideX + 1) * 0.5f) * cellUV;//画像の横
			vertex.v = (decal.cellY + (1 - sideZ) * 0.5f) * cellUV;//画像の縦
			vertex.su = 0.0f;//使わない
			vertex.sv = 0.0f;//使わない
		}
		drawNum++;
	}
	//描くものが無い
	if (drawNum == 0)return;

	//描画の設定
	const int prevDrawMode = GetDrawMode();//元の設定を覚える
	SetDrawMode(DX_DRAWMODE_BILINEAR);//なめらかに
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);//半透明
	SetUseLighting(FALSE);//ライトの影響なし
	SetUseZBuffer3D(TRUE);//手前の物に隠れる
	SetWriteZBuffer3D(FALSE);//深度は書かない

	//まとめて描く
	DrawPolygonIndexed3D(m_vertices.data(), drawNum * kVertexPerDecal,
		m_indices.data(), drawNum * 2, m_graphHandle, TRUE);

	//設定を戻す
	SetWriteZBuffer3D(TRUE);
	SetUseLighting(TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(prevDrawMode);
}

bool BloodDecalManager::FindFloor(const Vector3& start, const Vector3& end, Vector3& outPos, Vector3& outNormal) const
{
	//当たり判定の床を探す
	const auto colHit = MV1CollCheck_Line(m_stageColHandle, -1, start.ToDxLibVector(), end.ToDxLibVector());
	if (!colHit.HitFlag || colHit.Normal.y < kFloorNormalMinY)return false;

	//当たり判定と見た目で床の高さが違うので、見た目の床に貼る
	const Vector3 colPos = Vector3::FromDxLibVector(colHit.HitPosition);
	const Vector3 offset = kUp * kViewRayRange;
	const auto viewHit = MV1CollCheck_Line(m_stageViewHandle, -1, (colPos + offset).ToDxLibVector(), (colPos - offset).ToDxLibVector());
	if (!viewHit.HitFlag || viewHit.Normal.y < kFloorNormalMinY)return false;
	//見つけた床の位置と向き
	outPos = Vector3::FromDxLibVector(viewHit.HitPosition);
	outNormal = Vector3::FromDxLibVector(viewHit.Normal).Normalize();
	return true;
}

bool BloodDecalManager::CanFit(const Decal& decal, float halfSize) const
{
	const float cornerX = halfSize * kFitCornerRate;//調べる横の位置
	const float cornerZ = cornerX * decal.stretch;//調べる縦の位置
	const Vector3 offset = decal.normal * kFitRayRange;//線の長さ
	//四隅を順に調べる
	for (int sideX = -1; sideX <= 1; sideX += 2)
	{
		for (int sideZ = -1; sideZ <= 1; sideZ += 2)
		{
			const Vector3 pos = decal.center + decal.right * (sideX * cornerX) + decal.forward * (sideZ * cornerZ);
			//床が無ければはみ出している
			if (!MV1CollCheck_Line(m_stageViewHandle, -1, (pos + offset).ToDxLibVector(), (pos - offset).ToDxLibVector()).HitFlag)
			{
				return false;
			}
		}
	}
	return true;
}

void BloodDecalManager::AddSplats(const Vector3& ground, float baseAngle, float fallDelay, const SplatParam& param)
{
	const Vector3 above = ground + kUp * kRayUp;//線を飛ばす高さ
	for (int i = 0; i < param.num; i++)
	{
		//飛ぶ向きをばらつかせる
		const float angle = baseAngle + RandRange(-param.spreadAngle, param.spreadAngle);
		const Vector3 flatDir(sinf(angle), 0.0f, cosf(angle));
		//飛ぶ距離と着地点
		const float dist = RandRange(param.distMin, param.distMax);
		const Vector3 target = above + flatDir * dist;

		//壁の向こうには出さない
		if (MV1CollCheck_Line(m_stageColHandle, -1, above.ToDxLibVector(), target.ToDxLibVector()).HitFlag)continue;

		//着地点の床を探す
		Decal splat;
		if (!FindFloor(target, target - kUp * (kRayUp + kSplatRayDown), splat.center, splat.normal))continue;
		const float distRate = (dist - param.distMin) / (param.distMax - param.distMin);//距離の割合
		splat.halfSize = RandRange(param.sizeMin, param.sizeMax);
		splat.stretch = 1.0f + (param.stretchMax - 1.0f) * distRate;//遠いほど細長い
		splat.delay = fallDelay + dist / kFlySpeed;//遠いほど遅れて着く
		splat.growFrame = kSplatGrowFrame;
		splat.cellX = GetRand(kCellNum - 1);//形はランダム
		splat.cellY = kSplatCellY;
		AddDecal(splat, flatDir);
	}
}

void BloodDecalManager::AddDecal(Decal decal, const Vector3& dir)
{
	//斜面に沿わせる
	decal.right = decal.normal.Cross(dir).Normalize();
	decal.forward = decal.right.Cross(decal.normal).Normalize();

	float halfSize = decal.halfSize;
	//小さい粒は調べない
	if (halfSize * decal.stretch >= kFitSkipSize)
	{
		bool isFit = false;
		//はみ出すなら縮めて試す
		for (int i = 0; i < kFitTryNum; i++)
		{
			if (CanFit(decal, halfSize))
			{
				isFit = true;
				break;
			}
			halfSize *= kFitScaleRate;
		}
		//どうしても入らないなら出さない
		if (!isFit)return;
	}

	//メモ帳に書く
	decal.halfSize = halfSize;
	decal.age = 0.0f;
	decal.isActive = true;
	m_decals[m_nextIndex] = decal;
	//次の行へ(最後まで行ったら先頭に戻る)
	m_nextIndex = (m_nextIndex + 1) % kMaxDecalNum;
}

float BloodDecalManager::RandRange(float min, float max) const
{
	constexpr int kRandStep = 1000;//乱数の細かさ
	return min + (max - min) * (static_cast<float>(GetRand(kRandStep)) / kRandStep);
}
