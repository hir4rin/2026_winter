# Usage: powershell -ExecutionPolicy Bypass -File gen_hitwind_textures.ps1 <outDir> <suffix>
# HW_Arc  : wind arc strip for Ring (U = along the arc, tail 0 -> head 1 / V = across)
# HW_Ray  : radial streak from the center to the right edge (rotate around the center)
# HW_Wisp : thin faint strands for the lingering wind
param([string]$OutDir, [string]$Suffix = "a")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class WindTex
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

    // strands: many thin lines along U, broken by noise, brighter edge at v=0
    public static void Arc(string path, int w, int h, int seed, float edge)
    {
        float[] al = new float[w * h], lum = new float[w * h];
        for (int y = 0; y < h; y++)
        {
            float v = (y + 0.5f) / h;
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w;
                // wobble the strands a little along U
                float vw = v + (Fbm(u * 3f, 0.5f, seed + 7) - 0.5f) * 0.12f;
                float strand = 0;
                // thin strands
                float k1 = Fbm(u * 2.5f, vw * 46f, seed);
                strand += Sm(0.52f, 0.78f, k1);
                // coarse smoky body
                float k2 = Fbm(u * 1.5f, vw * 9f, seed + 31);
                strand = strand * 0.75f + Sm(0.3f, 0.75f, k2) * 0.6f;
                // breaks along U
                float br = Sm(0.25f, 0.6f, Fbm(u * 7f, vw * 14f, seed + 55));
                strand *= L(0.35f, 1f, br);

                // across: fade on both sides, more body near the edge
                float across = Sm(0f, 0.12f, v) * (1f - Sm(0.55f, 1f, v));
                float body = L(1f, 0.45f, v);
                // bright rim near v=0
                float rim = Sm(0.0f, 0.04f, v) * (1f - Sm(0.05f, 0.16f, v)) * edge;

                // along: tail fades in, head is a soft cut
                float along = Sm(0f, 0.75f, u) * (1f - Sm(0.93f, 1f, u));
                float rimAlong = Sm(0.3f, 0.92f, u) * (1f - Sm(0.95f, 1f, u));

                float a = (strand * body * across * 1.7f) * along + rim * rimAlong;
                float l = L(0.82f, 1f, C(rim * 2f + strand * 0.3f));
                al[y * w + x] = a; lum[y * w + x] = l;
            }
        }
        Save(al, lum, w, h, path);
    }

    public static void Ray(string path, int n)
    {
        float[] al = new float[n * n], lum = new float[n * n];
        float c = n * 0.5f;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float dx = (x + 0.5f - c) / c, dy = (y + 0.5f - c) / c;
                if (dx <= 0) continue;
                float t = dx; // 0 center .. 1 edge
                float width = L(0.02f, 0.003f, t);
                float along = Sm(0.18f, 0.4f, t) * (1f - Sm(0.72f, 1f, t));
                // bundle of thin broken lines (scratchy)
                float sum = 0;
                for (int k = 0; k < 4; k++)
                {
                    float off = (Hash(k, 1, 9) - 0.5f) * 0.09f * t;
                    float d = Math.Abs(dy - off) / width;
                    float line = (float)Math.Exp(-d * d * 2.5f);
                    float dash = Sm(0.35f, 0.6f, VN(t * 22f, k * 3.7f, 21));
                    sum += line * dash * L(0.5f, 1f, Hash(k, 2, 9));
                }
                al[y * n + x] = C(sum) * along; lum[y * n + x] = 1f;
            }
        Save(al, lum, n, n, path);
    }

    public static void Wisp(string path, int w, int h, int seed)
    {
        float[] al = new float[w * h], lum = new float[w * h];
        for (int y = 0; y < h; y++)
        {
            float v = (y + 0.5f) / h;
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w;
                float vw = v + (Fbm(u * 2f, 0.5f, seed + 3) - 0.5f) * 0.3f;
                float s = Sm(0.6f, 0.85f, Fbm(u * 1.8f, vw * 30f, seed));
                float across = Sm(0f, 0.25f, v) * (1f - Sm(0.75f, 1f, v));
                float along = Sm(0f, 0.5f, u) * (1f - Sm(0.8f, 1f, u));
                al[y * w + x] = s * across * along; lum[y * w + x] = 1f;
            }
        }
        Save(al, lum, w, h, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[WindTex]::Arc((Join-Path $OutDir "HW_Arc_$Suffix.png"), 512, 128, 11, 1.0)
[WindTex]::Arc((Join-Path $OutDir "HW_Arc2_$Suffix.png"), 512, 128, 47, 0.7)
[WindTex]::Ray((Join-Path $OutDir "HW_Ray_$Suffix.png"), 256)
[WindTex]::Wisp((Join-Path $OutDir "HW_Wisp_$Suffix.png"), 512, 64, 5)
Write-Output "done"
