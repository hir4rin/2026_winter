# 床に残る血の跡の画像を作る(data/Decal/BloodDecal.png)
# 512pxのマスを2x2に並べる。上の2マス=血だまり、下の2マス=飛沫(上向きに飛ぶ)
# 色はゲームで掛けるので白ベース。このファイルはBOM付きUTF-8で保存すること
param(
    [string]$OutPath = (Join-Path $PSScriptRoot "..\..\2026_winter\data\Decal\BloodDecal.png")
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;

public static class BloodDecalGen
{
    const int CellSize = 512;

    class Ball
    {
        public double X, Y, R, DirX, DirY, Stretch;
    }

    static Ball Round(double x, double y, double r)
    {
        return new Ball { X = x, Y = y, R = r, DirX = 1, DirY = 0, Stretch = 1 };
    }

    static double Rand(Random rnd, double min, double max)
    {
        return min + (max - min) * rnd.NextDouble();
    }

    static List<Ball> MakePool(Random rnd)
    {
        var balls = new List<Ball>();
        balls.Add(Round(0, 0, 0.24));
        int lobeNum = 10;
        for (int i = 0; i < lobeNum; i++)
        {
            double a = Math.PI * 2 * i / lobeNum + Rand(rnd, -0.2, 0.2);
            double d = Rand(rnd, 0.18, 0.42);
            balls.Add(Round(Math.Cos(a) * d, Math.Sin(a) * d, Rand(rnd, 0.06, 0.15)));
        }
        // 外へ伸びる筋
        for (int i = 0; i < 18; i++)
        {
            double a = Rand(rnd, 0, Math.PI * 2);
            double dx = Math.Cos(a);
            double dy = Math.Sin(a);
            double d = Rand(rnd, 0.34, 0.5);
            double r = Rand(rnd, 0.026, 0.05);
            double end = Rand(rnd, 0.62, 0.92);
            while (d < end)
            {
                balls.Add(new Ball { X = dx * d, Y = dy * d, R = r, DirX = dx, DirY = dy, Stretch = Rand(rnd, 2.2, 3.6) });
                d += Rand(rnd, 0.06, 0.11);
                r *= 0.78;
            }
        }
        // 離れた粒
        for (int i = 0; i < 34; i++)
        {
            double a = Rand(rnd, 0, Math.PI * 2);
            double d = Rand(rnd, 0.55, 0.9);
            balls.Add(Round(Math.Cos(a) * d, Math.Sin(a) * d, Rand(rnd, 0.008, 0.026)));
        }
        return balls;
    }

    static List<Ball> MakeSplash(Random rnd)
    {
        var balls = new List<Ball>();
        double coreY = 0.42;
        balls.Add(Round(0, coreY, 0.09));
        balls.Add(Round(Rand(rnd, -0.08, 0.08), coreY - 0.08, 0.06));
        int streakNum = 20;
        for (int i = 0; i < streakNum; i++)
        {
            double a = -Math.PI / 2 + Rand(rnd, -0.6, 0.6);
            double dx = Math.Cos(a);
            double dy = Math.Sin(a);
            double d = Rand(rnd, 0.16, 0.30);
            double r = Rand(rnd, 0.012, 0.03);
            double end = Rand(rnd, 0.6, 1.28);
            while (d < end)
            {
                double y = coreY + dy * d;
                double x = dx * d;
                if (x * x + y * y > 0.86 * 0.86) break;
                balls.Add(new Ball { X = x, Y = y, R = r, DirX = dx, DirY = dy, Stretch = Rand(rnd, 3.0, 5.2) });
                d += Rand(rnd, 0.09, 0.2);
                r *= Rand(rnd, 0.72, 0.95);
            }
        }
        return balls;
    }

    static double Smooth(double a, double b, double x)
    {
        double t = Math.Max(0.0, Math.Min(1.0, (x - a) / (b - a)));
        return t * t * (3 - 2 * t);
    }

    static double Field(List<Ball> balls, double x, double y)
    {
        double sum = 0;
        foreach (var b in balls)
        {
            double px = x - b.X;
            double py = y - b.Y;
            double along = (px * b.DirX + py * b.DirY) / b.Stretch;
            double across = -px * b.DirY + py * b.DirX;
            double range = b.R * 2.2;
            double t = 1.0 - (along * along + across * across) / (range * range);
            if (t > 0) sum += t * t;
        }
        return sum;
    }

    static void DrawCell(Bitmap bmp, int cellX, int cellY, List<Ball> balls, Random rnd)
    {
        double p1 = Rand(rnd, 0, 6.28), p2 = Rand(rnd, 0, 6.28), p3 = Rand(rnd, 0, 6.28), p4 = Rand(rnd, 0, 6.28);
        for (int py = 0; py < CellSize; py++)
        {
            for (int px = 0; px < CellSize; px++)
            {
                double x = (px + 0.5) / CellSize * 2 - 1;
                double y = (py + 0.5) / CellSize * 2 - 1;
                // 縁をでこぼこにする
                double wx = x + 0.026 * Math.Sin(y * 13 + p1) + 0.014 * Math.Sin(y * 37 + p2);
                double wy = y + 0.026 * Math.Sin(x * 15 + p3) + 0.014 * Math.Sin(x * 41 + p4);
                double f = Field(balls, wx, wy);
                double alpha = Smooth(0.56, 0.7, f);
                // マスの端は必ず透明
                double dist = Math.Sqrt(x * x + y * y);
                alpha *= 1 - Smooth(0.92, 0.985, dist);
                // 中ほど濃く、縁は少し明るい
                double depth = Smooth(0.6, 1.0, f);
                int shade = (int)(255 * (1.0 - 0.28 * depth));
                bmp.SetPixel(cellX * CellSize + px, cellY * CellSize + py,
                    Color.FromArgb((int)(alpha * 255 + 0.5), shade, shade, shade));
            }
        }
    }

    public static void Make(string path)
    {
        var rnd = new Random(20261108);
        using (var bmp = new Bitmap(CellSize * 2, CellSize * 2, PixelFormat.Format32bppArgb))
        {
            DrawCell(bmp, 0, 0, MakePool(rnd), rnd);
            DrawCell(bmp, 1, 0, MakePool(rnd), rnd);
            DrawCell(bmp, 0, 1, MakeSplash(rnd), rnd);
            DrawCell(bmp, 1, 1, MakeSplash(rnd), rnd);
            bmp.Save(path, ImageFormat.Png);
        }
    }
}
"@

$fullPath = [System.IO.Path]::GetFullPath($OutPath)
[BloodDecalGen]::Make($fullPath)
Write-Output "saved: $fullPath"
