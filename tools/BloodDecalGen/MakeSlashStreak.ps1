# 血殺の斬撃の線の画像を作る(data/UI/bloodUI/slash_streak.png)
# +X へ抜ける、かすれた筆の線。色は全部白で、形はアルファだけ
# このファイルはBOM付きUTF-8で保存すること
param(
    [string]$OutPath = (Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\bloodUI\slash_streak.png")
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class SlashStreakGen
{
    const int Width = 1280;
    const int Height = 96;
    const float PeakU = 0.28f;

    static float Smooth(float edge0, float edge1, float x)
    {
        float t = Math.Max(0f, Math.Min(1f, (x - edge0) / (edge1 - edge0)));
        return t * t * (3f - 2f * t);
    }

    static float[] Noise(Random rnd, int cellX, int cellY)
    {
        int nx = Width / cellX + 2;
        int ny = Height / cellY + 2;
        var grid = new float[nx * ny];
        for (int i = 0; i < grid.Length; i++) grid[i] = (float)rnd.NextDouble();
        var result = new float[Width * Height];
        for (int y = 0; y < Height; y++)
        {
            for (int x = 0; x < Width; x++)
            {
                float fx = (float)x / cellX, fy = (float)y / cellY;
                int ix = (int)fx, iy = (int)fy;
                float tx = fx - ix, ty = fy - iy;
                tx = tx * tx * (3 - 2 * tx);
                ty = ty * ty * (3 - 2 * ty);
                float top = grid[iy * nx + ix] * (1 - tx) + grid[iy * nx + ix + 1] * tx;
                float bottom = grid[(iy + 1) * nx + ix] * (1 - tx) + grid[(iy + 1) * nx + ix + 1] * tx;
                result[y * Width + x] = top * (1 - ty) + bottom * ty;
            }
        }
        return result;
    }

    public static void Make(string path, int seed)
    {
        var rnd = new Random(seed);
        float[] edge = Noise(rnd, 60, 6);
        float[] streak = Noise(rnd, 130, 4);

        byte[] bytes = new byte[Width * Height * 4];
        for (int y = 0; y < Height; y++)
        {
            for (int x = 0; x < Width; x++)
            {
                int i = y * Width + x;
                float u = (float)x / (Width - 1);
                float dy = Math.Abs(y - (Height - 1) * 0.5f) / (Height * 0.5f);

                // 入りは短く太くなり、先へ長く細くなる
                float p = u < PeakU ? (float)Math.Pow(u / PeakU, 0.6) : (float)Math.Pow(1.0 - (u - PeakU) / (1.0 - PeakU), 1.05);
                float hw = p * 0.88f * (1f + (edge[i] - 0.5f) * 0.35f);

                float a = 0f;
                if (hw > 0.02f)
                {
                    float body = 1f - Smooth(hw - 0.12f, hw, dy);
                    // 先へいくほどかすれる
                    float threshold = 0.1f + 0.45f * (float)Math.Pow(u, 1.6);
                    float keep = Smooth(threshold - 0.04f, threshold + 0.04f, streak[i]);
                    a = body * keep;
                }

                bytes[i * 4] = 255;
                bytes[i * 4 + 1] = 255;
                bytes[i * 4 + 2] = 255;
                bytes[i * 4 + 3] = (byte)(Math.Min(1f, a) * 255f + 0.5f);
            }
        }

        using (var bmp = new Bitmap(Width, Height, PixelFormat.Format32bppArgb))
        {
            var data = bmp.LockBits(new Rectangle(0, 0, Width, Height), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
            Marshal.Copy(bytes, 0, data.Scan0, bytes.Length);
            bmp.UnlockBits(data);
            bmp.Save(path, ImageFormat.Png);
        }
    }
}
"@

$path = [IO.Path]::GetFullPath($OutPath)
[SlashStreakGen]::Make($path, 6031)
Write-Host "wrote $path"
