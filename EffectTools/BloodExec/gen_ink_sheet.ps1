# 使い方: powershell -ExecutionPolicy Bypass -File gen_ink_sheet.ps1 <出力フォルダ>
# UBA_InkSheet : Ring に貼る暗い液体の面(U = 弧の向き 尾0→頭1 / V = 幅)。ふちは黒い煙のように崩れる
# UBA_Pool     : 床の血だまり(真上から)。ふちは不規則、中につや
# UBA_Spike    : はじけるとげ(V = 長さ 根元0→先1)。先は鋭く、ふちはぎざぎざ
# どれも白ベース(明るさ=つや・暗さ)。色はノードで付ける
param([string]$OutDir)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class InkTex
{
    static float C(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float L(float a, float b, float t) { return a + (b - a) * t; }
    static float Sm(float a, float b, float x) { float t = C((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    static float Hash(int x, int y, int s)
    {
        unchecked
        {
            int h = x * 374761393 + y * 668265263 + s * 144269504;
            h = (h ^ (h >> 13)) * 1274126177;
            return ((h ^ (h >> 16)) & 0xffff) / 65535f;
        }
    }
    static float VN(float x, float y, int s)
    {
        int x0 = (int)Math.Floor(x), y0 = (int)Math.Floor(y);
        float fx = x - x0, fy = y - y0;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
        float a = Hash(x0, y0, s), b = Hash(x0 + 1, y0, s), c = Hash(x0, y0 + 1, s), d = Hash(x0 + 1, y0 + 1, s);
        return L(L(a, b, fx), L(c, d, fx), fy);
    }
    static float Fbm(float x, float y, int s)
    {
        float a = 0.5f, sum = 0, n = 0;
        for (int i = 0; i < 5; i++) { sum += a * VN(x, y, s + i * 13); n += a; a *= 0.5f; x *= 2.03f; y *= 2.03f; }
        return sum / n;
    }

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

    // 暗い液体の面: 中は濃くつやがあり、ふちは黒くなって煙のように崩れる
    public static void Sheet(string path, int w, int h, int seed)
    {
        float[] al = new float[w * h], lum = new float[w * h];
        for (int y = 0; y < h; y++)
        {
            float v = (y + 0.5f) / h;
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w;
                float cv = 0.5f + (Fbm(u * 1.8f, 0.5f, seed + 7) - 0.5f) * 0.12f;
                // 尾は細く、頭は太く丸い
                float half = L(0.08f, 0.46f, Sm(0.0f, 0.65f, u));
                float cap = (float)Math.Sqrt(C(1f - Sm(0.78f, 0.99f, u)));
                half *= L(0.02f, 1f, cap);
                float d = Math.Abs(v - cv) / Math.Max(half, 0.001f);

                // ふちを流れる向きに引き伸ばしたノイズで崩す(煙・ちぎれ)
                float n1 = Fbm(u * 6f, v * 9f, seed + 21);
                float n2 = Fbm(u * 3f, v * 22f, seed + 33);
                float edge = d + (n1 - 0.5f) * 0.55f + (n2 - 0.5f) * 0.35f;
                float core = 1f - Sm(0.55f, 0.8f, edge);         // 濃い液体
                float smoke = (1f - Sm(0.6f, 1.15f, edge)) * Sm(0.35f, 0.7f, n2); // ふちの煙
                float a = Math.Max(core, smoke * 0.75f);

                // 尾の先は細かく崩して消す
                float tail = Sm(0.0f, 0.3f, u);
                a *= L(Sm(0.45f, 0.6f, Fbm(u * 20f, v * 7f, seed + 40)), 1f, tail);
                a *= Sm(0.0f, 0.08f, u);
                // 端は必ず0
                a *= Sm(0.0f, 0.04f, v) * Sm(1.0f, 0.96f, v) * Sm(1.0f, 0.985f, u);

                // 明るさ: 中は明るく(色が出る)、ふちと煙は黒
                float streak = Fbm(u * 2.2f, v * 30f, seed);
                float shade = L(0.05f, 0.85f, 1f - Sm(0.1f, 0.85f, d)) * L(0.75f, 1.1f, streak);
                float gloss = (float)Math.Exp(-Math.Pow((v - (cv - half * 0.3f)) / Math.Max(half * 0.12f, 0.002f), 2));
                gloss *= Sm(0.25f, 0.75f, u) * Sm(0.45f, 0.75f, streak);
                float l = C(shade * core + gloss * 0.55f);
                al[y * w + x] = a; lum[y * w + x] = l;
            }
        }
        Save(al, lum, w, h, path);
    }

    // 床の血だまり(真上から)
    public static void Pool(string path, int w, int seed)
    {
        float[] al = new float[w * w], lum = new float[w * w];
        for (int y = 0; y < w; y++)
            for (int x = 0; x < w; x++)
            {
                float px = (x + 0.5f) / w * 2 - 1, py = (y + 0.5f) / w * 2 - 1;
                float r = (float)Math.Sqrt(px * px + py * py);
                float ang = (float)Math.Atan2(py, px);
                float ca = (float)Math.Cos(ang), sa = (float)Math.Sin(ang);
                float rim = 0.62f + (Fbm(ca * 2.2f + 5, sa * 2.2f + 5, seed) - 0.5f) * 0.45f;
                float n = Fbm(px * 3f, py * 3f, seed + 9);
                float rr = r / rim + (n - 0.5f) * 0.25f;
                float body = 1f - Sm(0.86f, 1.0f, rr);
                // まわりに飛んだ滴
                float drops = Sm(0.72f, 0.78f, Fbm(px * 9f, py * 9f, seed + 50)) * Sm(1.4f, 1.0f, rr) * Sm(0.95f, 1.05f, rr);
                float a = Math.Max(body, drops);
                a *= Sm(0.98f, 0.9f, r);
                float shade = L(0.15f, 0.75f, 1f - Sm(0.3f, 1.0f, rr)) * L(0.8f, 1.1f, n);
                float gloss = Sm(0.62f, 0.8f, Fbm(px * 2.5f + 3, py * 2.5f + 1, seed + 77)) * (1f - Sm(0.5f, 0.9f, rr));
                lum[y * w + x] = C(shade + gloss * 0.45f);
                al[y * w + x] = a;
            }
        Save(al, lum, w, w, path);
    }

    // はじけるとげ(縦長。下が根元、上が先)
    public static void Spike(string path, int w, int h, int seed)
    {
        float[] al = new float[w * h], lum = new float[w * h];
        for (int y = 0; y < h; y++)
        {
            float t = 1f - (y + 0.5f) / h; // 根元0 → 先1
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w * 2 - 1;
                float half = 0.85f * (float)Math.Pow(1f - t, 0.8f) * Sm(0.0f, 0.12f, t);
                float n = Fbm(u * 2f, t * 14f, seed);
                float d = Math.Abs(u + (Fbm(t * 3f, 0.3f, seed + 5) - 0.5f) * 0.25f) / Math.Max(half, 0.001f) + (n - 0.5f) * 0.6f;
                float a = 1f - Sm(0.7f, 1.0f, d);
                a *= Sm(0.0f, 0.06f, t) * Sm(1.0f, 0.97f, t);
                float l = L(0.2f, 1.0f, 1f - Sm(0.0f, 0.9f, d)) * L(0.6f, 1f, t);
                al[y * w + x] = a; lum[y * w + x] = C(l);
            }
        }
        Save(al, lum, w, h, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[InkTex]::Sheet((Join-Path $OutDir "UBA_InkSheet_a.png"), 512, 192, 11)
[InkTex]::Sheet((Join-Path $OutDir "UBA_InkSheet2_a.png"), 512, 192, 57)
[InkTex]::Pool((Join-Path $OutDir "UBA_Pool_a.png"), 256, 5)
[InkTex]::Spike((Join-Path $OutDir "UBA_Spike_a.png"), 96, 384, 3)
Write-Output "done"
