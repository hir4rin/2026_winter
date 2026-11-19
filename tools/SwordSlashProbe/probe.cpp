// 攻撃のアニメで刀の先の動きを測って、風切りのエフェクトの向き(roll)と出すフレームの目安を出す
// probe.exe プレイヤー.mv1 刀.mv1 ComboChain01.csv ログ
#include "DxLib.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static const float kPivotHeight = 110.0f;//PlayerStateAttack.cpp の kSlashHeight と同じ
static const float kSweepFrames = 6.0f;//出すフレームから何フレーム前までを振りとみなすか
static const float kLeadAngle = 75.0f * DX_PI_F / 180.0f;//弧の真ん中から刃先側の端まで(エフェクトの作り)

static std::vector<std::string> Split(const std::string& s, char c)
{
	std::vector<std::string> out; std::string t; std::istringstream ss(s);
	while (std::getline(ss, t, c)) out.push_back(t);
	return out;
}

static VECTOR Cross(VECTOR a, VECTOR b) { return VGet(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static VECTOR Rot(VECTOR v, VECTOR k, float a) { return VAdd(VAdd(VScale(v, cosf(a)), VScale(Cross(k, v), sinf(a))), VScale(k, VDot(k, v) * (1.0f - cosf(a)))); }
static float SignedAngle(VECTOR from, VECTOR to, VECTOR axis) { return atan2f(VDot(Cross(from, to), axis), VDot(from, to)) * 180.0f / DX_PI_F; }

// PlayerWeapon::Update と同じ行列
static MATRIX WeaponMatrix(int player, int hand)
{
	const float d = DX_PI_F / 180.0f;
	MATRIX rot = MGetRotX(90.0f * d);
	rot = MMult(rot, MGetRotY(90.0f * d + DX_PI_F / 2));
	rot = MMult(rot, MGetRotZ(0.0f - DX_PI_F / 2));
	MATRIX s = MGetScale(VGet(0.6f, 0.6f, 0.6f));
	return MMult(MMult(s, rot), MV1GetFrameLocalWorldMatrix(player, hand));
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* fp = nullptr; fopen_s(&fp, __argv[4], "w");
	const int pl = MV1LoadModel(__argv[1]), wp = MV1LoadModel(__argv[2]);
	if (pl < 0 || wp < 0) { fprintf(fp, "load fail\n"); fclose(fp); return 2; }

	// 刀の長い軸と先の位置(刀のモデルの空間)//PlayerWeapon::CalcTipLocal と同じ
	VECTOR mn = VGet(1e9f, 1e9f, 1e9f), mx = VGet(-1e9f, -1e9f, -1e9f);
	MV1SetMatrix(wp, MGetIdent());
	for (int f = 0; f < MV1GetFrameNum(wp); ++f)
	{
		if (MV1GetFrameMeshNum(wp, f) == 0) continue;
		VECTOR a = MV1GetFrameMinVertexLocalPosition(wp, f), b = MV1GetFrameMaxVertexLocalPosition(wp, f);
		MATRIX m = MV1GetFrameLocalWorldMatrix(wp, f);
		for (int c = 0; c < 8; ++c)
		{
			VECTOR p = VTransform(VGet((c & 1) ? b.x : a.x, (c & 2) ? b.y : a.y, (c & 4) ? b.z : a.z), m);
			mn = VGet(fminf(mn.x, p.x), fminf(mn.y, p.y), fminf(mn.z, p.z));
			mx = VGet(fmaxf(mx.x, p.x), fmaxf(mx.y, p.y), fmaxf(mx.z, p.z));
		}
	}
	float ext[3] = { mx.x - mn.x, mx.y - mn.y, mx.z - mn.z };
	int ax = 0; for (int i = 1; i < 3; ++i) if (ext[i] > ext[ax]) ax = i;
	float lo = (&mn.x)[ax], hi = (&mx.x)[ax];
	VECTOR tipLocal = VGet(0, 0, 0); (&tipLocal.x)[ax] = (fabsf(hi) > fabsf(lo)) ? hi : lo;
	fprintf(fp, "weapon min(%.1f,%.1f,%.1f) max(%.1f,%.1f,%.1f) axis %d tip %.1f\n", mn.x, mn.y, mn.z, mx.x, mx.y, mx.z, ax, (&tipLocal.x)[ax]);

	const int hand = MV1SearchFrame(pl, "hand_r"), chest = MV1SearchFrame(pl, "spine_03");
	MV1SetMatrix(pl, MGetIdent());
	const VECTOR fwd = VGet(0, 0, -1), up = VGet(0, 1, 0);

	std::ifstream csv(__argv[3]);
	std::string line;
	while (std::getline(csv, line))
	{
		if (line.empty() || line[0] == '#') continue;
		auto t = Split(line, ',');
		if (t.size() < 25) continue;
		const std::string anim = t[1];
		const int idx = MV1GetAnimIndex(pl, anim.c_str());
		if (idx < 0) { fprintf(fp, "[%s] anim %s not found\n", t[3].c_str(), anim.c_str()); continue; }
		auto starts = Split(t[23], ';'), ends = Split(t[24], ';');
		const int at = MV1AttachAnim(pl, idx, -1, FALSE);
		const float total = MV1GetAttachAnimTotalTime(pl, at);
		for (size_t w = 0; w < starts.size() && w < ends.size(); ++w)
		{
			const float f0 = std::stof(starts[w]), f1 = (std::min)(std::stof(ends[w]), total);
			// 区間の前後も少し見る
			const float step = 0.5f;
			std::vector<float> fr; std::vector<VECTOR> tip; std::vector<VECTOR> ch;
			for (float f = (std::max)(0.0f, f0 - 4.0f); f <= (std::min)(total, f1 + 2.0f); f += step)
			{
				MV1SetAttachAnimTime(pl, at, f);
				MV1RefreshCollInfo(pl, -1);
				const MATRIX m = WeaponMatrix(pl, hand);
				fr.push_back(f); tip.push_back(VTransform(tipLocal, m)); ch.push_back(MV1GetFramePosition(pl, chest));
			}
			if (tip.size() < 3) continue;
			// 一番速いフレーム
			int peak = 1; float vmax = 0;
			std::vector<float> sp(tip.size(), 0.0f);
			for (size_t i = 1; i < tip.size(); ++i) { sp[i] = VSize(VSub(tip[i], tip[i - 1])) / step; if (sp[i] > vmax) { vmax = sp[i]; peak = (int)i; } }
			// 速い区間(最大の35%以上)
			int s = peak, e = peak;
			while (s > 1 && sp[s - 1] > vmax * 0.35f) --s;
			while (e + 1 < (int)tip.size() && sp[e + 1] > vmax * 0.35f) ++e;
			const VECTOR c = ch[peak];
			const VECTOR a = VSub(tip[s - 1], c), b = VSub(tip[e], c), mid = VSub(tip[peak], c);
			VECTOR n = Cross(a, b); const float nl = VSize(n); if (nl > 1e-4f) n = VScale(n, 1.0f / nl);
			const float sweep = acosf(fmaxf(-1.0f, fminf(1.0f, VDot(VNorm(a), VNorm(b))))) * 180.0f / DX_PI_F;
			// roll: 前(-Z)を軸に、上からどれだけ回したら n になるか
			VECTOR np = VSub(n, VScale(fwd, VDot(n, fwd)));
			const float roll = atan2f(VDot(np, Cross(fwd, up)), VDot(np, up)) * 180.0f / DX_PI_F;
			// 弧の真ん中が前からどれだけずれているか(上から見た角度)
			const float yaw = atan2f(mid.x, -mid.z) * 180.0f / DX_PI_F;
			fprintf(fp, "[%s] %s win%zu %.1f-%.1f : fast %.1f-%.1f peak %.1f v %.1f sweep %.0f roll %.0f n(%.2f,%.2f,%.2f) tipDist %.0f chestY %.0f midYaw %.0f midY %.0f\n",
				t[3].c_str(), anim.c_str(), w, f0, f1, fr[s - 1], fr[e], fr[peak], vmax, sweep, roll, n.x, n.y, n.z, VSize(mid), c.y, yaw, mid.y);
		}

		// 風切りを出すフレームごとに、ゲームの PlayerSlashEffect と同じ決まりで yaw / roll / spin を出す
		// 弧の中心は足元+110、真ん中から刃先側の端まで75°、刃先側の端=そのフレームの刀の先
		if (t.size() > 31 && !t[31].empty())
		{
			const VECTOR pivot = VGet(0, kPivotHeight, 0);
			std::string yaws, rolls, spins, info;
			for (const auto& sfText : Split(t[31], ';'))
			{
				const float sf = std::stof(sfText);
				std::vector<VECTOR> tp;
				for (float f = (std::max)(0.0f, sf - kSweepFrames); f <= sf + 0.001f; f += 0.5f)
				{
					MV1SetAttachAnimTime(pl, at, (std::min)(f, total));
					MV1RefreshCollInfo(pl, -1);
					tp.push_back(VSub(VTransform(tipLocal, WeaponMatrix(pl, hand)), pivot));
				}
				// 速い所だけ(最大の35%以上)を今からさかのぼる
				std::vector<float> sp(tp.size(), 0.0f); float vmax = 0;
				for (size_t i = 1; i < tp.size(); ++i) { sp[i] = VSize(VSub(tp[i], tp[i - 1])); vmax = fmaxf(vmax, sp[i]); }
				int s = (int)tp.size() - 1;
				while (s > 0 && sp[s] > vmax * 0.35f) --s;
				VECTOR n = VGet(0, 0, 0);
				for (int i = s; i + 1 < (int)tp.size(); ++i) n = VAdd(n, Cross(tp[i], tp[i + 1]));
				if (VSize(n) < 1e-4f) n = up; else n = VNorm(n);
				const VECTOR last = tp.back();
				VECTOR tipDir = VNorm(VSub(last, VScale(n, VDot(last, n))));
				const VECTOR center = Rot(tipDir, n, -kLeadAngle);
				// 前(-Z)を上のまわりに yaw 回して、n に垂直な水平の向きにする
				VECTOR fw = Cross(n, up);
				float yawDeg = 0, rollDeg = 0;
				if (VSize(fw) < 0.05f) { fw = fwd; rollDeg = (n.y > 0) ? 0.0f : 180.0f; }
				else
				{
					fw = VNorm(fw);
					if (VDot(fw, fwd) < 0) fw = VScale(fw, -1.0f);
					yawDeg = SignedAngle(fwd, fw, up);
					rollDeg = SignedAngle(up, n, fw);
				}
				const float spinDeg = SignedAngle(fw, center, n);
				auto add = [](std::string& str, float v) { char b[32]; sprintf_s(b, "%s%.0f", str.empty() ? "" : ";", v); str += b; };
				add(yaws, yawDeg); add(rolls, rollDeg); add(spins, spinDeg);
				char b[128]; sprintf_s(b, " [f%.1f dist %.0f sweepStart %.1f]", sf, VSize(last), (std::max)(0.0f, sf - kSweepFrames) + s * 0.5f); info += b;
			}
			fprintf(fp, "SLASH [%s] yaw %s roll %s spin %s%s\n", t[3].c_str(), yaws.c_str(), rolls.c_str(), spins.c_str(), info.c_str());
		}
		MV1DetachAnim(pl, at);
	}
	fclose(fp);
	DxLib_End();
	return 0;
}
