# Usage: powershell -ExecutionPolicy Bypass -File gen_liquid_arc.ps1 <outDir> <suffix>
# UBA_LiquidArc : liquid blood band for Ring (U = along the arc, tail 0 -> head 1 / V = across)
#   head is thick and round, tail thins and breaks into drops, streaks run along U,
#   a glossy highlight line runs a little off the center. White base (tint with the node color).
param([string]$OutDir, [string]$Suffix = "a")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class LiquidTex
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
        for (int i = 0; i < 4; i++) { sum += a * VN(x, y, s + i * 13); n += a; a *= 0.5f; x *= 2.03f; y *= 2.03f; }
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

    public static void Arc(string path, int w, int h, int seed)
    {
        float[] al = new float[w * h], lum = new float[w * h];
        for (int y = 0; y < h; y++)
        {
            float v = (y + 0.5f) / h;
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w;
                // center line wobbles a little
                float cv = 0.5f + (Fbm(u * 2.2f, 0.5f, seed + 7) - 0.5f) * 0.14f;
                // half width: thin tail -> thick head, round cap at the head
                float half = L(0.06f, 0.42f, Sm(0.0f, 0.82f, u));
                float cap = (float)Math.Sqrt(C(1f - Sm(0.86f, 1.0f, u)));
                half *= L(0.25f, 1f, cap);
                // edge wobble (liquid edge, not torn)
                half *= L(0.82f, 1.08f, Fbm(u * 5f, v * 2f, seed + 21));
                float d = Math.Abs(v - cv) / Math.Max(half, 0.001f);
                float body = 1f - Sm(0.78f, 1.0f, d);

                // tail breaks into drops
                float drops = Sm(0.45f, 0.62f, Fbm(u * 18f, v * 6f, seed + 40));
                float tailMix = Sm(0.08f, 0.38f, u);
                body *= L(drops, 1f, tailMix);
                // fade in at the very tail
                body *= Sm(0.0f, 0.1f, u);

                // streaks along U inside the liquid
                float streak = Fbm(u * 2.5f, v * 34f, seed);
                float a = body * L(0.72f, 1.0f, Sm(0.3f, 0.7f, streak));

                // shading: dark near the edges, glossy line a little above the center
                float shade = L(0.5f, 0.85f, 1f - d * d);
                float gloss = (float)Math.Exp(-Math.Pow((v - (cv - half * 0.35f)) / Math.Max(half * 0.16f, 0.002f), 2));
                gloss *= Sm(0.2f, 0.7f, u) * L(0.6f, 1f, streak);
                float l = C(shade * L(0.85f, 1.05f, streak) + gloss * 0.7f);

                al[y * w + x] = a; lum[y * w + x] = l;
            }
        }
        Save(al, lum, w, h, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[LiquidTex]::Arc((Join-Path $OutDir "UBA_LiquidArc_$Suffix.png"), 512, 128, 11)
[LiquidTex]::Arc((Join-Path $OutDir "UBA_LiquidArc2_$Suffix.png"), 512, 128, 57)
Write-Output "done"
