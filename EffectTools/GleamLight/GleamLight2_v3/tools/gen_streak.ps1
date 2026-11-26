param([string]$OutDir = (Join-Path $PSScriptRoot '..\Texture'))
# 弱2の放射状の白いギザギザの筋(4コマ縦並び)
$src = @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class StreakTex
{
    static float C01(float v){ return v < 0 ? 0 : (v > 1 ? 1 : v); }
    static float Smooth(float a, float b, float x){ float t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }
    public static void Sheet(string path, int w, int cellH, int rows, int seed)
    {
        var rnd = new Random(seed);
        var bmp = new Bitmap(w, cellH * rows, PixelFormat.Format32bppArgb);
        for (int r = 0; r < rows; r++)
        {
            int pts = 14;
            float[] ys = new float[pts + 1];
            for (int i = 0; i <= pts; i++) ys[i] = (float)(rnd.NextDouble() * 2 - 1) * 0.55f * (float)Math.Sin(Math.PI * i / pts);
            for (int y = 0; y < cellH; y++) for (int x = 0; x < w; x++)
            {
                float u = x / (float)(w - 1);
                float v = (y + 0.5f) / cellH * 2 - 1;
                float fi = u * pts; int i0 = Math.Min((int)fi, pts - 1); float t = fi - i0;
                float c = ys[i0] * (1 - t) + ys[i0 + 1] * t;
                float taper = (float)Math.Pow(Math.Sin(Math.PI * Math.Pow(u, 0.7)), 0.8);
                float wid = 0.05f + 0.16f * taper;
                float d = Math.Abs(v - c) / wid;
                float core = 1 - Smooth(0.5f, 1.0f, d);
                float glow = (float)Math.Exp(-Math.Abs(v - c) / 0.18f) * 0.35f;
                float edge = Smooth(0f, 0.06f, u) * Smooth(0f, 0.06f, 1 - u) * Smooth(0f, 0.12f, 1 - Math.Abs(v));
                float a = C01(Math.Max(core, glow) * taper) * edge;
                int A = (int)(a * 255);
                bmp.SetPixel(x, r * cellH + y, Color.FromArgb(A, 235, 242, 255));
            }
        }
        bmp.Save(path, ImageFormat.Png);
    }
}
'@
Add-Type -TypeDefinition $src -ReferencedAssemblies System.Drawing
$OutDir = (Resolve-Path $OutDir).Path
[StreakTex]::Sheet((Join-Path $OutDir 'GL_Streak.png'), 512, 64, 4, 11)
'done'
