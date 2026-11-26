# Procedural textures for the Saiyan aura (v2).
# Usage: powershell -ExecutionPolicy Bypass -File gen_textures.ps1 [outDir]
param([string]$OutDir = (Join-Path (Get-Location).Path "Texture"))

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class TexGen
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }

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

    // ---------- Aura silhouette: union of flame tongues around a body-shaped ellipse ----------
    const int Tongues = 22;
    static float[] tPhi, tLen, tWid, tFreq, tPhase, tBend;

    static void InitTongues(int seed)
    {
        var rnd = new Random(seed);
        tPhi = new float[Tongues]; tLen = new float[Tongues]; tWid = new float[Tongues];
        tFreq = new float[Tongues]; tPhase = new float[Tongues]; tBend = new float[Tongues];
        for (int i = 0; i < Tongues; i++)
        {
            // spread from the lower left, over the head, to the lower right
            float u = (i + 0.5f) / Tongues;
            float phi = (u * 2 - 1) * 2.7f + (float)(rnd.NextDouble() - 0.5) * 0.15f;
            tPhi[i] = phi;
            float top = (float)Math.Cos(phi);               // 1 at the head, -0.9 at the feet
            tLen[i] = 0.16f + 0.30f * C01(top * 0.5f + 0.55f) + (float)rnd.NextDouble() * 0.10f;
            tWid[i] = 0.11f + (float)rnd.NextDouble() * 0.05f;
            tFreq[i] = 1 + rnd.Next(3);                        // integer => seamless loop
            tPhase[i] = (float)(rnd.NextDouble() * Math.PI * 2);
            tBend[i] = (float)(rnd.NextDouble() - 0.5) * 0.25f;
        }
    }

    // Positive inside. x,y in [-1,1], y up. k shrinks the shape for inner colour bands.
    static float AuraField(float x, float y, float time, float k)
    {
        const float cy = -0.22f, rx = 0.40f, ry = 0.60f;
        float ex = x / (rx * (0.55f + 0.45f * k)), ey = (y - cy) / (ry * (0.75f + 0.25f * k));
        float f = (1 - (float)Math.Sqrt(ex * ex + ey * ey)) * rx;
        for (int i = 0; i < Tongues; i++)
        {
            float phi = tPhi[i];
            float bx = rx * 0.92f * (float)Math.Sin(phi) * (0.6f + 0.4f * k);
            float by = cy + ry * 0.92f * (float)Math.Cos(phi) * (0.8f + 0.2f * k);
            // tongues point up, leaning outward on the sides
            float wob = 0.10f * (float)Math.Sin(time * 2 * Math.PI * tFreq[i] + tPhase[i] * 1.7f);
            float ang = (float)Math.Sin(phi) * 0.55f + wob;
            float dx = (float)Math.Sin(ang), dy = (float)Math.Cos(ang);
            float len = tLen[i] * (1 + 0.38f * (float)Math.Sin(time * 2 * Math.PI * tFreq[i] + tPhase[i])) * (0.35f + 0.65f * k);
            float wid = tWid[i] * k;
            float px = x - bx, py = y - by;
            float along = px * dx + py * dy;
            float across = px * dy - py * dx;
            float t = along / len;
            across -= tBend[i] * t * t * len;                 // curl the tip
            float g;
            if (t < 0) g = wid - Math.Abs(across) + along * 2;
            else if (t > 1) g = -(t - 1) * len - Math.Abs(across);
            else g = wid * (float)Math.Pow(1 - t, 1.15) * (1 + 0.35f * (float)Math.Sin(Math.PI * t)) - Math.Abs(across);
            if (g > f) f = g;
        }
        return f;
    }

    public static void AuraSheet(string path, int cell, int cols, int rows, int seed)
    {
        InitTongues(seed);
        int w = cell * cols, h = cell * rows, n = cols * rows;
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        float pix = 2.0f / cell;
        for (int fi = 0; fi < n; fi++)
        {
            float time = (float)fi / n;
            int ox = (fi % cols) * cell, oy = (fi / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1;
                    float y = 1 - (py + 0.5f) / cell * 2;
                    float outer = C01(AuraField(x, y, time, 1.00f) / pix + 0.5f);
                    if (outer <= 0) continue;
                    float mid = C01(AuraField(x, y, time, 0.72f) / pix + 0.5f);
                    float inner = C01(AuraField(x, y, time, 0.48f) / pix + 0.5f);
                    // cel bands: orange rim -> yellow -> pale translucent core (body stays visible)
                    float r = 1.00f, g = 0.55f, b = 0.08f, a = 0.95f;
                    r += (1.00f - r) * mid; g += (0.86f - g) * mid; b += (0.25f - b) * mid; a += (0.80f - a) * mid;
                    r += (1.00f - r) * inner; g += (0.97f - g) * inner; b += (0.70f - b) * inner; a += (0.10f - a) * inner;
                    int idx = (oy + py) * w + ox + px;
                    R[idx] = r; G[idx] = g; B[idx] = b; A[idx] = a * outer;
                }
        }
        Save(R, G, B, A, w, h, path);
    }

    // ---------- Soft (realistic) aura envelope: same silhouette, no cel bands ----------
    static float Hash(int x, int y, int seed)
    {
        unchecked
        {
            int h = x * 374761393 + y * 668265263 + seed * 144269504;
            h = (h ^ (h >> 13)) * 1274126177;
            return ((h ^ (h >> 16)) & 0xffff) / 65535f;
        }
    }

    // value noise, periodic in y with period py cells
    static float Noise(float x, float y, int py, int seed)
    {
        int x0 = (int)Math.Floor(x), y0 = (int)Math.Floor(y);
        float fx = x - x0, fy = y - y0;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
        int ya = ((y0 % py) + py) % py, yb = (((y0 + 1) % py) + py) % py;
        float a = Hash(x0, ya, seed), b = Hash(x0 + 1, ya, seed);
        float c = Hash(x0, yb, seed), d = Hash(x0 + 1, yb, seed);
        return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
    }

    const float FitScale = 0.905f, FitShift = 0.2f;

    public static void SoftAuraSheet(string path, int cell, int cols, int rows, int seed)
    {
        InitTongues(seed);
        int w = cell * cols, h = cell * rows, n = cols * rows;
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int fi = 0; fi < n; fi++)
        {
            float time = (float)fi / n;
            int ox = (fi % cols) * cell, oy = (fi / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    // Tongue tips reach y=1.1 plus ~0.15 of glow, so fit shape space [-0.85, 1.25]
                    // into the cell with a margin (otherwise the tips are cut flat at the cell edge).
                    // Compensate in the effect: sprite scale / FitScale, position + half-height * FitShift.
                    float x = ((px + 0.5f) / cell * 2 - 1) / FitScale;
                    float y = (1 - (py + 0.5f) / cell * 2) / FitScale + FitShift;
                    float f = AuraField(x, y, time, 1.0f);
                    if (f < -0.25f) continue;
                    // bright rim, soft falloff outside, dim hollow centre
                    float a = f < 0 ? (float)Math.Exp(f / 0.045f) : 0.30f + 0.70f * (float)Math.Exp(-f / 0.07f);
                    float hollow = C01(AuraField(x, y, time, 0.5f) / 0.12f);
                    a *= 1 - 0.65f * hollow;
                    // upward flow streaks (stretched noise scrolling up, loops over the sheet)
                    float s = Noise(x * 9, (y - time * 2) * 3, 6, seed) * 0.6f + Noise(x * 18, (y - time * 2) * 6, 12, seed + 7) * 0.4f;
                    a *= 0.45f + 0.75f * s;
                    // whiter near the rim, golden elsewhere (node colour tints further)
                    float rimW = f < 0 ? 0 : (float)Math.Exp(-f / 0.05f);
                    int idx = (oy + py) * w + ox + px;
                    // safety fade at the cell border so nothing can end in a hard line
                    float border = C01(Math.Min(Math.Min(px, cell - 1 - px), Math.Min(py, cell - 1 - py)) / 10f);
                    R[idx] = 1; G[idx] = 0.85f + 0.15f * rimW; B[idx] = 0.55f + 0.45f * rimW;
                    A[idx] = C01(a) * border;
                }
        }
        Save(R, G, B, A, w, h, path);
    }

    // Horizontal light streak that fades to zero well before the texture edges
    // (the built-in streak reaches the edges and gets cut flat when stretched).
    public static void SoftStreak(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, y = (py + 0.5f) / h * 2 - 1;
                float along = (float)Math.Pow(Math.Max(0, 1 - x * x / 0.81f), 1.5);   // zero at |x| >= 0.9
                float across = (float)Math.Exp(-y * y / (2 * 0.12f * 0.12f));
                float edgeY = C01((1 - Math.Abs(y)) / 0.15f);
                int idx = py * w + px;
                R[idx] = 1; G[idx] = 1; B[idx] = 1; A[idx] = along * across * edgeY;
            }
        Save(R, G, B, A, w, h, path);
    }

    // Tileable grey noise for distortion (heat haze)
    public static void NoiseTile(string path, int size, int seed)
    {
        float[] R = new float[size * size], G = new float[size * size], B = new float[size * size], A = new float[size * size];
        for (int py = 0; py < size; py++)
            for (int px = 0; px < size; px++)
            {
                float u = (float)px / size, v = (float)py / size;
                // periodic in both axes: use periodic cells in x via wrap
                float n1 = PNoise(u * 4, v * 4, 4, seed), n2 = PNoise(u * 8, v * 8, 8, seed + 3);
                float val = n1 * 0.65f + n2 * 0.35f;
                float dx = (float)Math.Sqrt(Math.Max(0, 1 - Math.Pow(Math.Abs(u - 0.5f) * 2, 2)));
                float dy = (float)Math.Sqrt(Math.Max(0, 1 - Math.Pow(Math.Abs(v - 0.5f) * 2, 2)));
                int idx = py * size + px;
                R[idx] = val; G[idx] = PNoise(u * 4 + 17, v * 4, 4, seed + 11) * 0.65f + n2 * 0.35f; B[idx] = 1;
                A[idx] = dx * dy;
            }
        Save(R, G, B, A, size, size, path);
    }

    static float PNoise(float x, float y, int p, int seed)
    {
        int x0 = (int)Math.Floor(x), y0 = (int)Math.Floor(y);
        float fx = x - x0, fy = y - y0;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
        int xa = ((x0 % p) + p) % p, xb = (((x0 + 1) % p) + p) % p;
        int ya = ((y0 % p) + p) % p, yb = (((y0 + 1) % p) + p) % p;
        float a = Hash(xa, ya, seed), b = Hash(xb, ya, seed), c = Hash(xa, yb, seed), d = Hash(xb, yb, seed);
        float top = a + (b - a) * fx, bot = c + (d - c) * fx;
        return top + (bot - top) * fy;
    }

    // ---------- Jagged lightning (midpoint displacement) ----------
    static void Bolt(System.Collections.Generic.List<float[]> segs, float x0, float y0, float x1, float y1, float disp, int depth, Random rnd)
    {
        if (depth == 0) { segs.Add(new float[] { x0, y0, x1, y1 }); return; }
        float mx = (x0 + x1) / 2, my = (y0 + y1) / 2;
        float nx = -(y1 - y0), ny = x1 - x0;
        float o = (float)(rnd.NextDouble() * 2 - 1) * disp;
        mx += nx * o; my += ny * o;
        // constant ratio keeps the bolt jagged at every scale
        Bolt(segs, x0, y0, mx, my, disp, depth - 1, rnd);
        Bolt(segs, mx, my, x1, y1, disp, depth - 1, rnd);
        if (depth == 4 && rnd.NextDouble() < 0.6)
        {
            float bl = 0.5f + (float)rnd.NextDouble() * 0.4f;
            float ang = (float)(rnd.NextDouble() - 0.5) * 1.6f;
            float dx = (x1 - x0), dy = (y1 - y0);
            float ex = mx + (dx * (float)Math.Cos(ang) - dy * (float)Math.Sin(ang)) * bl;
            float ey = my + (dx * (float)Math.Sin(ang) + dy * (float)Math.Cos(ang)) * bl;
            Bolt(segs, mx, my, ex, ey, disp, 3, rnd);
        }
    }

    static float SegDist(float px, float py, float[] s)
    {
        float vx = s[2] - s[0], vy = s[3] - s[1];
        float t = ((px - s[0]) * vx + (py - s[1]) * vy) / (vx * vx + vy * vy + 1e-9f);
        t = t < 0 ? 0 : (t > 1 ? 1 : t);
        float dx = px - (s[0] + vx * t), dy = py - (s[1] + vy * t);
        return (float)Math.Sqrt(dx * dx + dy * dy);
    }

    public static void LightningSheet(string path, int cell, int cols, int rows, int seed)
    {
        LightningSheet(path, cell, cols, rows, seed, 0.006f, 0.035f, 0.75f);
    }

    // coreW: half width of the white core, glowW: falloff of the blue glow, glowA: glow strength
    public static void LightningSheet(string path, int cell, int cols, int rows, int seed, float coreW, float glowW, float glowA)
    {
        var rnd = new Random(seed);
        int w = cell * cols, h = cell * rows, n = cols * rows;
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int fi = 0; fi < n; fi++)
        {
            var segs = new System.Collections.Generic.List<float[]>();
            Bolt(segs, -0.92f, (float)(rnd.NextDouble() - 0.5) * 0.3f, 0.92f, (float)(rnd.NextDouble() - 0.5) * 0.3f, 0.30f, 6, rnd);
            int ox = (fi % cols) * cell, oy = (fi / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = 1 - (py + 0.5f) / cell * 2;
                    float d = 10;
                    foreach (var s in segs) { float sd = SegDist(x, y, s); if (sd < d) d = sd; }
                    float core = C01(1 - (d - coreW) / 0.008f);
                    float glow = (float)Math.Exp(-d / glowW);
                    float edge = C01(Math.Min(Math.Min(px, cell - 1 - px), Math.Min(py, cell - 1 - py)) / 12f);
                    int idx = (oy + py) * w + ox + px;
                    R[idx] = 0.55f + 0.45f * core; G[idx] = 0.80f + 0.20f * core; B[idx] = 1;
                    A[idx] = Math.Max(core, glow * glowA) * edge;
                }
        }
        Save(R, G, B, A, w, h, path);
    }

    // ---------- Rock chip ----------
    public static void Rock(string path, int size, int seed)
    {
        var rnd = new Random(seed);
        float[] amp = new float[5]; float[] ph = new float[5];
        for (int i = 0; i < 5; i++) { amp[i] = (float)rnd.NextDouble() * 0.12f / (i + 1); ph[i] = (float)(rnd.NextDouble() * 6.28); }
        float[] R = new float[size * size], G = new float[size * size], B = new float[size * size], A = new float[size * size];
        float pix = 2.0f / size;
        for (int py = 0; py < size; py++)
            for (int px = 0; px < size; px++)
            {
                float x = (px + 0.5f) / size * 2 - 1, y = 1 - (py + 0.5f) / size * 2;
                float th = (float)Math.Atan2(y, x), r = (float)Math.Sqrt(x * x + y * y);
                float rad = 0.72f;
                for (int i = 0; i < 5; i++) rad += amp[i] * (float)Math.Sin((i + 2) * th + ph[i]);
                // faceted: quantize the angle a little
                float f = rad - r;
                float m = C01(f / pix + 0.5f);
                float shade = 0.55f + 0.35f * C01((x * -0.6f + y * 0.8f) * 0.8f + 0.3f);
                float rim = C01(f / 0.12f);
                shade *= 0.45f + 0.55f * rim;
                int idx = py * size + px;
                R[idx] = shade * 0.95f; G[idx] = shade * 0.85f; B[idx] = shade * 0.72f; A[idx] = m;
            }
        Save(R, G, B, A, size, size, path);
    }

    // ---------- Reference humanoid silhouette (for checking scale only) ----------
    static float Capsule(float x, float y, float ax, float ay, float bx, float by, float r)
    {
        return r - SegDist(x, y, new float[] { ax, ay, bx, by });
    }

    public static void Humanoid(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        float pix = 2.0f / w;
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                // x in [-1,1], y in [0, 2*h/w] from the feet up
                float x = (px + 0.5f) / w * 2 - 1, y = (h - py - 0.5f) / w * 2;
                float f = -1;
                f = Math.Max(f, 0.17f - (float)Math.Sqrt(x * x + (y - 3.45f) * (y - 3.45f)));   // head
                f = Math.Max(f, Capsule(x, y, 0, 2.55f, 0, 3.15f, 0.30f));                      // chest
                f = Math.Max(f, Capsule(x, y, 0, 2.0f, 0, 2.6f, 0.22f));                        // waist
                f = Math.Max(f, Capsule(x, y, -0.30f, 3.0f, -0.52f, 2.15f, 0.10f));             // arms
                f = Math.Max(f, Capsule(x, y, 0.30f, 3.0f, 0.52f, 2.15f, 0.10f));
                f = Math.Max(f, Capsule(x, y, -0.13f, 1.9f, -0.22f, 0.1f, 0.12f));              // legs
                f = Math.Max(f, Capsule(x, y, 0.13f, 1.9f, 0.22f, 0.1f, 0.12f));
                int idx = py * w + px;
                R[idx] = 0.10f; G[idx] = 0.12f; B[idx] = 0.20f; A[idx] = C01(f / pix + 0.5f);
            }
        Save(R, G, B, A, w, h, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
# v3: realistic look, v2 silhouette
[TexGen]::SoftAuraSheet((Join-Path $OutDir "aura_soft_sheet.png"), 256, 4, 4, 11)
[TexGen]::SoftAuraSheet((Join-Path $OutDir "aura_soft_sheet_b.png"), 256, 4, 4, 23)
[TexGen]::LightningSheet((Join-Path $OutDir "lightning_real_sheet.png"), 256, 4, 4, 5, 0.0035, 0.06, 0.55)
[TexGen]::NoiseTile((Join-Path $OutDir "haze_noise.png"), 128, 9)
[TexGen]::SoftStreak((Join-Path $OutDir "streak_soft.png"), 256, 64)
Get-ChildItem $OutDir | Select-Object Name, Length
