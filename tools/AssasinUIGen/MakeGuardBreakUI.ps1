# ガードブレイクUIのPNGを作る(グラデーションと光)
# 実行: powershell -ExecutionPolicy Bypass -File MakeGuardBreakUI.ps1
# 出力: 2026_winter/data/UI/GuardBreak/ blade.png blade_glow.png glow.png disc.png button.png band.png
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class GuardBreakImage
{
    static double Clamp(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
    static double Smooth(double a, double b, double v) { double t = Clamp((v - a) / (b - a)); return t * t * (3 - 2 * t); }
    static double Mix(double a, double b, double t) { return a + (b - a) * t; }
    static double[] Mix(double[] a, double[] b, double t) { return new double[] { Mix(a[0], b[0], t), Mix(a[1], b[1], t), Mix(a[2], b[2], t) }; }
    static void Set(Bitmap b, int x, int y, double[] c, double a)
    {
        b.SetPixel(x, y, Color.FromArgb((int)(Clamp(a) * 255), (int)Clamp(c[0] / 255) == 1 ? 255 : (int)c[0], (int)Clamp(c[1] / 255) == 1 ? 255 : (int)c[1], (int)Clamp(c[2] / 255) == 1 ? 255 : (int)c[2]));
    }
    static readonly double[] Deep = { 6, 34, 120 };
    static readonly double[] Azure = { 24, 132, 255 };
    static readonly double[] Sky = { 110, 212, 255 };
    static readonly double[] Ice = { 214, 244, 255 };

    public static void Blade(string path)
    {
        int w = 256, h = 40;
        var b = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        {
            double u = Math.Abs((x + .5) / w * 2 - 1), v = Math.Abs((y + .5) / h * 2 - 1);
            double half = (1 - u) * .92;
            if (half <= 0) { Set(b, x, y, Azure, 0); continue; }
            double d = v / half;
            double[] c = Mix(Ice, Sky, Smooth(0, .3, d));
            c = Mix(c, Azure, Smooth(.3, .68, d));
            c = Mix(c, Deep, Smooth(.72, .97, d));
            c = Mix(c, Deep, Smooth(.55, 1, u) * .55);
            Set(b, x, y, c, 1 - Smooth(.96, 1.04, d));
        }
        b.Save(path, ImageFormat.Png); b.Dispose();
    }
    public static void BladeGlow(string path)
    {
        int w = 256, h = 40;
        var b = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        {
            double u = Math.Abs((x + .5) / w * 2 - 1), v = Math.Abs((y + .5) / h * 2 - 1);
            double a = Math.Pow(Clamp(1 - v), 2.2) * Math.Pow(Clamp(1 - u * u), 1.2);
            Set(b, x, y, Azure, a);
        }
        b.Save(path, ImageFormat.Png); b.Dispose();
    }
    public static void Glow(string path)
    {
        int s = 256;
        var b = new Bitmap(s, s, PixelFormat.Format32bppArgb);
        for (int y = 0; y < s; y++) for (int x = 0; x < s; x++)
        {
            double dx = (x + .5) / s * 2 - 1, dy = (y + .5) / s * 2 - 1;
            double d = Math.Sqrt(dx * dx + dy * dy);
            Set(b, x, y, Azure, Math.Pow(Clamp(1 - d), 2.0));
        }
        b.Save(path, ImageFormat.Png); b.Dispose();
    }
    public static void Disc(string path)
    {
        int s = 256;
        var b = new Bitmap(s, s, PixelFormat.Format32bppArgb);
        for (int y = 0; y < s; y++) for (int x = 0; x < s; x++)
        {
            double dx = (x + .5) / s * 2 - 1, dy = (y + .5) / s * 2 - 1;
            double d = Math.Sqrt(dx * dx + dy * dy) / .96;
            double[] c = Mix(new double[] { 16, 78, 190 }, new double[] { 6, 26, 92 }, Smooth(.1, .92, d));
            c = Mix(c, Sky, Smooth(.91, .97, d) * .9);
            Set(b, x, y, c, (1 - Smooth(.985, 1.01, d)) * .94);
        }
        b.Save(path, ImageFormat.Png); b.Dispose();
    }
    public static void Button(string path)
    {
        int s = 128;
        var b = new Bitmap(s, s, PixelFormat.Format32bppArgb);
        for (int y = 0; y < s; y++) for (int x = 0; x < s; x++)
        {
            double dx = (x + .5) / s * 2 - 1, dy = (y + .5) / s * 2 - 1;
            double d = Math.Sqrt(dx * dx + dy * dy) / .94;
            double[] c = Mix(Sky, Azure, Smooth(-.8, .05, dy));
            c = Mix(c, new double[] { 10, 74, 214 }, Smooth(.05, .85, dy));
            double hx = dx / .72, hy = (dy + .48) / .36;
            c = Mix(c, Ice, (1 - Smooth(.55, 1.05, Math.Sqrt(hx * hx + hy * hy))) * .5);
            c = Mix(c, Ice, Smooth(.9, .97, d) * .85);
            Set(b, x, y, c, 1 - Smooth(.98, 1.02, d));
        }
        b.Save(path, ImageFormat.Png); b.Dispose();
    }
    public static void Band(string path)
    {
        int w = 512, h = 64;
        var b = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        {
            double u = (x + .5) / w, v = (y + .5) / h;
            double[] c = Mix(new double[] { 18, 112, 240 }, new double[] { 8, 60, 176 }, Smooth(0, .55, u));
            c = Mix(c, Deep, Smooth(.55, 1, u));
            c = Mix(c, Azure, (1 - Smooth(0, .5, v)) * .45);
            double s = Math.Sin((x + y * 1.4) * .045);
            c = Mix(c, Sky, Smooth(.82, 1, s) * .14 * (1 - u));
            double line = Math.Max(1 - Smooth(.0, .055, Math.Abs(v - .07)), 1 - Smooth(.0, .055, Math.Abs(v - .93)));
            c = Mix(c, Sky, line * (1 - Smooth(.45, 1, u)) * .95);
            double edge = Smooth(0, .06, v) * (1 - Smooth(.94, 1, v));
            Set(b, x, y, c, edge * .96 * (1 - Smooth(.64, 1, u)));
        }
        b.Save(path, ImageFormat.Png); b.Dispose();
    }
}
"@

$out = Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\GuardBreak"
New-Item -ItemType Directory -Force $out | Out-Null
$out = (Resolve-Path $out).Path

[GuardBreakImage]::Blade((Join-Path $out "blade.png"))
[GuardBreakImage]::BladeGlow((Join-Path $out "blade_glow.png"))
[GuardBreakImage]::Glow((Join-Path $out "glow.png"))
[GuardBreakImage]::Disc((Join-Path $out "disc.png"))
[GuardBreakImage]::Button((Join-Path $out "button.png"))
[GuardBreakImage]::Band((Join-Path $out "band.png"))
Write-Host "saved $out"
