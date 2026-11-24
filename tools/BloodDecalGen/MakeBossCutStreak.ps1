# ボスのトドメの血殺の線を作る(NINJA GAIDEN 4 の軍将の血殺が参考)
#   boss_cut_streak.png : ボスが切れる線に沿わせる、細い筆の線を2〜3本束ねたもの(画面の上から下まで突き抜けさせて使う)
#   boss_edge_streaks.png : 画面の右端に並べる、かすれた縦の線の束
# どちらも色は全部白で、形はアルファだけ
# このファイルはBOM付きUTF-8で保存すること
param(
    [string]$OutDir = (Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\bloodUI")
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class BossCutStreakGen
{
    static float Smooth(float edge0, float edge1, float x)
    {
        float t = Math.Max(0f, Math.Min(1f, (x - edge0) / (edge1 - edge0)));
        return t * t * (3f - 2f * t);
    }

    // 1次元のなめらかなノイズ(0〜1)
    static float[] Noise1(Random rnd, int length, int cell)
    {
        int n = length / cell + 2;
        var grid = new float[n];
        for (int i = 0; i < n; i++) grid[i] = (float)rnd.NextDouble();
        var result = new float[length];
        for (int x = 0; x < length; x++)
        {
            float f = (float)x / cell;
            int i = (int)f;
            float t = f - i;
            t = t * t * (3 - 2 * t);
            result[x] = grid[i] * (1 - t) + grid[i + 1] * t;
        }
        return result;
    }

    // 筆の線1本: along(線の長さの向き)ごとに中心と太さを持ち、かすれで途切れる
    class Stroke
    {
        public float[] center;  // 横の位置(px)
        public float[] half;    // 太さの半分(px)、0なら途切れ
        public float[] rough;   // ふちのがたつき
        public float[] dry;     // かすれ具合(0〜1、大きいほど毛の跡が抜ける)
        public float[] bristle; // 毛ごとの絵の具の量(線の幅方向)
    }

    const int BristleNum = 18;

    static Stroke MakeStroke(Random rnd, int length, float baseCenter, float baseHalf, float waveAmp, float gapRate, float startU, float endU, float fray, float dryRate = 0.55f)
    {
        var s = new Stroke { center = new float[length], half = new float[length], rough = new float[length], dry = new float[length], bristle = new float[BristleNum] };
        var wave = Noise1(rnd, length, 320);
        var wave2 = Noise1(rnd, length, 90);
        var width = Noise1(rnd, length, 140);
        var gap = Noise1(rnd, length, 110);
        var dry = Noise1(rnd, length, 60);
        var rough = Noise1(rnd, length, 6);
        for (int i = 0; i < BristleNum; i++) s.bristle[i] = (float)rnd.NextDouble();
        for (int x = 0; x < length; x++)
        {
            float u = (float)x / (length - 1);
            s.center[x] = baseCenter + (wave[x] - 0.5f) * waveAmp + (wave2[x] - 0.5f) * waveAmp * 0.2f;
            float h = baseHalf * (0.8f + width[x] * 0.4f);
            // 始まりと終わりは細くなって消える
            float span = Smooth(startU, startU + 0.05f, u) * (1f - Smooth(endU - 0.05f, endU, u));
            // 途切れは少しだけ(長い線のところどころ)
            float keep = Smooth(gapRate - 0.03f, gapRate + 0.03f, gap[x]);
            s.half[x] = h * (0.35f + 0.65f * span) * keep * (span > 0.02f ? 1f : 0f);
            s.rough[x] = (rough[x] - 0.5f) * fray;
            // かすれ: 端と、ところどころで毛の跡が抜ける
            s.dry[x] = Math.Min(0.85f, dry[x] * dryRate + (1f - span) * 0.6f);
        }
        return s;
    }

    static float StrokeAlpha(Stroke s, int x, float y)
    {
        float h = s.half[x];
        if (h < 0.4f) return 0f;
        float off = y - s.center[x];
        float d = Math.Abs(off);
        float edge = h + s.rough[x];
        float body = 1f - Smooth(edge - 1.0f, edge + 0.3f, d);
        if (body <= 0f) return 0f;
        // 毛の跡: 幅方向の位置で毛を選び、絵の具が足りない毛は抜ける
        float v = Math.Max(0f, Math.Min(0.999f, (off / Math.Max(edge, 0.5f) + 1f) * 0.5f));
        float b = s.bristle[(int)(v * BristleNum)];
        float keep = Smooth(s.dry[x] - 0.06f, s.dry[x] + 0.06f, b);
        return body * keep;
    }

    static void Save(string path, int w, int h, float[] alpha)
    {
        byte[] bytes = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            bytes[i * 4] = 255;
            bytes[i * 4 + 1] = 255;
            bytes[i * 4 + 2] = 255;
            bytes[i * 4 + 3] = (byte)(Math.Max(0f, Math.Min(1f, alpha[i])) * 255f + 0.5f);
        }
        using (var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb))
        {
            var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
            Marshal.Copy(bytes, 0, data.Scan0, bytes.Length);
            bmp.UnlockBits(data);
            bmp.Save(path, ImageFormat.Png);
        }
    }

    // 切れる線: 横長(+X が線の向き)。真ん中の太い1本と、そばの細い線
    const int CutW = 2048, CutH = 96;

    public static void MakeCut(string path, int seed)
    {
        Save(path, CutW, CutH, BuildCut(seed));
    }

    static float[] BuildCut(int seed)
    {
        const int W = CutW, H = CutH;
        var rnd = new Random(seed);
        float c = H * 0.5f;
        var strokes = new[]
        {
            MakeStroke(rnd, W, c, 9.0f, 10f, 0.08f, 0.0f, 1.0f, 1.6f),          // 一番太い線(端から端まで)
            MakeStroke(rnd, W, c - 15f, 3.2f, 12f, 0.22f, 0.03f, 0.82f, 1.0f),   // 細い線
            MakeStroke(rnd, W, c + 13f, 2.4f, 12f, 0.3f, 0.2f, 1.0f, 0.8f),      // もっと細い線
        };
        var alpha = new float[W * H];
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
            {
                float a = 0f;
                foreach (var s in strokes) a = Math.Max(a, StrokeAlpha(s, x, y));
                alpha[y * W + x] = a;
            }
        return alpha;
    }

    // 切れる線の画像(横長)の (along 0〜CutW, across 0〜CutH) を線形補間で読む
    static float SampleCut(float[] cut, float along, float across)
    {
        if (across < 0f || across > CutH - 1 || along < 0f || along > CutW - 1) return 0f;
        int ax = (int)along, ay = (int)across;
        int ax1 = Math.Min(ax + 1, CutW - 1), ay1 = Math.Min(ay + 1, CutH - 1);
        float tx = along - ax, ty = across - ay;
        float a = cut[ay * CutW + ax] * (1 - tx) + cut[ay * CutW + ax1] * tx;
        float b = cut[ay1 * CutW + ax] * (1 - tx) + cut[ay1 * CutW + ax1] * tx;
        return a * (1 - ty) + b * ty;
    }

    // 右端の縦の線の束を、ボスの切れる線(BuildCut)を複製して並べて作る
    // 左は細く間が空き、右ほど太く詰まる。どの線も下へいくほど不規則に太くなる。上は画面の外へ見切れ、上が右・下が左へ少し傾ける
    public static void MakeEdgeFromCut(string path, int cutSeed, int seed)
    {
        const int W = 400, H = 1024;
        var cut = BuildCut(cutSeed);
        var rnd = new Random(seed);
        var alpha = new float[W * H];
        float slant = (float)Math.Tan(4.0 * Math.PI / 180.0);
        const int copyNum = 11;
        for (int i = 0; i < copyNum; i++)
        {
            float t = (i + 0.2f + (float)rnd.NextDouble() * 0.6f) / copyNum;
            t = (float)Math.Pow(t, 0.8);
            float cx = 40f + t * (W - 60f);
            // 線の太さ(画像の高さ96を何pxにするか)//右ほど太い
            float thickTop = 28f + 60f * t;
            float thickBottom = thickTop * (1.6f + (float)rnd.NextDouble() * 1.0f);
            float thickPow = 0.6f + (float)rnd.NextDouble() * 1.4f;
            var thickNoise = Noise1(rnd, H, 120 + rnd.Next(150));
            // 画像のどこを使うか(線ごとにずらす)・上下反転
            float alongStart = (float)rnd.NextDouble() * (CutW - H - 2);
            bool flip = rnd.NextDouble() < 0.5;
            float lineSlant = slant * (0.85f + (float)rnd.NextDouble() * 0.3f);
            for (int y = 0; y < H; y++)
            {
                float u = (float)y / (H - 1);
                float thick = (thickTop + (thickBottom - thickTop) * (float)Math.Pow(u, thickPow)) * (0.8f + 0.4f * thickNoise[y]);
                float center = cx + (H * 0.5f - y) * lineSlant;
                float along = alongStart + y;
                int x0 = Math.Max(0, (int)(center - thick * 0.5f - 2));
                int x1 = Math.Min(W - 1, (int)(center + thick * 0.5f + 2));
                for (int x = x0; x <= x1; x++)
                {
                    float across = ((x - center) / thick + 0.5f) * (CutH - 1);
                    if (flip) across = CutH - 1 - across;
                    // 原作に寄せて、細かい毛のすじはぼかして埋め、大きなかすれだけ残す(くっきりさせる)
                    // 線の長さの向きに長くぼかして、まだらを縦につなげる
                    float sum = 0f;
                    for (int dy = -3; dy <= 3; dy++)
                        for (int dx = -1; dx <= 1; dx++)
                            sum += SampleCut(cut, along + dy * 12f, across + dx * 2.2f);
                    float a = Smooth(0.34f, 0.42f, sum / 21f);
                    int k = y * W + x;
                    if (a > alpha[k]) alpha[k] = a;
                }
            }
        }
        Save(path, W, H, alpha);
    }

    // 右端の縦の線の束: 幅の広いかすれた筆を1回、上から下へまっすぐ引いた形(縦長、上から下)
    // 筆の毛のかたまりが1本ずつの線になる//線はみんな同じ筆の幅の中に並ぶので、上へいくほど全体がそろって細くなる
    // 原作に合わせて、根元(下)ほど太く、右ほど線が太く詰まる。上は画面の外へ見切れる。上が右・下が左へ少し傾ける
    const float EdgeSlantDeg = 4.0f;

    class Clump
    {
        public float v0, v1;    // 筆の幅の中の位置(0=左 1=右)
        public float startU;    // 始まり(マイナスなら画面の外から)
        public float[] ink;     // 長さ方向の墨の量
        public float[] edgeL;   // ふちの細かいがたつき(左右)
        public float[] edgeR;
        public float split;     // 太い線の中に入るかすれのすじの位置(0〜1、マイナスなら無し)
        public float[] splitInk;
        public bool isBand;     // 右の端の白い帯(墨が切れない)
        public float gapR;      // 右隣までの間(筆の幅の中の位置)
        public float[] near;    // 右隣に寄る具合(枝分かれのように、ところどころでくっつく)
        public float thickPow;  // 上から下へ太くなる具合(線ごとに違う)
        public float[] thick;   // 太さのむら(線ごとに不規則)
    }

    public static void MakeEdge(string path, int seed)
    {
        const int W = 400, H = 1024;
        var rnd = new Random(seed);
        var alpha = new float[W * H];
        float slant = (float)Math.Tan(EdgeSlantDeg * Math.PI / 180.0);

        // 筆の毛のかたまりを左から並べる//左は細く間が空き、右ほど太く詰まる
        var clumps = new System.Collections.Generic.List<Clump>();
        float v = 0f;
        while (v < 1f)
        {
            // 太さの差を大きく(細い線が多く、ときどき太い線)//右の端は白い帯のようにつながる
            float r = (float)rnd.NextDouble();
            float w = (0.009f + 0.09f * (float)Math.Pow(v, 1.1)) * (0.3f + 1.9f * r * r);
            float gap = (0.045f - 0.04f * v) * (0.4f + (float)rnd.NextDouble() * 1.2f);
            bool isBand = v > 0.86f;
            if (isBand) { w = Math.Max(w, 0.06f); gap = 0.002f + (float)rnd.NextDouble() * 0.004f; }
            var c = new Clump { v0 = v, v1 = Math.Min(1f, v + w), isBand = isBand };
            // 左の細い線は途中から始まるものもある
            c.startU = (v < 0.5f && rnd.NextDouble() < 0.35) ? 0.05f + (float)rnd.NextDouble() * 0.35f : -0.2f;
            c.ink = Noise1(rnd, H, 120 + rnd.Next(120));
            c.edgeL = Noise1(rnd, H, 40);
            c.edgeR = Noise1(rnd, H, 40);
            c.near = Noise1(rnd, H, 180 + rnd.Next(140));
            c.thickPow = 0.5f + (float)rnd.NextDouble() * 1.8f;
            c.thick = Noise1(rnd, H, 110 + rnd.Next(150));
            c.split = (w > 0.03f && rnd.NextDouble() < 0.8) ? 0.25f + (float)rnd.NextDouble() * 0.5f : -1f;
            c.splitInk = Noise1(rnd, H, 160);
            clumps.Add(c);
            v += w + gap;
        }
        for (int i = 0; i + 1 < clumps.Count; i++) clumps[i].gapR = clumps[i + 1].v0 - clumps[i].v1;

        for (int y = 0; y < H; y++)
        {
            float u = (float)y / (H - 1);
            // 筆の幅: 下ほど広い//中心は傾ける
            float brushW = 330f * (0.7f + 0.3f * (float)Math.Pow(u, 0.9));
            float right = W - 8f + (H * 0.5f - y) * slant * 0.5f;
            float left = right - brushW;
            // 上へいくほど少しかすれる
            float dry = 0.04f + 0.22f * (float)Math.Pow(1f - u, 2.0);
            for (int ci = 0; ci < clumps.Count; ci++)
            {
                var c = clumps[ci];
                // 隣の線に寄る//右隣・左隣との間をそれぞれ半分ずつ詰める(くっつくと枝分かれのように見える)
                float closeR = Smooth(0.58f, 0.82f, c.near[y]);
                float closeL = ci > 0 ? Smooth(0.58f, 0.82f, clumps[ci - 1].near[y]) : 0f;
                float shiftV = 0.5f * c.gapR * closeR - (ci > 0 ? 0.5f * clumps[ci - 1].gapR * closeL : 0f);
                // 始まりはとがる
                float grow = Math.Max(0f, Math.Min(1f, (u - c.startU) / 0.12f));
                if (grow <= 0f) continue;
                // 墨が切れるのは細い線だけ(細くなってから途切れる)//太い線は途切れず、中のかすれのすじで表す
                bool canBreak = !c.isBand && (c.v1 - c.v0) < 0.03f;
                float inkRate = canBreak ? Smooth(dry - 0.02f, dry + 0.12f, c.ink[y]) : 1f;
                if (inkRate < 0.08f) continue;
                float cx = left + ((c.v0 + c.v1) * 0.5f + shiftV) * brushW;
                // 上から下へ太くなる//太くなり方は線ごとに違い、途中で不規則に太さが変わる
                float thickRate = (0.5f + 0.9f * (float)Math.Pow(u, c.thickPow)) * (0.7f + 0.7f * c.thick[y]);
                float half = (c.v1 - c.v0) * 0.5f * brushW * (0.25f + 0.75f * grow) * inkRate * thickRate;
                half = Math.Max(half, 0.8f);
                int x0 = Math.Max(0, (int)(cx - half - 2));
                int x1 = Math.Min(W - 1, (int)(cx + half + 2));
                // ふちは細かくがたつかせる(線の向きはまっすぐのまま)
                float hl = half * (0.9f + c.edgeL[y] * 0.2f);
                float hr = half * (0.9f + c.edgeR[y] * 0.2f);
                // 太い線の中のかすれのすじ(ところどころ)
                float splitRate = c.split > 0f ? Smooth(0.42f, 0.6f, c.splitInk[y]) : 0f;
                bool isSplit = splitRate > 0.05f;
                float splitX = cx - hl + (hl + hr) * c.split;
                float splitHalf = (hl + hr) * 0.06f * splitRate;
                for (int x = x0; x <= x1; x++)
                {
                    float off = x - cx;
                    float edge = off < 0 ? hl : hr;
                    float a = 1f - Smooth(edge - 0.8f, edge + 0.5f, Math.Abs(off));
                    if (isSplit) a *= Smooth(splitHalf - 0.5f, splitHalf + 0.8f, Math.Abs(x - splitX));
                    int k = y * W + x;
                    if (a > alpha[k]) alpha[k] = a;
                }
            }
        }
        Save(path, W, H, alpha);
    }
}
"@

$dir = [IO.Path]::GetFullPath($OutDir)
[BossCutStreakGen]::MakeCut((Join-Path $dir "boss_cut_streak.png"), 6031)
[BossCutStreakGen]::MakeEdgeFromCut((Join-Path $dir "boss_edge_streaks.png"), 6031, 4127)
Write-Host "wrote $dir\boss_cut_streak.png, boss_edge_streaks.png"
