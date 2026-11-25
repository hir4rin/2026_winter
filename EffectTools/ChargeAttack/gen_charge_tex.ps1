# 溜め攻撃のエフェクト(A/B/C/E)の画像を作る
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File gen_charge_tex.ps1 [出力フォルダ]
# どれも白で、形はアルファ。色はノードで付ける
# CA_Enso.png     : 足元の筆の円相
# CA_EnsoRamp.png : 円相を描いていく順番(書き始め1 → 書き終わり0.45)。AlphaCutoff で使う
# CA_InkArc.png   : 輪の墨の帯(u=振りの始め0→先1、v=幅)。尾はかすれる
# CA_SmokeArc.png : 輪の瘴気の帯(煙のむら)
# CA_EdgeLine.png : 細い光の線(輪のふち・一閃の線)
# CA_Crack.png    : 裂け目(真ん中が太いレンズ形)
# CA_Smoke.png    : 煙の粒
# CA_Glow.png     : 光の粒
# CA_Star.png     : 十字の光
# CA_Drop.png     : 墨のしずく
param([string]$OutDir = (Join-Path $PSScriptRoot "Texture"))

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class CaTex
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Smooth(float a, float b, float x) { float t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    static void Save(float[] a, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            buf[i * 4 + 0] = 255; buf[i * 4 + 1] = 255; buf[i * 4 + 2] = 255;
            buf[i * 4 + 3] = (byte)(C01(a[i]) * 255);
        }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    // 値ノイズ(横はタイルする)
    static float Hash(int x, int y, int s) { unchecked { int h = x * 374761393 + y * 668265263 + s * 982451653; h = (h ^ (h >> 13)) * 1274126177; return ((h ^ (h >> 16)) & 0xffff) / 65535f; } }
    static float VNoise(float x, float y, int period, int s)
    {
        int x0 = (int)Math.Floor(x), y0 = (int)Math.Floor(y);
        float fx = x - x0, fy = y - y0;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
        int xa = ((x0 % period) + period) % period, xb = (((x0 + 1) % period) + period) % period;
        float a = Hash(xa, y0, s), b = Hash(xb, y0, s), c = Hash(xa, y0 + 1, s), d = Hash(xb, y0 + 1, s);
        return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
    }
    static float Fbm(float x, float y, int period, int s)
    {
        float v = 0, amp = 0.5f, f = 1;
        for (int o = 0; o < 5; o++) { v += VNoise(x * f, y * f, period * (int)f, s + o) * amp; amp *= 0.5f; f *= 2; }
        return v;
    }

    static float Bristle(float v, float s, int seed)
    {
        double n = 0;
        n += Math.Sin(v * 23.0 + seed * 1.3 + s * 2.0) * 0.45;
        n += Math.Sin(v * 47.0 + seed * 2.7 - s * 3.0) * 0.35;
        n += Math.Sin(v * 89.0 + seed * 0.9 + s * 5.0) * 0.20;
        return (float)(n * 0.5 + 0.5);
    }

    static float BrushAlpha(float s, float v, float width, int seed, float dryStart)
    {
        float d = Math.Abs(v - 0.5f) * 2f / Math.Max(width, 0.001f);
        if (d >= 1.05f) return 0;
        float edge = 1f - Smooth(0.82f, 1.05f, d);
        float br = Bristle(v, s, seed);
        float a = edge * (0.8f + 0.2f * br);
        float dry = Smooth(dryStart, 1.0f, s);
        if (dry > 0) a *= Smooth(dry * 0.8f - 0.08f, dry * 0.8f + 0.08f, br);
        return a;
    }

    public static void Enso(string path, string rampPath, int n)
    {
        float[] a = new float[n * n], ra = new float[n * n];
        double a0 = -2.2, sweep = Math.PI * 2 * 0.93;
        float R = 0.80f, maxW = 0.17f, minW = 0.03f;
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
                if (s > 1) { ra[i] = 0; continue; }
                float w = (float)Math.Pow(1 - s, 1.6) * (maxW - minW) + minW;
                float jit = (float)(Math.Sin(s * 40) * 0.004 + Math.Sin(s * 97) * 0.002);
                float v = (dist - (R + jit)) / (maxW * 1.1f) + 0.5f;
                float al = BrushAlpha(s, v, w / (maxW * 1.1f), 3, 0.5f);
                if (s > 0.88f) al *= 1 - Smooth(0.88f, 1.0f, s);
                if (s < 0.015f) al *= Smooth(0f, 0.015f, s);
                a[i] = al;
                ra[i] = 1f - 0.55f * s;
            }
        Save(a, n, n, path);
        Save(ra, n, n, rampPath);
    }

    // u=0 尾(振り始め) 1=先。尾はかすれ、先は太く丸い
    public static void InkArc(string path, int w, int h)
    {
        float[] a = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
                float q = 1 - u;
                float cap = (float)Math.Sqrt(C01(q / 0.04f) * (2 - C01(q / 0.04f)));
                float width = 0.95f * cap * (1 - q * 0.45f);
                float rag = (Fbm(u * 24, v * 3, 24, 5) - 0.5f) * 0.25f;
                float al = BrushAlpha(q, v + rag * 0.4f, width, 11, 0.45f);
                al *= 1 - Smooth(0.9f, 1.0f, q);
                a[y * w + x] = al;
            }
        Save(a, w, h, path);
    }

    public static void SmokeArc(string path, int w, int h)
    {
        float[] a = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
                float n = Fbm(u * 10, v * 2.5f, 10, 21);
                float band = 1 - Smooth(0.25f, 0.5f, Math.Abs(v - 0.5f) + (n - 0.5f) * 0.35f);
                float along = Smooth(0.0f, 0.5f, u) * (1 - Smooth(0.96f, 1.0f, u));
                a[y * w + x] = band * along * (0.55f + 0.6f * n);
            }
        Save(a, w, h, path);
    }

    public static void EdgeLine(string path, int w, int h)
    {
        float[] a = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
                float d = Math.Abs(v - 0.5f) * 2;
                float core = 1 - Smooth(0.0f, 0.18f, d);
                float halo = (float)Math.Exp(-d * 5) * 0.45f * (1 - Smooth(0.85f, 1.0f, d));
                float along = Smooth(0.0f, 0.6f, u) * (1 - Smooth(0.97f, 1.0f, u));
                a[y * w + x] = Math.Max(core, halo) * along;
            }
        Save(a, w, h, path);
    }

    public static void Crack(string path, int w, int h)
    {
        float[] a = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
                float half = (float)Math.Pow(Math.Sin(Math.PI * u), 0.7) * 0.5f + 0.01f;
                float jag = (Fbm(u * 40, 0.5f, 40, 9) - 0.5f) * 0.12f;
                float d = Math.Abs(v - 0.5f + jag) / half;
                a[y * w + x] = (1 - Smooth(0.6f, 1.0f, d));
            }
        Save(a, w, h, path);
    }

    public static void Smoke(string path, int n)
    {
        float[] a = new float[n * n];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float px = (x + 0.5f) / n * 2 - 1, py = (y + 0.5f) / n * 2 - 1;
                float d = (float)Math.Sqrt(px * px + py * py);
                float nz = Fbm((x + 0.5f) / n * 4, (y + 0.5f) / n * 4, 4, 33);
                a[y * n + x] = (1 - Smooth(0.25f, 1.0f, d + (nz - 0.5f) * 0.6f)) * (0.5f + 0.7f * nz);
            }
        Save(a, n, n, path);
    }

    public static void Glow(string path, int n)
    {
        float[] a = new float[n * n];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float px = (x + 0.5f) / n * 2 - 1, py = (y + 0.5f) / n * 2 - 1;
                float d = (float)Math.Sqrt(px * px + py * py);
                float core = 1 - Smooth(0.0f, 0.2f, d);
                float halo = (float)Math.Exp(-d * 4.5) * (1 - Smooth(0.8f, 1.0f, d));
                a[y * n + x] = Math.Max(core, halo * 0.8f);
            }
        Save(a, n, n, path);
    }

    public static void Star(string path, int n)
    {
        float[] a = new float[n * n];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float px = Math.Abs((x + 0.5f) / n * 2 - 1), py = Math.Abs((y + 0.5f) / n * 2 - 1);
                float r1 = Math.Max(0, 1 - px) * (float)Math.Exp(-py * 60 / Math.Max(1 - px, 0.05f));
                float r2 = Math.Max(0, 1 - py) * (float)Math.Exp(-px * 60 / Math.Max(1 - py, 0.05f));
                float d = (float)Math.Sqrt(px * px + py * py);
                float core = (float)Math.Exp(-d * 9);
                a[y * n + x] = Math.Min(1, r1 + r2 + core);
            }
        Save(a, n, n, path);
    }

    public static void Drop(string path, int n)
    {
        float[] a = new float[n * n];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float px = (x + 0.5f) / n * 2 - 1, py = (y + 0.5f) / n * 2 - 1;
                float nz = Fbm((x + 0.5f) / n * 3, (y + 0.5f) / n * 3, 3, 47);
                float d = (float)Math.Sqrt(px * px + py * py) + (nz - 0.5f) * 0.35f;
                a[y * n + x] = 1 - Smooth(0.55f, 0.7f, d);
            }
        Save(a, n, n, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[CaTex]::Enso((Join-Path $OutDir "CA_Enso.png"), (Join-Path $OutDir "CA_EnsoRamp.png"), 512)
[CaTex]::InkArc((Join-Path $OutDir "CA_InkArc.png"), 512, 128)
[CaTex]::SmokeArc((Join-Path $OutDir "CA_SmokeArc.png"), 512, 128)
[CaTex]::EdgeLine((Join-Path $OutDir "CA_EdgeLine.png"), 512, 64)
[CaTex]::Crack((Join-Path $OutDir "CA_Crack.png"), 512, 64)
[CaTex]::Smoke((Join-Path $OutDir "CA_Smoke.png"), 128)
[CaTex]::Glow((Join-Path $OutDir "CA_Glow.png"), 128)
[CaTex]::Star((Join-Path $OutDir "CA_Star.png"), 256)
[CaTex]::Drop((Join-Path $OutDir "CA_Drop.png"), 64)
Write-Host "done: $OutDir"
