# Usage: powershell -ExecutionPolicy Bypass -File gen_movewind.ps1 <outDir> <suffix>
# MW_Film_<s>.png      : tileable thin strands along U (film surface, scrolled)
# MW_Cocoon_<s>.efkmodel: shell around the body (feet at y=0, height 9, +Z forward). U = front->back, V = around
param([string]$OutDir, [string]$Suffix = "a")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.IO;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class MoveWind
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

    // value noise periodic in x (px) and y (py)
    static float VNp(float x, float y, int px, int py, int s)
    {
        int x0 = (int)Math.Floor(x), y0 = (int)Math.Floor(y);
        float fx = x - x0, fy = y - y0;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
        int xa = ((x0 % px) + px) % px, xb = (xa + 1) % px;
        int ya = ((y0 % py) + py) % py, yb = (ya + 1) % py;
        float a = Hash(xa, ya, s), b = Hash(xb, ya, s), c = Hash(xa, yb, s), d = Hash(xb, yb, s);
        return L(L(a, b, fx), L(c, d, fx), fy);
    }

    static void Save(float[] al, int w, int h, string path)
    {
        Bitmap bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        BitmapData bd = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            buf[i * 4 + 0] = 255; buf[i * 4 + 1] = 255; buf[i * 4 + 2] = 255;
            buf[i * 4 + 3] = (byte)(C(al[i]) * 255);
        }
        Marshal.Copy(buf, 0, bd.Scan0, buf.Length);
        bmp.UnlockBits(bd);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    public static void Film(string path, int w, int h, int seed)
    {
        float[] al = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (float)x / w, v = (float)y / h;
                float warp = (VNp(u * 3, v * 4, 3, 4, seed + 7) - 0.5f) * 0.06f;
                float vv = v + warp;
                float s1 = VNp(u * 2, vv * 48, 2, 48, seed);
                float s2 = VNp(u * 3, vv * 90, 3, 90, seed + 3);
                float lines = Sm(0.62f, 0.85f, s1) * 0.8f + Sm(0.7f, 0.9f, s2) * 0.6f;
                float breaks = Sm(0.3f, 0.7f, VNp(u * 4, vv * 6, 4, 6, seed + 11));
                float body = VNp(u * 2, v * 3, 2, 3, seed + 19) * 0.12f;
                al[y * w + x] = C(lines * breaks + body);
            }
        Save(al, w, h, path);
    }

    // ribbon band: U = along (tapered both ends), V = across (soft edges + strands)
    public static void Band(string path, int w, int h, int seed)
    {
        float[] al = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
                float along = Sm(0.0f, 0.2f, u) * (1 - Sm(0.55f, 1.0f, u));
                float half = L(0.25f, 0.5f, along);
                float d = Math.Abs(v - 0.5f) / half;
                float body = 1 - Sm(0.5f, 1.0f, d);
                float strands = Sm(0.55f, 0.85f, VNp(u * 3, v * 20, 3, 20, seed));
                float edge = Sm(0.55f, 0.8f, d) * (1 - Sm(0.8f, 1.0f, d));
                al[y * w + x] = C((body * 0.35f + strands * body * 0.6f + edge * 0.5f) * along);
            }
        Save(al, w, h, path);
    }

    class Mesh
    {
        public List<float[]> V = new List<float[]>();
        public List<int> F = new List<int>();

        public void AddGrid(float[,,] P, float[,] A, float[,,] UV, int rows, int cols)
        {
            int baseIdx = V.Count, w = cols + 1;
            var N = new float[rows, w, 3];
            for (int j = 0; j < rows - 1; j++)
                for (int i = 0; i < cols; i++)
                {
                    Acc(P, N, j, i, j + 1, i + 1, j, i + 1);
                    Acc(P, N, j, i, j + 1, i, j + 1, i + 1);
                }
            for (int j = 0; j < rows; j++)
                for (int k = 0; k < 3; k++) { float s = N[j, 0, k] + N[j, cols, k]; N[j, 0, k] = s; N[j, cols, k] = s; }
            for (int j = 0; j < rows; j++)
                for (int i = 0; i < w; i++)
                {
                    float nx = N[j, i, 0], ny = N[j, i, 1], nz = N[j, i, 2];
                    float nl = (float)Math.Sqrt(nx * nx + ny * ny + nz * nz); if (nl < 1e-6f) { nx = 0; ny = 1; nz = 0; nl = 1; }
                    int i2 = Math.Min(i + 1, cols), i1 = Math.Max(i - 1, 0);
                    float tx = P[j, i2, 0] - P[j, i1, 0], ty = P[j, i2, 1] - P[j, i1, 1], tz = P[j, i2, 2] - P[j, i1, 2];
                    float tl = (float)Math.Sqrt(tx * tx + ty * ty + tz * tz); if (tl < 1e-6f) { tx = 1; ty = 0; tz = 0; tl = 1; }
                    V.Add(new float[] { P[j, i, 0], P[j, i, 1], P[j, i, 2], nx / nl, ny / nl, nz / nl, tx / tl, ty / tl, tz / tl, UV[j, i, 0], UV[j, i, 1], A[j, i] });
                }
            for (int j = 0; j < rows - 1; j++)
                for (int i = 0; i < cols; i++)
                {
                    int a = baseIdx + j * w + i, b = a + 1, c = a + w + 1, d = a + w;
                    F.Add(a); F.Add(c); F.Add(b);
                    F.Add(a); F.Add(d); F.Add(c);
                }
        }

        static void Acc(float[,,] P, float[,,] N, int j0, int i0, int j1, int i1, int j2, int i2)
        {
            float ux = P[j1, i1, 0] - P[j0, i0, 0], uy = P[j1, i1, 1] - P[j0, i0, 1], uz = P[j1, i1, 2] - P[j0, i0, 2];
            float vx = P[j2, i2, 0] - P[j0, i0, 0], vy = P[j2, i2, 1] - P[j0, i0, 1], vz = P[j2, i2, 2] - P[j0, i0, 2];
            float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
            int[,] idx = { { j0, i0 }, { j1, i1 }, { j2, i2 } };
            for (int k = 0; k < 3; k++) { N[idx[k, 0], idx[k, 1], 0] += nx; N[idx[k, 0], idx[k, 1], 1] += ny; N[idx[k, 0], idx[k, 1], 2] += nz; }
        }

        public void Write(string path)
        {
            using (var bw = new BinaryWriter(File.Create(path)))
            {
                bw.Write(5); bw.Write(1.0f); bw.Write(1); bw.Write(1);
                bw.Write(V.Count);
                foreach (var v in V)
                {
                    float bx = v[4] * v[8] - v[5] * v[7], by = v[5] * v[6] - v[3] * v[8], bz = v[3] * v[7] - v[4] * v[6];
                    bw.Write(v[0]); bw.Write(v[1]); bw.Write(v[2]);
                    bw.Write(v[3]); bw.Write(v[4]); bw.Write(v[5]);
                    bw.Write(bx); bw.Write(by); bw.Write(bz);
                    bw.Write(v[6]); bw.Write(v[7]); bw.Write(v[8]);
                    bw.Write(v[9]); bw.Write(v[10]);
                    byte al = (byte)Math.Max(0, Math.Min(255, (int)(v[11] * 255)));
                    bw.Write((byte)255); bw.Write((byte)255); bw.Write((byte)255); bw.Write(al);
                }
                bw.Write(F.Count / 3);
                for (int i = 0; i < F.Count; i++) bw.Write(F[i]);
            }
        }
    }

    // zFront..zBack along the body, ellipse cross-section (rx, ry) around (0, cy)
    public static void Cocoon(string path, float zFront, float zBack, float rx, float ry, float cy, float lean)
    {
        int rows = 40, cols = 48;
        var P = new float[rows, cols + 1, 3];
        var A = new float[rows, cols + 1];
        var UV = new float[rows, cols + 1, 2];
        for (int j = 0; j < rows; j++)
        {
            float t = (float)j / (rows - 1);
            float z = L(zFront, zBack, t);
            float s = t < 0.3f ? 0.25f + 0.75f * (float)Math.Pow(Math.Sin(t / 0.3f * Math.PI / 2), 0.6) : 1 - 0.3f * (float)Math.Pow((t - 0.3f) / 0.7f, 1.4);
            float fade = Sm(0.0f, 0.18f, t) * (1 - Sm(0.45f, 1.0f, t));
            for (int i = 0; i <= cols; i++)
            {
                float a = (float)(2 * Math.PI * i / cols);
                float x = (float)Math.Cos(a) * rx * s;
                float y = cy + (float)Math.Sin(a) * ry * s + lean * (z - zBack) / (zFront - zBack);
                P[j, i, 0] = x; P[j, i, 1] = y; P[j, i, 2] = z;
                float ground = Sm(0.0f, 1.8f, y);
                float top = 1 - Sm(cy + ry * 0.75f, cy + ry * 1.0f, y);
                A[j, i] = fade * ground * top;
                UV[j, i, 0] = t; UV[j, i, 1] = (float)i / cols;
            }
        }
        var m = new Mesh();
        m.AddGrid(P, A, UV, rows, cols);
        m.Write(path);
    }
}
'@

$tex = Join-Path $OutDir "Texture"
$mdl = Join-Path $OutDir "Model"
[MoveWind]::Film((Join-Path $tex "MW_Film_$Suffix.png"), 512, 256, 17)
[MoveWind]::Band((Join-Path $tex "MW_Band_$Suffix.png"), 256, 64, 23)
[MoveWind]::Cocoon((Join-Path $mdl "MW_Cocoon_$Suffix.efkmodel"), 2.2, -5.5, 2.1, 4.9, 4.4, 0.6)
"done"
