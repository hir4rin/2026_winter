# ミサイル用のテクスチャ(白＋アルファ。色はノードで付ける)
param([string]$OutDir = "$PSScriptRoot\..\..\2026_winter\data\Effect\Kingmech\Texture")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class Fx
{
    static double C01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
    static double Band(double r, double c, double w, double px) { return C01((w * 0.5 - Math.Abs(r - c)) / px + 0.5); }
    static double Hash(int x, int y, int s) { uint h = (uint)(x * 374761393 + y * 668265263 + s * 1442695040); h = (h ^ (h >> 13)) * 1274126177; return ((h ^ (h >> 16)) & 0xffff) / 65535.0; }
    static double Noise(double x, double y, int s)
    {
        int xi = (int)Math.Floor(x), yi = (int)Math.Floor(y); double fx = x - xi, fy = y - yi;
        fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
        double a = Hash(xi, yi, s), b = Hash(xi + 1, yi, s), c = Hash(xi, yi + 1, s), d = Hash(xi + 1, yi + 1, s);
        return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
    }
    static double Fbm(double x, double y, int s) { double v = 0, amp = 0.5; for (int i = 0; i < 4; i++) { v += Noise(x, y, s + i) * amp; x *= 2; y *= 2; amp *= 0.5; } return v; }
    static void Save(string path, int size, Func<double, double, double> alpha)
    {
        var bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        for (int y = 0; y < size; y++) for (int x = 0; x < size; x++)
        {
            double u = (x + 0.5) / size * 2 - 1, v = (y + 0.5) / size * 2 - 1;
            int a = (int)(C01(alpha(u, v)) * 255);
            bmp.SetPixel(x, y, Color.FromArgb(a, 255, 255, 255));
        }
        bmp.Save(path, ImageFormat.Png);
    }
    public static void Run(string dir)
    {
        int n = 512; double px = 2.0 / n;
        // 予告の外周: 太い輪＋内側の細い輪＋目盛り
        Save(dir + "\\WarnRing.png", n, (u, v) =>
        {
            double r = Math.Sqrt(u * u + v * v), ang = Math.Atan2(v, u);
            double ring = Band(r, 0.93, 0.045, px) + Band(r, 0.84, 0.012, px) * 0.8;
            double tick = Math.Abs(((ang / (2 * Math.PI) * 32) % 1 + 1) % 1 - 0.5) < 0.12 ? Band(r, 0.885, 0.05, px) * 0.9 : 0;
            double glow = Math.Exp(-Math.Abs(r - 0.93) / 0.03) * 0.35 * (r < 0.99 ? 1 : 0);
            return Math.Max(Math.Max(ring, tick), glow) * C01((0.995 - r) / px);
        });
        // 予告の中の塗り: 縁ほど濃い
        Save(dir + "\\WarnFill.png", n, (u, v) =>
        {
            double r = Math.Sqrt(u * u + v * v);
            return (0.18 + 0.6 * Math.Pow(r, 4)) * C01((0.97 - r) / px) + Band(r, 0.955, 0.03, px) * 0.6;
        });
        // 回る破線の輪(三角の矢印つき)
        Save(dir + "\\WarnDash.png", n, (u, v) =>
        {
            double r = Math.Sqrt(u * u + v * v), ang = Math.Atan2(v, u);
            double t = ((ang / (2 * Math.PI) * 12) % 1 + 1) % 1;
            double dash = t < 0.55 ? Band(r, 0.9, 0.05, px) : 0;
            double arrow = (t > 0.68 && t < 0.88) ? C01((0.06 - Math.Abs(r - 0.9) - Math.Abs(t - 0.68) * 0.25) / px) : 0;
            return Math.Max(dash, arrow);
        });
        // 衝撃波の輪: 外側が鋭く、内側へなだらか
        Save(dir + "\\ShockRing.png", n, (u, v) =>
        {
            double r = Math.Sqrt(u * u + v * v);
            if (r > 0.95) return 0;
            double outer = C01((0.95 - r) / 0.02);
            double inner = Math.Pow(C01((r - 0.45) / 0.5), 2.2);
            return outer * inner;
        });
        // 焦げ跡: ぎざぎざの暗い円
        Save(dir + "\\Scorch.png", n, (u, v) =>
        {
            double r = Math.Sqrt(u * u + v * v), ang = Math.Atan2(v, u);
            double edge = 0.62 + 0.22 * Fbm(Math.Cos(ang) * 3 + 10, Math.Sin(ang) * 3 + 10, 7);
            double body = C01((edge - r) / 0.12);
            double spots = 0.65 + 0.35 * Fbm(u * 6 + 3, v * 6 + 3, 3);
            return body * spots * C01((0.98 - r) / px);
        });
        // 火の粉の筋: 両端が画像の手前で0
        Save(dir + "\\SparkStreak.png", 128, (u, v) =>
        {
            double along = C01((0.9 - Math.Abs(u)) / 0.25);
            along *= u < 0 ? 1 : C01((0.9 - u) / 0.7);
            return along * Math.Exp(-v * v / 0.02);
        });
    }
}
"@
New-Item -ItemType Directory -Force $OutDir | Out-Null
[Fx]::Run([System.IO.Path]::GetFullPath($OutDir))
"done $OutDir"
