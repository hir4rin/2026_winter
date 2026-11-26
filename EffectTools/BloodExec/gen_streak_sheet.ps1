# 使い方: powershell -ExecutionPolicy Bypass -File gen_streak_sheet.ps1 <出力フォルダ>
# NG4 の絶技集(YouTube 9eDsgRaA3FY の 34.8〜35.2 秒)の液体の血をまねた画像
# UBA_StreakSheet : Ring に貼る「細い筋の束」の面(U = 弧の向き 尾0→頭1 / V = 幅)。ふちは細い糸にほつれる、くっきり
# UBA_Strand      : 尾を引く滴(2x2、1コマ 128x512。上が頭、下が尾)。移動方向ビルボード用
# どれも白ベース(明るさ=つや)。色はノードで付ける
param([string]$OutDir)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class StreakTex
{
    static float C(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float L(float a, float b, float t) { return a + (b - a) * t; }
    static float Sm(float a, float b, float x) { float t = C((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    static void Save(float[] al, float[] lum, int w, int h, string path)
    {
        Bitmap bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        BitmapData bd = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            byte v = (byte)(C(lum[i]) * 255);
            buf[i * 4 + 0] = v; buf[i * 4 + 1] = v; buf[i * 4 + 2] = v;
            buf[i * 4 + 3] = (byte)(C(al[i]) * 255);
        }
        Marshal.Copy(buf, 0, bd.Scan0, buf.Length);
        bmp.UnlockBits(bd);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    static float Gauss(Random r) { return (float)(Math.Sqrt(-2 * Math.Log(1 - r.NextDouble())) * Math.Cos(2 * Math.PI * r.NextDouble())); }

    // 1本の筋を描く(u: 0..1 横、v: 0..1 縦、太さは v の単位)
    static void Fiber(float[] al, float[] lum, int w, int h, float u0, float u1, float vc, float width, float bend, float phase, float headBulb, float gloss)
    {
        int x0 = Math.Max(0, (int)(u0 * w) - 2), x1 = Math.Min(w - 1, (int)(u1 * w) + 2);
        for (int x = x0; x <= x1; x++)
        {
            float u = (x + 0.5f) / w;
            float t = C((u - u0) / (u1 - u0));
            // 尾は細く、頭へ向かって太る。頭は丸く膨らむ
            float th = width * (0.12f + 0.88f * (float)Math.Pow(t, 0.8f));
            float bulb = headBulb * (float)Math.Exp(-Math.Pow((t - 0.9f) / 0.08f, 2));
            th *= 1f + bulb;
            // 頭の先で丸く閉じる
            float capT = C((u1 - u) / Math.Max(width * 1.6f * h / w, 0.002f));
            th *= (float)Math.Sqrt(capT);
            float cv = vc + bend * (float)Math.Sin(t * 3.0f + phase) * (1 - t * 0.5f);
            int y0 = Math.Max(0, (int)((cv - th) * h) - 2), y1 = Math.Min(h - 1, (int)((cv + th) * h) + 2);
            for (int y = y0; y <= y1; y++)
            {
                float v = (y + 0.5f) / h;
                float d = Math.Abs(v - cv) / Math.Max(th, 0.0005f);
                float a = 1f - Sm(0.75f, 1.0f, d);
                a *= Sm(0.0f, 0.04f, t);
                if (a <= 0) continue;
                int i = y * w + x;
                // 明るさ: ふちは暗め、太い筋の中心の少し上につや
                float shade = L(0.35f, 0.7f, 1f - d);
                float hl = gloss * (float)Math.Exp(-Math.Pow((v - (cv - th * 0.35f)) / Math.Max(th * 0.22f, 0.001f), 2)) * Sm(0.3f, 0.8f, t);
                float l = C(shade + hl);
                if (a > al[i]) { lum[i] = L(lum[i], l, C(a * 1.2f)); al[i] = a; }
                else lum[i] = Math.Max(lum[i] * 0.98f, l * a + lum[i] * (1 - a));
            }
        }
    }

    public static void Sheet(string path, int w, int h, int seed)
    {
        float[] al = new float[w * h], lum = new float[w * h];
        Random r = new Random(seed);
        // 細い筋をたくさん。中央に集まり、少しだけ太い束がある
        for (int k = 0; k < 90; k++)
        {
            float g = Gauss(r);
            float vc = 0.5f + g * 0.14f;
            if (vc < 0.12f || vc > 0.88f) continue;
            float thick = (float)Math.Pow(r.NextDouble(), 3);
            float width = L(0.006f, 0.05f, thick) * L(1f, 0.55f, Math.Abs(g) / 2.5f);
            float u1 = L(0.7f, 0.99f, (float)r.NextDouble()) - Math.Abs(g) * 0.08f;
            float u0 = L(0.0f, 0.45f, (float)r.NextDouble());
            float bend = (float)(r.NextDouble() - 0.5) * 0.06f;
            Fiber(al, lum, w, h, u0, u1, vc, width, bend, (float)(r.NextDouble() * 6.28), thick > 0.3 ? 0.5f : 0.15f, thick > 0.25 ? 0.45f : 0.1f);
        }
        // 尾のまわりに離れた滴
        for (int k = 0; k < 40; k++)
        {
            float vc = 0.5f + Gauss(r) * 0.2f;
            if (vc < 0.1f || vc > 0.9f) continue;
            float uc = L(0.05f, 0.8f, (float)r.NextDouble());
            float len = L(0.015f, 0.06f, (float)r.NextDouble());
            Fiber(al, lum, w, h, uc, uc + len, vc, L(0.006f, 0.016f, (float)r.NextDouble()), 0, 0, 0.4f, 0.2f);
        }
        // 端は必ず0
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
                al[y * w + x] *= Sm(0f, 0.03f, v) * Sm(1f, 0.97f, v) * Sm(1f, 0.99f, u);
            }
        Save(al, lum, w, h, path);
    }

    // 尾を引く滴(縦向き。上が頭)。2x2 のコマ
    public static void Strands(string path, int cw, int chh, int seed)
    {
        int w = cw * 2, h = chh * 2;
        float[] al = new float[w * h], lum = new float[w * h];
        Random r = new Random(seed);
        for (int c = 0; c < 4; c++)
        {
            float[] ca = new float[cw * chh], cl = new float[cw * chh];
            // 横向きで描いてから回す(Fiber は u 方向に伸びる)
            float[] ta = new float[chh * cw], tl = new float[chh * cw];
            float mainW = L(0.13f, 0.2f, (float)r.NextDouble());
            Fiber(ta, tl, chh, cw, 0.02f, 0.9f, 0.5f, mainW, L(0.12f, 0.22f, (float)r.NextDouble()) * (c % 2 == 0 ? 1 : -1), (float)(r.NextDouble() * 6.28), 0.6f, 0.5f);
            // わきの細い筋と滴
            for (int k = 0; k < 3 + c; k++)
            {
                float vc = 0.5f + (float)(r.NextDouble() - 0.5) * 0.5f;
                float u1 = L(0.3f, 0.8f, (float)r.NextDouble());
                Fiber(ta, tl, chh, cw, u1 - L(0.08f, 0.3f, (float)r.NextDouble()), u1, vc, L(0.03f, 0.07f, (float)r.NextDouble()), 0, 0, 0.5f, 0.2f);
            }
            for (int y = 0; y < chh; y++)
                for (int x = 0; x < cw; x++)
                {
                    // ta の (u=chh-1-y 方向, v=x) → 上が頭
                    int su = chh - 1 - y, sv = x;
                    float a = ta[sv * chh + su];
                    float v = (x + 0.5f) / cw, t = (y + 0.5f) / chh;
                    a *= Sm(0f, 0.04f, v) * Sm(1f, 0.96f, v) * Sm(0f, 0.01f, t) * Sm(1f, 0.99f, t);
                    int ox = (c % 2) * cw + x, oy = (c / 2) * chh + y;
                    al[oy * w + ox] = a; lum[oy * w + ox] = tl[sv * chh + su];
                }
        }
        Save(al, lum, w, h, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[StreakTex]::Sheet((Join-Path $OutDir "UBA_StreakSheet_a.png"), 768, 192, 11)
[StreakTex]::Sheet((Join-Path $OutDir "UBA_StreakSheet2_a.png"), 768, 192, 57)
[StreakTex]::Strands((Join-Path $OutDir "UBA_Strand_a.png"), 128, 512, 7)
Write-Output "done"
