# 回復エフェクト(L 筆の螺旋)の画像を作る
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File gen_heal_tex.ps1 [出力フォルダ]
# HE_Enso.png     : 足元の筆の円相(色入り)
# HE_EnsoRamp.png : 円相を描いていく順番(アルファ。書き始め1 → 書き終わり0.45)。AlphaCutoff で使う
# HE_Brush.png    : 体を巻く筆の帯(左=尾・かすれ、右=先)
# HE_Glow.png     : 粒・光
# HE_Plus.png     : 体の上の方に出る＋の光
param([string]$OutDir = (Join-Path $PSScriptRoot "Heal_v1\Texture"))

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class HeTex
{
    // 芯は白に近い緑、外は翡翠
    static readonly float[] Core = { 0.88f, 1.0f, 0.93f };
    static readonly float[] Main = { 0.35f, 0.91f, 0.65f };

    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Smooth(float a, float b, float x) { float t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    static void Save(float[] r, float[] g, float[] b, float[] a, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            buf[i * 4 + 0] = (byte)(C01(b[i]) * 255);
            buf[i * 4 + 1] = (byte)(C01(g[i]) * 255);
            buf[i * 4 + 2] = (byte)(C01(r[i]) * 255);
            buf[i * 4 + 3] = (byte)(C01(a[i]) * 255);
        }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    // 毛の筋(v=幅方向 0..1)。値 0..1
    static float Bristle(float v, float s, int seed)
    {
        double n = 0;
        n += Math.Sin(v * 23.0 + seed * 1.3 + s * 2.0) * 0.45;
        n += Math.Sin(v * 47.0 + seed * 2.7 - s * 3.0) * 0.35;
        n += Math.Sin(v * 89.0 + seed * 0.9 + s * 5.0) * 0.20;
        return (float)(n * 0.5 + 0.5);
    }

    // 筆の1点の色(k=幅の中心からの近さ 0..1)
    static void Tint(float k, out float r, out float g, out float b)
    {
        float t = Smooth(0.25f, 0.85f, k);
        r = Main[0] + (Core[0] - Main[0]) * t;
        g = Main[1] + (Core[1] - Main[1]) * t;
        b = Main[2] + (Core[2] - Main[2]) * t;
    }

    // s=書き始めからの位置 0..1、v=幅方向 0..1 で筆のアルファ
    static float BrushAlpha(float s, float v, float width, int seed)
    {
        float d = Math.Abs(v - 0.5f) * 2f / Math.Max(width, 0.001f);   // 0=中心 1=ふち
        if (d >= 1.05f) return 0;
        float edge = 1f - Smooth(0.82f, 1.05f, d);
        float br = Bristle(v, s, seed);
        float a = edge * (0.8f + 0.2f * br);
        // かすれ: 後半ほど毛の間が抜ける
        float dry = Smooth(0.5f, 1.0f, s);
        if (dry > 0) a *= Smooth(dry * 0.8f - 0.08f, dry * 0.8f + 0.08f, br);
        return a;
    }

    public static void Enso(string path, string rampPath, int n)
    {
        float[] r = new float[n * n], g = new float[n * n], b = new float[n * n], a = new float[n * n];
        float[] rr = new float[n * n], rg = new float[n * n], rb = new float[n * n], ra = new float[n * n];
        double a0 = -2.2, sweep = Math.PI * 2 * 0.93;
        float R = 0.80f, maxW = 0.17f, minW = 0.03f;   // 半径・太さ(半分の大きさに対して)
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                int i = y * n + x;
                float px = (x + 0.5f) / n * 2 - 1, py = (y + 0.5f) / n * 2 - 1;
                float dist = (float)Math.Sqrt(px * px + py * py);
                double ang = Math.Atan2(py, px) - a0;
                while (ang < 0) ang += Math.PI * 2;
                while (ang >= Math.PI * 2) ang -= Math.PI * 2;
                float s = (float)(ang / sweep);
                rr[i] = rg[i] = rb[i] = 1;
                if (s > 1) { ra[i] = 0; continue; }
                float w = (float)Math.Pow(1 - s, 1.6) * (maxW - minW) + minW;
                float jit = (float)(Math.Sin(s * 40) * 0.004 + Math.Sin(s * 97) * 0.002);
                float v = (dist - (R + jit)) / (maxW * 1.1f) + 0.5f;   // 幅方向(太さに関係なく同じ尺度)
                float al = BrushAlpha(s, v, w / (maxW * 1.1f), 3);
                // 書き終わりは薄く消える
                if (s > 0.88f) al *= 1 - Smooth(0.88f, 1.0f, s);
                // 書き始めは丸く
                if (s < 0.015f) al *= Smooth(0f, 0.015f, s);
                float k = 1 - Math.Abs(v - 0.5f) * 2f / Math.Max(w / (maxW * 1.1f), 0.001f);
                Tint(C01(k), out r[i], out g[i], out b[i]);
                a[i] = al;
                ra[i] = 1f - 0.55f * s;
            }
        Save(r, g, b, a, n, n, path);
        Save(rr, rg, rb, ra, n, n, rampPath);
    }

    // 帯: u=長さ(0=尾 1=先)、v=幅
    public static void Brush(string path, int w, int h)
    {
        float[] r = new float[w * h], g = new float[w * h], b = new float[w * h], a = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                int i = y * w + x;
                float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
                float q = 1 - u;   // 先からの距離
                float cap = (float)Math.Sqrt(C01(q / 0.07f) * (2 - C01(q / 0.07f)));   // 先は丸く
                float width = 0.9f * cap * (1 - q * 0.7f);
                float s = q;
                float al = BrushAlpha(s, v, width, 7);
                al *= 1 - Smooth(0.85f, 1.0f, q);   // 尾は消える
                float k = 1 - Math.Abs(v - 0.5f) * 2f / width;
                Tint(C01(k), out r[i], out g[i], out b[i]);
                a[i] = al;
            }
        Save(r, g, b, a, w, h, path);
    }

    // ＋の形(角の丸い十字)とまわりの光
    public static void Plus(string path, int n)
    {
        float[] r = new float[n * n], g = new float[n * n], b = new float[n * n], a = new float[n * n];
        float hw = 0.13f, len = 0.62f;   // 腕の太さの半分・長さ(半分の大きさに対して)
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                int i = y * n + x;
                float px = Math.Abs((x + 0.5f) / n * 2 - 1), py = Math.Abs((y + 0.5f) / n * 2 - 1);
                // 角の丸い棒までの距離(横と縦の近い方)
                float d1 = (float)Math.Sqrt(Math.Pow(Math.Max(px - (len - hw), 0), 2) + Math.Pow(Math.Max(py, 0), 2)) - hw;
                float d2 = (float)Math.Sqrt(Math.Pow(Math.Max(py - (len - hw), 0), 2) + Math.Pow(Math.Max(px, 0), 2)) - hw;
                float d = Math.Min(d1, d2);
                float core = 1 - Smooth(-0.01f, 0.015f, d);
                float halo = (float)Math.Exp(-Math.Max(d, 0) * 9) * 0.55f * (1 - Smooth(0.75f, 1.0f, Math.Max(px, py)));
                a[i] = Math.Max(core, halo);
                Tint(C01(core), out r[i], out g[i], out b[i]);
            }
        Save(r, g, b, a, n, n, path);
    }

    public static void Glow(string path, int n)
    {
        float[] r = new float[n * n], g = new float[n * n], b = new float[n * n], a = new float[n * n];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                int i = y * n + x;
                float px = (x + 0.5f) / n * 2 - 1, py = (y + 0.5f) / n * 2 - 1;
                float d = (float)Math.Sqrt(px * px + py * py);
                float core = 1 - Smooth(0.0f, 0.22f, d);
                float halo = (float)Math.Exp(-d * 4.5) * (1 - Smooth(0.8f, 1.0f, d));
                a[i] = Math.Max(core, halo * 0.8f);
                Tint(C01(core * 1.2f), out r[i], out g[i], out b[i]);
            }
        Save(r, g, b, a, n, n, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[HeTex]::Enso((Join-Path $OutDir "HE_Enso.png"), (Join-Path $OutDir "HE_EnsoRamp.png"), 512)
[HeTex]::Brush((Join-Path $OutDir "HE_Brush.png"), 512, 128)
[HeTex]::Glow((Join-Path $OutDir "HE_Glow.png"), 128)
[HeTex]::Plus((Join-Path $OutDir "HE_Plus.png"), 128)
Write-Host "done: $OutDir"
