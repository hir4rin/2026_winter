# Usage: powershell -ExecutionPolicy Bypass -File gen_blood_textures.ps1 <outDir> <suffix>
param([string]$OutDir, [string]$Suffix = "a", [string]$Palette = "red")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class BloodTex
{
    // dark/light r,g,b then specular r,g,b
    static float[] Pal = { 0.20f, 0.56f, 0.006f, 0.024f, 0.045f, 0.105f, 0.93f, 0.42f, 0.47f };
    static float[] MistRgb = { 0.56f, 0.01f, 0.085f };
    public static bool Pure = false;
    public static void SetPalette(string name)
    {
        if (name == "pure") { Pure = true; }
        if (name == "white") { Pal = new float[] { 0.60f, 0.97f, 0.56f, 0.95f, 0.55f, 0.93f, 1f, 1f, 1f }; MistRgb = new float[] { 0.95f, 0.93f, 0.92f }; }
        if (name == "ink") { Pal = new float[] { 0.02f, 0.09f, 0.015f, 0.06f, 0.02f, 0.07f, 0.55f, 0.52f, 0.55f }; MistRgb = new float[] { 0.06f, 0.05f, 0.06f }; }
    }

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

    class Cap { public float ax, ay, bx, by, ra, rm, rb; }

    static Cap MkCap(float ax, float ay, float bx, float by, float ra, float rm, float rb)
    {
        Cap c = new Cap(); c.ax = ax; c.ay = ay; c.bx = bx; c.by = by; c.ra = ra; c.rm = rm; c.rb = rb; return c;
    }

    static float CapF(Cap c, float x, float y)
    {
        float vx = c.bx - c.ax, vy = c.by - c.ay;
        float t = C(((x - c.ax) * vx + (y - c.ay) * vy) / (vx * vx + vy * vy + 1e-6f));
        float dx = x - (c.ax + vx * t), dy = y - (c.ay + vy * t);
        float r = t < 0.5f ? L(c.ra, c.rm, t * 2) : L(c.rm, c.rb, (t - 0.5f) * 2);
        return r - (float)Math.Sqrt(dx * dx + dy * dy);
    }

    static float SMax(float a, float b, float k)
    {
        float h = C(0.5f + 0.5f * (a - b) / k);
        return L(b, a, h) + k * h * (1 - h);
    }

    class Sheet
    {
        public int w, h; public float[] R, G, B, A;
        public Sheet(int w, int h)
        {
            this.w = w; this.h = h; R = new float[w * h]; G = new float[w * h]; B = new float[w * h]; A = new float[w * h];
        }
        public void Save(string path)
        {
            Bitmap bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
            BitmapData d = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
            byte[] buf = new byte[w * h * 4];
            for (int i = 0; i < w * h; i++)
            {
                if (Pure) { R[i] = 1; G[i] = 1; B[i] = 1; A[i] = C(A[i] / 0.55f); }
                buf[i * 4] = (byte)(C(B[i]) * 255); buf[i * 4 + 1] = (byte)(C(G[i]) * 255);
                buf[i * 4 + 2] = (byte)(C(R[i]) * 255); buf[i * 4 + 3] = (byte)(C(A[i]) * 255);
            }
            Marshal.Copy(buf, 0, d.Scan0, buf.Length);
            bmp.UnlockBits(d); bmp.Save(path, ImageFormat.Png); bmp.Dispose();
        }
    }

    static void Liquid(Sheet S, int ox, int oy, int cell, List<Cap> caps, int seed, float nAmp, float nFreq, float k, float domeW, float minA)
    {
        float[] f = new float[cell * cell], h = new float[cell * cell];
        float pix = 2f / cell, maxF = 0.001f;
        for (int py = 0; py < cell; py++)
            for (int px = 0; px < cell; px++)
            {
                float x = (px + 0.5f) / cell * 2 - 1, y = 1 - (py + 0.5f) / cell * 2;
                float v = -9;
                foreach (Cap c in caps) { float g = CapF(c, x, y); v = v < -8 ? g : SMax(v, g, k); }
                v += (Fbm(x * nFreq + 3, y * nFreq + 7, seed) - 0.5f) * nAmp;
                f[py * cell + px] = v;
                if (v > maxF) maxF = v;
                float q = 1 - C(v / domeW);
                h[py * cell + px] = (float)Math.Sqrt(Math.Max(0, 1 - q * q)) * domeW;
            }
        float lx = -0.42f, ly = 0.62f, lz = 0.66f;
        float hx = lx, hy = ly, hz = lz + 1, hl = (float)Math.Sqrt(hx * hx + hy * hy + hz * hz); hx /= hl; hy /= hl; hz /= hl;
        for (int py = 1; py < cell - 1; py++)
            for (int px = 1; px < cell - 1; px++)
            {
                int i = py * cell + px;
                int idx = (oy + py) * S.w + ox + px;
                float x = (px + 0.5f) / cell * 2 - 1, y = 1 - (py + 0.5f) / cell * 2;
                float dhx = (h[i + 1] - h[i - 1]) / (2 * pix), dhy = (h[i - cell] - h[i + cell]) / (2 * pix);
                float nx = -dhx, ny = -dhy, nz = 1, nl = (float)Math.Sqrt(nx * nx + ny * ny + nz * nz); nx /= nl; ny /= nl; nz /= nl;
                float dif = C(nx * lx + ny * ly + nz * lz);
                float spec = (float)Math.Pow(C(nx * hx + ny * hy + nz * hz), 46);
                float t = Sm(0.25f, 0.95f, dif);
                float r = L(Pal[0], Pal[1], t), g = L(Pal[2], Pal[3], t), b = L(Pal[4], Pal[5], t);
                float sp = spec * 0.85f;
                r = L(r, Pal[6], sp); g = L(g, Pal[7], sp); b = L(b, Pal[8], sp);
                float aa = C(f[i] / pix + 0.5f);
                float er = C(f[i] / maxF * 1.15f + (Fbm(x * 7 + 11, y * 7 + 5, seed + 3) - 0.5f) * 0.55f);
                float edge = C(Math.Min(Math.Min(px, cell - 1 - px), Math.Min(py, cell - 1 - py)) / 8f);
                S.R[idx] = r; S.G[idx] = g; S.B[idx] = b;
                S.A[idx] = aa * L(minA, 1, Sm(0, 0.85f, er)) * edge;
            }
    }

    static void FillRgb(Sheet S, float r, float g, float b)
    {
        for (int i = 0; i < S.w * S.h; i++) { S.R[i] = r; S.G[i] = g; S.B[i] = b; }
    }

    public static void Core(string path, int cell)
    {
        Sheet S = new Sheet(cell * 2, cell * 2); FillRgb(S, 0.4f, 0.015f, 0.08f);
        for (int v = 0; v < 4; v++)
        {
            Random r = new Random(101 + v * 17);
            List<Cap> caps = new List<Cap>();
            float cx = (float)(r.NextDouble() - 0.5) * 0.2f;
            caps.Add(MkCap(cx, -0.9f, cx * 0.5f, -0.22f, 0.008f, L(0.11f, 0.16f, (float)r.NextDouble()), 0.09f));
            int n = 5 + r.Next(3);
            for (int i = 0; i < n; i++)
            {
                float a = (float)(r.NextDouble() * 2 - 1); a = a * Math.Abs(a) * 0.72f + (float)(r.NextDouble() - 0.5) * 0.2f;
                float q = (float)r.NextDouble();
                float len = L(0.42f, 1.5f, q * q) * (1 - Math.Abs(a) * 0.3f);
                float curl = (float)(r.NextDouble() - 0.5) * 0.3f;
                float sx = (float)Math.Sin(a), cy = (float)Math.Cos(a);
                float mx = sx * len * 0.55f + curl * 0.5f, my = -0.62f + cy * len * 0.55f;
                float bx = sx * len + curl, by = -0.62f + cy * len;
                float rb = L(0.07f, 0.13f, (float)r.NextDouble());
                caps.Add(MkCap(cx * 0.5f, -0.5f, mx, my, rb, rb * 0.5f, L(0.018f, 0.04f, (float)r.NextDouble())));
                float rt = q < 0.45f ? L(0.022f, 0.05f, (float)r.NextDouble()) : 0.012f;
                caps.Add(MkCap(mx, my, bx, by, 0.022f, 0.014f, rt));
            }
            for (int i = 0; i < 4; i++)
            {
                float dx = (float)(r.NextDouble() - 0.5) * 1.3f, dy = L(-0.2f, 0.82f, (float)r.NextDouble());
                caps.Add(MkCap(dx, dy - 0.04f, dx, dy + 0.04f, 0.008f, 0.022f, 0.01f));
            }
            Liquid(S, (v % 2) * cell, (v / 2) * cell, cell, caps, 200 + v, 0.1f, 6, 0.06f, 0.1f, 0.5f);
        }
        S.Save(path);
    }

    public static void Chunk(string path, int cell)
    {
        Sheet S = new Sheet(cell * 2, cell * 2); FillRgb(S, 0.4f, 0.015f, 0.08f);
        for (int v = 0; v < 4; v++)
        {
            Random r = new Random(303 + v * 29);
            List<Cap> caps = new List<Cap>();
            float[] ys = { -0.82f, -0.42f, -0.02f, 0.4f, 0.8f };
            float[] xs = new float[5], rs = { 0.018f, L(0.05f, 0.09f, (float)r.NextDouble()), L(0.11f, 0.16f, (float)r.NextDouble()), L(0.05f, 0.1f, (float)r.NextDouble()), 0.022f };
            for (int i = 0; i < 5; i++) xs[i] = (float)(r.NextDouble() - 0.5) * 0.3f;
            for (int i = 0; i < 4; i++)
                caps.Add(MkCap(xs[i], ys[i], xs[i + 1], ys[i + 1], rs[i], (rs[i] + rs[i + 1]) * 0.5f * L(0.7f, 1, (float)r.NextDouble()), rs[i + 1]));
            for (int i = 0; i < 4; i++)
            {
                int at = 1 + r.Next(3);
                float sd = i % 2 == 0 ? 1 : -1;
                float ex = xs[at] + sd * L(0.22f, 0.5f, (float)r.NextDouble()), ey = ys[at] + L(-0.05f, 0.4f, (float)r.NextDouble());
                caps.Add(MkCap(xs[at], ys[at], ex, ey, rs[at] * 0.55f, 0.014f, L(0.016f, 0.038f, (float)r.NextDouble())));
            }
            for (int i = 0; i < 2; i++)
            {
                float dx = (float)(r.NextDouble() - 0.5) * 0.9f, dy = L(-0.75f, 0.72f, (float)r.NextDouble());
                caps.Add(MkCap(dx, dy - 0.06f, dx, dy + 0.06f, 0.008f, 0.024f, 0.012f));
            }
            Liquid(S, (v % 2) * cell, (v / 2) * cell, cell, caps, 400 + v, 0.11f, 5, 0.05f, 0.12f, 0.55f);
        }
        S.Save(path);
    }

    public static void Fine(string path, int cell)
    {
        Sheet S = new Sheet(cell * 2, cell * 2); FillRgb(S, 0.4f, 0.015f, 0.08f);
        for (int v = 0; v < 4; v++)
        {
            float rh = 0.16f + v * 0.04f;
            List<Cap> caps = new List<Cap>();
            caps.Add(MkCap(0, -0.8f, 0, 0.45f, 0.02f, rh * 0.5f, rh));
            Liquid(S, (v % 2) * cell, (v / 2) * cell, cell, caps, 500 + v, 0, 1, 0.03f, 0.3f, 0.7f);
        }
        S.Save(path);
    }

    public static void Mist(string path, int cell)
    {
        Sheet S = new Sheet(cell * 2, cell * 2); FillRgb(S, MistRgb[0], MistRgb[1], MistRgb[2]);
        for (int v = 0; v < 4; v++)
        {
            int ox = (v % 2) * cell, oy = (v / 2) * cell, seed = 600 + v * 7;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = 1 - (py + 0.5f) / cell * 2;
                    float d = (float)Math.Sqrt(x * x / 0.72f + y * y / 0.9f);
                    float e = 1 - Sm(0.12f, 0.92f, d);
                    float n = Fbm(x * 3.2f + 9, y * 1.7f + v * 3, seed);
                    float m = Sm(0.32f, 0.72f, n + e * 0.28f - 0.1f);
                    int idx = (oy + py) * S.w + ox + px;
                    S.A[idx] = e * m * 0.85f;
                }
        }
        S.Save(path);
    }

    public static void Shred(string path, int cell)
    {
        Sheet S = new Sheet(cell * 2, cell * 2); FillRgb(S, 0.07f, 0.004f, 0.018f);
        for (int v = 0; v < 4; v++)
        {
            int ox = (v % 2) * cell, oy = (v / 2) * cell, seed = 700 + v * 11;
            float pix = 2f / cell;
            Random r = new Random(seed);
            float tilt = (float)(r.NextDouble() - 0.5) * 0.25f;
            for (int py = 2; py < cell - 2; py++)
                for (int px = 2; px < cell - 2; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = 1 - (py + 0.5f) / cell * 2;
                    float sx = x - tilt * y;
                    float w = 0.13f * (float)Math.Pow(Math.Max(0, 1 - y * y / 0.72f), 0.8);
                    float n = (Fbm(x * 10 + 2, y * 5 + 8, seed) - 0.5f) * 0.42f + (Fbm(x * 26, y * 14, seed + 5) - 0.5f) * 0.12f;
                    float f = w - Math.Abs(sx) + n;
                    int idx = (oy + py) * S.w + ox + px;
                    float a = C(f / pix + 0.5f);
                    float rim = 1 - C(f / 0.09f);
                    S.R[idx] = L(0.05f, 0.28f, rim); S.G[idx] = 0.004f; S.B[idx] = L(0.012f, 0.05f, rim);
                    float er = C(f / 0.24f + (Fbm(x * 8, y * 8, seed + 9) - 0.5f) * 0.5f);
                    S.A[idx] = a * L(0.55f, 1, er);
                }
        }
        S.Save(path);
    }

    public static void Strand(string path, int w, int h)
    {
        Sheet S = new Sheet(w, h);
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float u = (px + 0.5f) / w, v = (py + 0.5f) / h * 2 - 1;
                float aa = C((1 - Math.Abs(v)) / 0.28f);
                int i = py * w + px;
                S.R[i] = 1; S.G[i] = 1; S.B[i] = 1;
                float t = C(0.55f + (Fbm(u * 6, v * 0.8f, 800) - 0.5f) * 1.2f);
                S.A[i] = aa * L(0.55f, 1, t);
            }
        S.Save(path);
    }

    public static void Preview(string dir, string[] names, string outPath)
    {
        int cw = 320;
        Bitmap b = new Bitmap(cw * 3, cw * 2, PixelFormat.Format32bppArgb);
        Graphics g = Graphics.FromImage(b);
        g.Clear(Color.FromArgb(70, 72, 78));
        for (int i = 0; i < names.Length; i++)
        {
            Image m = Image.FromFile(System.IO.Path.Combine(dir, names[i]));
            g.DrawImage(m, (i % 3) * cw + 4, (i / 3) * cw + 4, cw - 8, cw - 8);
            m.Dispose();
        }
        g.Dispose(); b.Save(outPath, ImageFormat.Png); b.Dispose();
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
[BloodTex]::SetPalette($Palette)
$n = @("BH_Core_$Suffix.png","BH_Chunk_$Suffix.png","BH_Fine_$Suffix.png","BH_Mist_$Suffix.png","BH_Shred_$Suffix.png","BH_Strand_$Suffix.png")
[BloodTex]::Core((Join-Path $OutDir $n[0]), 256)
[BloodTex]::Chunk((Join-Path $OutDir $n[1]), 256)
[BloodTex]::Fine((Join-Path $OutDir $n[2]), 64)
[BloodTex]::Mist((Join-Path $OutDir $n[3]), 256)
[BloodTex]::Shred((Join-Path $OutDir $n[4]), 128)
[BloodTex]::Strand((Join-Path $OutDir $n[5]), 128, 32)
[BloodTex]::Preview($OutDir, $n, (Join-Path (Split-Path $OutDir -Parent) "preview_textures_$Suffix.png"))
Get-ChildItem $OutDir | Select-Object Name, Length
