# 溜め中のエフェクトの素材を作る(白い画像。色はノードで付ける)
# powershell -NoProfile -ExecutionPolicy Bypass -File gen_chargehold_tex.ps1
param([string]$OutDir = (Join-Path $PSScriptRoot "Texture"))

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class ChTex
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }

    static void Save(float[] a, float[] l, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            byte v = (byte)(C01(l == null ? 1 : l[i]) * 255);
            buf[i * 4 + 0] = v; buf[i * 4 + 1] = v; buf[i * 4 + 2] = v;
            buf[i * 4 + 3] = (byte)(C01(a[i]) * 255);
        }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    static float Edge(int px, int py, int x0, int y0, int w, int h, float m)
    {
        return C01(Math.Min(Math.Min(px - x0, x0 + w - 1 - px), Math.Min(py - y0, y0 + h - 1 - py)) / m);
    }

    // 風の筋: 横長、両端は細く消える
    public static void Streak(string path, int w, int h)
    {
        float[] A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, y = (py + 0.5f) / h * 2 - 1;
                float along = (float)Math.Pow(Math.Max(0, 1 - x * x / 0.81f), 1.2) * (0.55f + 0.45f * C01((x + 1) * 0.8f));
                float wid = 0.18f + 0.22f * C01((x + 1) / 2);
                float across = (float)Math.Exp(-y * y / (2 * wid * wid));
                A[py * w + px] = along * across * Edge(px, py, 0, 0, w, h, 3);
            }
        Save(A, null, w, h, path);
    }

    // 床の風の弧: 中心からの円弧(約120度)、先が太く尾が細い
    public static void WindArc(string path, int s)
    {
        float[] A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = 1 - (py + 0.5f) / s * 2;
                float r = (float)Math.Sqrt(x * x + y * y), th = (float)Math.Atan2(y, x);
                float u = th / 2.1f;// 0..1 が弧
                if (u < 0 || u > 1) continue;
                float wid = 0.012f + 0.05f * (float)Math.Pow(u, 1.5);
                float rr = 0.8f - 0.06f * u;
                float d = Math.Abs(r - rr);
                float a = (float)Math.Exp(-d * d / (2 * wid * wid)) * C01(u / 0.06f) * C01((1 - u) / 0.05f) * (0.3f + 0.7f * u);
                A[py * s + px] = a * Edge(px, py, 0, 0, s, s, 4);
            }
        Save(A, null, s, s, path);
    }

    // 衝撃の輪: 細い輪＋内側にかすかなにじみ
    public static void ShockRing(string path, int s)
    {
        float[] A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float r = (float)Math.Sqrt(x * x + y * y);
                float d = r - 0.86f;
                float ring = (float)Math.Exp(-d * d / (2 * 0.025f * 0.025f));
                float inner = d < 0 ? 0.25f * (float)Math.Exp(d / 0.12f) : 0;
                A[py * s + px] = Math.Max(ring, inner) * Edge(px, py, 0, 0, s, s, 4);
            }
        Save(A, null, s, s, path);
    }

    static void Bolt(List<float[]> segs, float x0, float y0, float x1, float y1, float disp, int depth, Random rnd)
    {
        if (depth == 0) { segs.Add(new float[] { x0, y0, x1, y1 }); return; }
        float mx = (x0 + x1) / 2, my = (y0 + y1) / 2;
        float o = (float)(rnd.NextDouble() * 2 - 1) * disp;
        mx += -(y1 - y0) * o; my += (x1 - x0) * o;
        Bolt(segs, x0, y0, mx, my, disp, depth - 1, rnd);
        Bolt(segs, mx, my, x1, y1, disp, depth - 1, rnd);
        if (depth == 4 && rnd.NextDouble() < 0.6)
        {
            float bl = 0.45f + (float)rnd.NextDouble() * 0.35f, ang = (float)(rnd.NextDouble() - 0.5) * 1.6f;
            float dx = x1 - x0, dy = y1 - y0;
            Bolt(segs, mx, my, mx + (dx * (float)Math.Cos(ang) - dy * (float)Math.Sin(ang)) * bl, my + (dx * (float)Math.Sin(ang) + dy * (float)Math.Cos(ang)) * bl, disp, 3, rnd);
        }
    }

    static float SegDist(float px, float py, float[] s)
    {
        float vx = s[2] - s[0], vy = s[3] - s[1];
        float t = C01(((px - s[0]) * vx + (py - s[1]) * vy) / (vx * vx + vy * vy + 1e-9f));
        float dx = px - (s[0] + vx * t), dy = py - (s[1] + vy * t);
        return (float)Math.Sqrt(dx * dx + dy * dy);
    }

    // 稲妻: 横長のコマ(cw×ch)を cols×rows。芯は白、まわりはにじみ(明るさで芯を残す)
    public static void Lightning(string path, int cw, int ch, int cols, int rows, int seed)
    {
        var rnd = new Random(seed);
        int w = cw * cols, h = ch * rows;
        float[] A = new float[w * h], L = new float[w * h];
        float asp = (float)ch / cw;
        for (int fi = 0; fi < cols * rows; fi++)
        {
            var segs = new List<float[]>();
            Bolt(segs, -0.9f, (float)(rnd.NextDouble() - 0.5) * 0.2f, 0.9f, (float)(rnd.NextDouble() - 0.5) * 0.2f, 0.22f, 7, rnd);
            int ox = (fi % cols) * cw, oy = (fi / cols) * ch;
            for (int py = 0; py < ch; py++)
                for (int px = 0; px < cw; px++)
                {
                    float x = (px + 0.5f) / cw * 2 - 1, y = (1 - (py + 0.5f) / ch * 2) * asp;
                    float d = 10;
                    foreach (var s in segs) { float sd = SegDist(x, y, s); if (sd < d) d = sd; }
                    float core = C01(1 - (d - 0.004f) / 0.006f);
                    float glow = (float)Math.Exp(-d / 0.03f) * 0.6f;
                    int idx = (oy + py) * w + ox + px;
                    A[idx] = Math.Max(core, glow) * Edge(px, py, 0, 0, cw, ch, 6);
                    L[idx] = 0.75f + 0.25f * core;
                }
        }
        Save(A, L, w, h, path);
    }

    // 岩のかけら: 2×2 の4種。上から光が当たった暗い灰色
    public static void Rocks(string path, int cell, int seed)
    {
        var rnd = new Random(seed);
        int w = cell * 2;
        float[] A = new float[w * w], L = new float[w * w];
        for (int fi = 0; fi < 4; fi++)
        {
            float[] amp = new float[6], ph = new float[6];
            for (int i = 0; i < 6; i++) { amp[i] = (float)rnd.NextDouble() * 0.16f / (i + 1); ph[i] = (float)(rnd.NextDouble() * 6.28); }
            float sx = 0.8f + (float)rnd.NextDouble() * 0.4f;
            int ox = (fi % 2) * cell, oy = (fi / 2) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = ((px + 0.5f) / cell * 2 - 1) / sx, y = 1 - (py + 0.5f) / cell * 2;
                    float th = (float)Math.Atan2(y, x), r = (float)Math.Sqrt(x * x + y * y);
                    float rad = 0.62f;
                    for (int i = 0; i < 6; i++) rad += amp[i] * (float)Math.Sin((i + 2) * th + ph[i]);
                    float f = rad - r;
                    // 面: 角度を5段に分けて明るさを変える
                    int facet = (int)Math.Floor((th + 3.1416f) / 6.2832f * 5);
                    float shade = 0.28f + 0.1f * ((facet * 37 + fi * 11) % 5) / 4f + 0.35f * C01(y * 0.8f + 0.3f);
                    shade *= 0.5f + 0.5f * C01(f / 0.1f);
                    int idx = (oy + py) * w + ox + px;
                    A[idx] = C01(f / (2.0f / cell) + 0.5f) * Edge(px, py, 0, 0, cell, cell, 2);
                    L[idx] = shade;
                }
        }
        Save(A, L, w, w, path);
    }

    // 炎の舌: 縦長の涙形、下が丸く上がとがる。中はやや抜く
    public static void Flame(string path, int w, int h)
    {
        float[] A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, y = 1 - (py + 0.5f) / h * 2;// y: -1 下 .. 1 上
                float t = (y + 0.85f) / 1.7f;
                float half = t < 0 ? 0 : (float)(0.75 * Math.Pow(Math.Sin(Math.PI * Math.Pow(C01(t), 0.55)), 1.1) * (1 - 0.15 * t));
                float d = half - Math.Abs(x + 0.12f * (float)Math.Sin(t * 5.0) * t);
                float a = C01(d / 0.25f) * C01(1 - t * 0.35f);
                A[py * w + px] = a * Edge(px, py, 0, 0, w, h, 4);
            }
        Save(A, null, w, h, path);
    }

    // 気の筋: 縦の細いうねる線、4種を横に並べる
    public static void Wisp(string path, int cw, int ch, int seed)
    {
        var rnd = new Random(seed);
        int w = cw * 4;
        float[] A = new float[w * ch], L = new float[w * ch];
        for (int fi = 0; fi < 4; fi++)
        {
            float f1 = 1.5f + (float)rnd.NextDouble() * 1.5f, f2 = 3 + (float)rnd.NextDouble() * 3, p1 = (float)rnd.NextDouble() * 6, p2 = (float)rnd.NextDouble() * 6;
            float lean = ((float)rnd.NextDouble() - 0.5f) * 0.5f;
            int ox = fi * cw;
            for (int py = 0; py < ch; py++)
                for (int px = 0; px < cw; px++)
                {
                    float x = (px + 0.5f) / cw * 2 - 1, t = 1 - (py + 0.5f) / ch;// t: 0 下 .. 1 上
                    float cx = (0.28f * (float)Math.Sin(f1 * t * 3.1416f + p1) + 0.1f * (float)Math.Sin(f2 * t * 3.1416f + p2)) * t + lean * t * t;
                    float wid = 0.03f + 0.05f * (1 - t);
                    float d = Math.Abs(x - cx);
                    float core = C01(1 - d / wid);
                    float glow = (float)Math.Exp(-d / 0.1f) * 0.35f;
                    float fade = C01(t / 0.08f) * C01((1 - t) / 0.35f);
                    int idx = py * w + ox + px;
                    A[idx] = Math.Max(core, glow) * fade * Edge(px, py, 0, 0, cw, ch, 4);
                    L[idx] = 0.7f + 0.3f * core;
                }
        }
        Save(A, L, w, ch, path);
    }

    // ひび: 中心から8本、枝分かれ。黒く使う(Blend)・赤く使う(Add)の両方
    public static void Crack(string path, int s, int seed)
    {
        var rnd = new Random(seed);
        var segs = new List<float[]>();
        for (int i = 0; i < 9; i++)
        {
            float ang = i / 9f * 6.2832f + (float)(rnd.NextDouble() - 0.5) * 0.5f;
            float x = 0, y = 0, len = 0.55f + (float)rnd.NextDouble() * 0.33f;
            int n = 9;
            for (int j = 0; j < n; j++)
            {
                ang += (float)(rnd.NextDouble() - 0.5) * 0.8f;
                float st = len / n, nx = x + (float)Math.Cos(ang) * st, ny = y + (float)Math.Sin(ang) * st;
                segs.Add(new float[] { x, y, nx, ny, 1 - (float)j / n });
                if (j == 3 || j == 6)
                {
                    float ba = ang + (rnd.NextDouble() < 0.5 ? 0.7f : -0.7f), bx = nx, by = ny;
                    for (int k = 0; k < 3; k++)
                    {
                        float bnx = bx + (float)Math.Cos(ba) * st * 0.8f, bny = by + (float)Math.Sin(ba) * st * 0.8f;
                        segs.Add(new float[] { bx, by, bnx, bny, 0.5f * (1 - (float)k / 3) });
                        bx = bnx; by = bny; ba += (float)(rnd.NextDouble() - 0.5) * 0.8f;
                    }
                }
                x = nx; y = ny;
            }
        }
        float[] A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float a = 0;
                foreach (var g in segs)
                {
                    float d = SegDist(x, y, g), wid = 0.003f + 0.01f * g[4];
                    a = Math.Max(a, C01(1 - (d - wid) / 0.004f));
                }
                A[py * s + px] = a * Edge(px, py, 0, 0, s, s, 4);
            }
        Save(A, null, s, s, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[ChTex]::Streak((Join-Path $OutDir "CH_Streak.png"), 256, 32)
[ChTex]::WindArc((Join-Path $OutDir "CH_WindArc.png"), 512)
[ChTex]::ShockRing((Join-Path $OutDir "CH_ShockRing.png"), 512)
[ChTex]::Lightning((Join-Path $OutDir "CH_Lightning.png"), 256, 64, 2, 4, 7)
[ChTex]::Rocks((Join-Path $OutDir "CH_Rocks.png"), 128, 3)
[ChTex]::Flame((Join-Path $OutDir "CH_Flame.png"), 128, 256)
[ChTex]::Wisp((Join-Path $OutDir "CH_Wisp.png"), 64, 256, 5)
[ChTex]::Crack((Join-Path $OutDir "CH_Crack.png"), 512, 11)
"done"
