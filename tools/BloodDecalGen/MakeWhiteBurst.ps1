# 血殺の白い飛沫の画像を作る(data/UI/bloodUI/white_burst_0～2.png)
# 中心が当たった点で、+X(右)へ飛び散る。細い棘・破片・霧状の粒でできている
# 色は全部白で、形はアルファだけ。このファイルはBOM付きUTF-8で保存すること
param(
    [string]$OutDir = (Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\bloodUI")
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class WhiteBurstGen
{
    const int Size = 1024;
    const float Center = Size * 0.5f;
    const float Radius = Size * 0.49f;

    static double Rand(Random rnd, double min, double max)
    {
        return min + (max - min) * rnd.NextDouble();
    }

    // 真ん中に集まる乱数(-1～1)
    static double RandCenter(Random rnd)
    {
        return (rnd.NextDouble() + rnd.NextDouble() + rnd.NextDouble()) / 1.5 - 1.0;
    }

    // 飛ぶ向き。fanRate の割合で +X の扇、残りは全方向
    static double RandAngle(Random rnd, double fanRate, double fanWidth)
    {
        if (rnd.NextDouble() < fanRate) return RandCenter(rnd) * fanWidth;
        return Rand(rnd, 0, Math.PI * 2);
    }

    static PointF Polar(double angle, double dist)
    {
        return new PointF((float)(Center + Math.Cos(angle) * dist * Radius), (float)(Center + Math.Sin(angle) * dist * Radius));
    }

    static void Drop(Graphics g, Brush brush, double angle, double dist, double r, double stretch)
    {
        var state = g.Save();
        PointF p = Polar(angle, dist);
        g.TranslateTransform(p.X, p.Y);
        g.RotateTransform((float)(angle * 180.0 / Math.PI));
        float rx = (float)(r * Radius * stretch);
        float ry = (float)(r * Radius);
        g.FillEllipse(brush, -rx, -ry, rx * 2, ry * 2);
        g.Restore(state);
    }

    // ぎざぎざの破片
    static void Shard(Graphics g, Brush brush, Random rnd, double angle, double dist, double size, double stretch)
    {
        int num = rnd.Next(5, 10);
        PointF c = Polar(angle, dist);
        double ca = Math.Cos(angle), sa = Math.Sin(angle);
        var pts = new PointF[num];
        for (int i = 0; i < num; i++)
        {
            double t = Math.PI * 2 * i / num + Rand(rnd, -0.25, 0.25);
            double r = size * Radius * Rand(rnd, 0.35, 1.25);
            double lx = Math.Cos(t) * r * stretch;
            double ly = Math.Sin(t) * r;
            pts[i] = new PointF((float)(c.X + lx * ca - ly * sa), (float)(c.Y + lx * sa + ly * ca));
        }
        g.FillPolygon(brush, pts);
    }

    // 先のとがった棘。幅を少しずつ乱す
    static void Spike(Graphics g, Brush brush, Random rnd, double angle, double start, double length, double width)
    {
        const int num = 14;
        double bend = Rand(rnd, -0.07, 0.07);
        var pts = new PointF[num * 2];
        for (int i = 0; i < num; i++)
        {
            double t = (double)i / (num - 1);
            double dist = start + length * t;
            double half = width * Math.Pow(1.0 - t, 1.25) * Rand(rnd, 0.7, 1.3) + 0.0007;
            double a = angle + bend * t * t;
            double side = half / Math.Max(dist, 0.04);
            pts[i] = Polar(a - side, dist);
            pts[num * 2 - 1 - i] = Polar(a + side, dist);
        }
        g.FillPolygon(brush, pts);
    }

    // かすれて途切れる筋
    static void BrokenStreak(Graphics g, Brush brush, Random rnd, double angle, double start, double end, double width)
    {
        double d = start;
        while (d < end)
        {
            double len = Rand(rnd, 0.02, 0.11);
            Spike(g, brush, rnd, angle + Rand(rnd, -0.006, 0.006), d, len, width * (1.0 - 0.6 * (d - start) / (end - start)));
            d += len + Rand(rnd, 0.004, 0.04);
        }
    }

    static float[] Alpha(Bitmap bmp)
    {
        var data = bmp.LockBits(new Rectangle(0, 0, Size, Size), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        byte[] bytes = new byte[data.Stride * Size];
        Marshal.Copy(data.Scan0, bytes, 0, bytes.Length);
        bmp.UnlockBits(data);
        var result = new float[Size * Size];
        for (int i = 0; i < result.Length; i++) result[i] = bytes[i * 4 + 1] / 255.0f;
        return result;
    }

    static float[] Soften(float[] src)
    {
        var a = new float[src.Length];
        for (int y = 0; y < Size; y++)
        {
            for (int x = 0; x < Size; x++)
            {
                int l = Math.Max(0, x - 1), r = Math.Min(Size - 1, x + 1);
                int u = Math.Max(0, y - 1), d = Math.Min(Size - 1, y + 1);
                a[y * Size + x] = (src[y * Size + x] * 4 + src[y * Size + l] + src[y * Size + r] + src[u * Size + x] + src[d * Size + x]) / 8.0f;
            }
        }
        return a;
    }

    static float[] Noise(Random rnd, int cell)
    {
        int n = Size / cell + 2;
        var grid = new float[n * n];
        for (int i = 0; i < grid.Length; i++) grid[i] = (float)rnd.NextDouble();
        var result = new float[Size * Size];
        for (int y = 0; y < Size; y++)
        {
            for (int x = 0; x < Size; x++)
            {
                float fx = (float)x / cell, fy = (float)y / cell;
                int ix = (int)fx, iy = (int)fy;
                float tx = fx - ix, ty = fy - iy;
                tx = tx * tx * (3 - 2 * tx);
                ty = ty * ty * (3 - 2 * ty);
                float top = grid[iy * n + ix] * (1 - tx) + grid[iy * n + ix + 1] * tx;
                float bottom = grid[(iy + 1) * n + ix] * (1 - tx) + grid[(iy + 1) * n + ix + 1] * tx;
                result[y * Size + x] = top * (1 - ty) + bottom * ty;
            }
        }
        return result;
    }

    public static void Make(string path, int seed)
    {
        var rnd = new Random(seed);
        using (var main = new Bitmap(Size, Size, PixelFormat.Format32bppArgb))
        using (var fine = new Bitmap(Size, Size, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(main))
        using (var f = Graphics.FromImage(fine))
        using (var brush = new SolidBrush(Color.White))
        {
            g.Clear(Color.Black);
            f.Clear(Color.Black);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            f.SmoothingMode = SmoothingMode.AntiAlias;

            // 芯(小さい破片の集まり)
            for (int i = 0; i < 320; i++)
            {
                Shard(g, brush, rnd, RandAngle(rnd, 0.35, 1.2), Math.Pow(rnd.NextDouble(), 0.75) * 0.13, Rand(rnd, 0.012, 0.032), Rand(rnd, 1.0, 2.2));
            }

            // 芯から出る細かい棘(長いものは少ない)
            for (int i = 0; i < 620; i++)
            {
                bool isFan = rnd.NextDouble() < 0.72;
                double a = isFan ? RandCenter(rnd) * 1.15 : Rand(rnd, 0, Math.PI * 2);
                double len = 0.05 + Math.Pow(rnd.NextDouble(), 2.8) * (isFan ? 0.5 : 0.17);
                len *= 1.0 - 0.3 * Math.Min(1.0, Math.Abs(a));
                Spike(g, brush, rnd, a, Rand(rnd, 0.03, 0.13), len, Rand(rnd, 0.003, 0.015));
            }

            // かすれた筋
            for (int i = 0; i < 52; i++)
            {
                BrokenStreak(g, brush, rnd, RandCenter(rnd) * 0.6, Rand(rnd, 0.14, 0.34), Rand(rnd, 0.5, 0.97), Rand(rnd, 0.002, 0.0065));
            }

            // 破片(内側ほど大きい)
            for (int i = 0; i < 520; i++)
            {
                double d = 0.12 + Math.Pow(rnd.NextDouble(), 1.25) * 0.82;
                double size = (0.0028 + 0.012 * Math.Pow(rnd.NextDouble(), 2.6)) * (1.15 - d);
                Shard(g, brush, rnd, RandAngle(rnd, 0.8, 1.15), d, size, Rand(rnd, 1.0, 2.6));
            }

            // 中くらいの粒
            for (int i = 0; i < 46; i++)
            {
                Drop(g, brush, RandCenter(rnd) * 1.1, Rand(rnd, 0.22, 0.82), Rand(rnd, 0.0045, 0.0125), Rand(rnd, 1.1, 2.6));
            }

            // 霧状の粒。ノイズで濃淡をつける
            float[] cluster = Noise(rnd, 54);
            for (int i = 0; i < 16000; i++)
            {
                bool isFan = rnd.NextDouble() < 0.8;
                double a = isFan ? RandCenter(rnd) * 1.25 : Rand(rnd, 0, Math.PI * 2);
                double d = 0.06 + Math.Pow(rnd.NextDouble(), 2.0) * (isFan ? 0.92 : 0.5);
                PointF p = Polar(a, d);
                int px = Math.Max(0, Math.Min(Size - 1, (int)p.X));
                int py = Math.Max(0, Math.Min(Size - 1, (int)p.Y));
                // 濃淡はつけるが、はっきりした境目は作らない
                double keep = Math.Max(0.0, Math.Min(1.0, (cluster[py * Size + px] - 0.2) / 0.5));
                if (rnd.NextDouble() > keep * keep * (3.0 - 2.0 * keep) * 0.9 + 0.1) continue;
                Drop(f, brush, a, d, Rand(rnd, 0.0007, 0.0029), Rand(rnd, 1.0, 3.0));
            }

            // 外側ほど強くノイズで削って、かすれさせる
            float[] body = Soften(Alpha(main));
            float[] noiseA = Noise(rnd, 7);
            float[] noiseB = Noise(rnd, 3);
            float[] fineA = Alpha(fine);
            byte[] outBytes = new byte[Size * Size * 4];
            for (int y = 0; y < Size; y++)
            {
                for (int x = 0; x < Size; x++)
                {
                    int i = y * Size + x;
                    double dx = (x - Center) / Radius, dy = (y - Center) / Radius;
                    double dist = Math.Min(1.0, Math.Sqrt(dx * dx + dy * dy) / 0.55);
                    float amount = 0.2f + 0.54f * (float)dist;
                    float v = body[i] - ((noiseA[i] - 0.5f) * 0.6f + (noiseB[i] - 0.5f) * 0.4f + 0.04f) * amount * 2.0f;
                    float t = Math.Max(0f, Math.Min(1f, (v - 0.42f) / 0.16f));
                    t = t * t * (3 - 2 * t);
                    if (body[i] < 0.03f) t = 0f;
                    float a = Math.Max(t, fineA[i]);
                    outBytes[i * 4] = 255;
                    outBytes[i * 4 + 1] = 255;
                    outBytes[i * 4 + 2] = 255;
                    outBytes[i * 4 + 3] = (byte)(a * 255.0f + 0.5f);
                }
            }

            using (var result = new Bitmap(Size, Size, PixelFormat.Format32bppArgb))
            {
                var data = result.LockBits(new Rectangle(0, 0, Size, Size), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                Marshal.Copy(outBytes, 0, data.Scan0, outBytes.Length);
                result.UnlockBits(data);
                result.Save(path, ImageFormat.Png);
            }
        }
    }
}
"@

$dir = [IO.Path]::GetFullPath($OutDir)
for ($i = 0; $i -lt 3; $i++) {
    $path = Join-Path $dir ("white_burst_{0}.png" -f $i)
    [WhiteBurstGen]::Make($path, 8100 + $i * 37)
    Write-Host "wrote $path"
}
