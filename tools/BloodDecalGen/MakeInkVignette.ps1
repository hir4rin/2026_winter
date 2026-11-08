# 血殺の間に画面のふちを暗くする墨のビネットを作る(data/UI/bloodUI/ink_vignette.png)
# 色は全部黒で、濃さはアルファだけ。このファイルはBOM付きUTF-8で保存すること
param(
    [string]$OutPath = (Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\bloodUI\ink_vignette.png")
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class InkVignetteGen
{
    const int Width = 1280;
    const int Height = 720;

    static float Smooth(float edge0, float edge1, float x)
    {
        float t = Math.Max(0f, Math.Min(1f, (x - edge0) / (edge1 - edge0)));
        return t * t * (3f - 2f * t);
    }

    static float[] Noise(Random rnd, int cell)
    {
        int nx = Width / cell + 2;
        int ny = Height / cell + 2;
        var grid = new float[nx * ny];
        for (int i = 0; i < grid.Length; i++) grid[i] = (float)rnd.NextDouble();
        var result = new float[Width * Height];
        for (int y = 0; y < Height; y++)
        {
            for (int x = 0; x < Width; x++)
            {
                float fx = (float)x / cell, fy = (float)y / cell;
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
        // 大きな雲の形 → 細かいにじみ
        float[] n0 = Noise(rnd, 170);
        float[] n1 = Noise(rnd, 84);
        float[] n2 = Noise(rnd, 38);
        float[] n3 = Noise(rnd, 17);

        byte[] bytes = new byte[Width * Height * 4];
        for (int y = 0; y < Height; y++)
        {
            for (int x = 0; x < Width; x++)
            {
                int i = y * Width + x;
                float nx = (x - Width * 0.5f) / (Width * 0.5f);
                float ny = (y - Height * 0.5f) / (Height * 0.5f);
                // 四角い画面に沿わせる(丸と四角の間の距離)
                float ax = Math.Abs(nx), ay = Math.Abs(ny);
                float round = (float)Math.Sqrt(nx * nx + ny * ny) / 1.414f;
                float box = Math.Max(ax, ay);
                float dist = round * 0.55f + box * 0.45f;

                float fbm = n0[i] * 0.5f + n1[i] * 0.28f + n2[i] * 0.15f + n3[i] * 0.07f;
                float shaped = dist + (fbm - 0.5f) * 0.62f;

                // 中央は必ず空ける
                float clear = Smooth(0.34f, 0.56f, dist);
                float a = Smooth(0.55f, 0.90f, shaped) * clear;

                bytes[i * 4] = 0;
                bytes[i * 4 + 1] = 0;
                bytes[i * 4 + 2] = 0;
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
[InkVignetteGen]::Make($path, 4217)
Write-Host "wrote $path"
