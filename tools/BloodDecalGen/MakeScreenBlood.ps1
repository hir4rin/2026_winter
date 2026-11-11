# 画面(レンズ)に付く血の画像を作る(data/UI/ScreenBlood/)
# 血殺の血のUI(splash2_UI・white_burst)と同じ、べた塗りでふちがくっきりした墨の飛沫
# splat_0～3.png = 冠のような塊から+X(右)へ飛び散る飛沫、spray_0～2.png = 尾を引く粒が+Xへ帯になって並ぶ
# 色は全部白で、形はアルファだけ(色はゲームで掛ける)。このファイルはBOM付きUTF-8で保存すること
param(
    [string]$OutDir = (Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\ScreenBlood")
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class ScreenBloodGen
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

    // 正規化した座標(-1～1)を画像の座標へ
    static PointF ToPixel(double x, double y)
    {
        return new PointF((float)(Center + x * Radius), (float)(Center + y * Radius));
    }

    static void Ellipse(Graphics g, Brush brush, double x, double y, double angle, double r, double stretch)
    {
        var state = g.Save();
        PointF p = ToPixel(x, y);
        g.TranslateTransform(p.X, p.Y);
        g.RotateTransform((float)(angle * 180.0 / Math.PI));
        float rx = (float)(r * Radius * stretch);
        float ry = (float)(r * Radius);
        g.FillEllipse(brush, -rx, -ry, rx * 2, ry * 2);
        g.Restore(state);
    }

    // ぎざぎざの破片
    static void Shard(Graphics g, Brush brush, Random rnd, double x, double y, double angle, double size, double stretch)
    {
        int num = rnd.Next(5, 10);
        double ca = Math.Cos(angle), sa = Math.Sin(angle);
        var pts = new PointF[num];
        for (int i = 0; i < num; i++)
        {
            double t = Math.PI * 2 * i / num + Rand(rnd, -0.25, 0.25);
            double r = size * Rand(rnd, 0.4, 1.25);
            double lx = Math.Cos(t) * r * stretch;
            double ly = Math.Sin(t) * r;
            pts[i] = ToPixel(x + lx * ca - ly * sa, y + lx * sa + ly * ca);
        }
        g.FillPolygon(brush, pts);
    }

    // (x,y)から angle の向きへ伸びて先がとがる棘。tipBall なら先に粒
    static void Spike(Graphics g, Brush brush, Random rnd, double x, double y, double angle, double length, double width, bool tipBall)
    {
        const int num = 12;
        double bend = Rand(rnd, -0.15, 0.15);
        var pts = new PointF[num * 2];
        double px = x, py = y;
        for (int i = 0; i < num; i++)
        {
            double t = (double)i / (num - 1);
            double a = angle + bend * t * t;
            double half = width * Math.Pow(1.0 - t, 1.1) * Rand(rnd, 0.8, 1.2) + 0.0008;
            double cx = x + Math.Cos(angle) * length * t + Math.Cos(a + Math.PI / 2) * bend * length * t * t * 0.3;
            double cy = y + Math.Sin(angle) * length * t + Math.Sin(a + Math.PI / 2) * bend * length * t * t * 0.3;
            double nx = -Math.Sin(a), ny = Math.Cos(a);
            pts[i] = ToPixel(cx + nx * half, cy + ny * half);
            pts[num * 2 - 1 - i] = ToPixel(cx - nx * half, cy - ny * half);
            px = cx; py = cy;
        }
        g.FillPolygon(brush, pts);
        if (tipBall) Ellipse(g, brush, px, py, angle, width * Rand(rnd, 0.7, 1.2), Rand(rnd, 1.0, 1.6));
    }

    // 尾を引く粒(頭が angle の向きへ飛び、尾は後ろへ細くなる)
    static void Tadpole(Graphics g, Brush brush, Random rnd, double x, double y, double angle, double r, double tail)
    {
        Ellipse(g, brush, x, y, angle, r, Rand(rnd, 1.05, 1.5));
        if (tail > 0) Spike(g, brush, rnd, x, y, angle + Math.PI + Rand(rnd, -0.08, 0.08), tail, r * 0.85, false);
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

    // ふちを少し削ってかすれさせ、くっきりしたアルファにして保存する
    static void Save(Bitmap main, string path, Random rnd)
    {
        float[] body = Soften(Alpha(main));
        float[] noiseA = Noise(rnd, 6);
        float[] noiseB = Noise(rnd, 3);
        byte[] outBytes = new byte[Size * Size * 4];
        for (int y = 0; y < Size; y++)
        {
            for (int x = 0; x < Size; x++)
            {
                int i = y * Size + x;
                double dx = (x - Center) / Radius, dy = (y - Center) / Radius;
                // 端は必ず透明
                double edge = Math.Sqrt(dx * dx + dy * dy);
                float v = body[i] - ((noiseA[i] - 0.5f) * 0.6f + (noiseB[i] - 0.5f) * 0.4f) * 0.35f;
                float t = Math.Max(0f, Math.Min(1f, (v - 0.42f) / 0.14f));
                t = t * t * (3 - 2 * t);
                if (body[i] < 0.03f || edge > 0.99) t = 0f;
                outBytes[i * 4] = 255;
                outBytes[i * 4 + 1] = 255;
                outBytes[i * 4 + 2] = 255;
                outBytes[i * 4 + 3] = (byte)(t * 255.0f + 0.5f);
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

    // 冠のような塊から+Xへ飛び散る飛沫。塊の中心は(-0.45, 0)
    public static void MakeSplat(string path, int seed)
    {
        var rnd = new Random(seed);
        const double cx = -0.45;
        using (var main = new Bitmap(Size, Size, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(main))
        using (var brush = new SolidBrush(Color.White))
        {
            g.Clear(Color.Black);
            g.SmoothingMode = SmoothingMode.AntiAlias;

            // 塊(いびつな丸の集まり)
            double core = Rand(rnd, 0.17, 0.22);
            Ellipse(g, brush, cx, 0, Rand(rnd, 0, 6.28), core * 0.8, Rand(rnd, 1.0, 1.25));
            for (int i = 0; i < 14; i++)
            {
                double a = Rand(rnd, 0, Math.PI * 2);
                double d = core * Rand(rnd, 0.25, 0.75);
                Ellipse(g, brush, cx + Math.Cos(a) * d, Math.Sin(a) * d, a, core * Rand(rnd, 0.25, 0.5), Rand(rnd, 1.0, 1.6));
            }
            // 冠の棘(全方向、先に粒)
            int crownNum = rnd.Next(22, 30);
            for (int i = 0; i < crownNum; i++)
            {
                double a = Math.PI * 2 * i / crownNum + Rand(rnd, -0.12, 0.12);
                double start = core * Rand(rnd, 0.55, 0.8);
                Spike(g, brush, rnd, cx + Math.Cos(a) * start, Math.Sin(a) * start, a,
                    core * Rand(rnd, 0.35, 1.1), core * Rand(rnd, 0.06, 0.14), rnd.NextDouble() < 0.6);
            }
            // +Xへ伸びる長い棘
            for (int i = 0; i < 9; i++)
            {
                double a = RandCenter(rnd) * 0.55;
                double start = core * 0.7;
                Spike(g, brush, rnd, cx + Math.Cos(a) * start, Math.Sin(a) * start, a,
                    Rand(rnd, 0.3, 0.75), Rand(rnd, 0.012, 0.03), rnd.NextDouble() < 0.5);
            }
            // 尾を引く粒(+Xの扇)
            for (int i = 0; i < 70; i++)
            {
                double a = RandCenter(rnd) * 0.75;
                double d = core + Math.Pow(rnd.NextDouble(), 0.9) * 1.1;
                double x = cx + Math.Cos(a) * d, y = Math.Sin(a) * d;
                if (x * x + y * y > 0.9 * 0.9) continue;
                double r = Rand(rnd, 0.005, 0.022) * (1.3 - 0.6 * d / 1.3);
                Tadpole(g, brush, rnd, x, y, a, r, rnd.NextDouble() < 0.7 ? r * Rand(rnd, 2.5, 7.0) : 0);
            }
            // 小さな破片
            for (int i = 0; i < 160; i++)
            {
                double a = rnd.NextDouble() < 0.75 ? RandCenter(rnd) * 0.9 : Rand(rnd, 0, Math.PI * 2);
                double d = core * 0.9 + Math.Pow(rnd.NextDouble(), 1.4) * 1.0;
                double x = cx + Math.Cos(a) * d, y = Math.Sin(a) * d;
                if (x * x + y * y > 0.9 * 0.9) continue;
                Shard(g, brush, rnd, x, y, a, Rand(rnd, 0.002, 0.008), Rand(rnd, 1.0, 2.2));
            }
            Save(main, path, rnd);
        }
    }

    // 尾を引く粒が+Xへ帯になって並ぶ(splash2_UI のような形)。-X側ほど大きく多い
    public static void MakeSpray(string path, int seed)
    {
        var rnd = new Random(seed);
        using (var main = new Bitmap(Size, Size, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(main))
        using (var brush = new SolidBrush(Color.White))
        {
            g.Clear(Color.Black);
            g.SmoothingMode = SmoothingMode.AntiAlias;

            double bend = Rand(rnd, -0.25, 0.25);
            for (int i = 0; i < 110; i++)
            {
                double u = Math.Pow(rnd.NextDouble(), 1.1);// 0=根元(-X) 1=先(+X)
                double x = -0.82 + u * 1.65;
                double spread = 0.12 + 0.5 * u;
                double y = bend * u * u + RandCenter(rnd) * spread;
                if (x * x + y * y > 0.9 * 0.9) continue;
                double a = Rand(rnd, -0.3, 0.3) + bend * u + Math.Atan2(y - bend * u * u, 1.6) * 0.8;
                double r = Rand(rnd, 0.005, 0.024) * (1.2 - 0.7 * u);
                if (rnd.NextDouble() < 0.12)
                {
                    // 小さな冠
                    Ellipse(g, brush, x, y, a, r * 1.6, Rand(rnd, 1.0, 1.4));
                    int n = rnd.Next(5, 9);
                    for (int k = 0; k < n; k++)
                    {
                        double b = Rand(rnd, 0, Math.PI * 2);
                        Spike(g, brush, rnd, x, y, b, r * Rand(rnd, 2.0, 4.0), r * 0.4, rnd.NextDouble() < 0.5);
                    }
                }
                else
                {
                    Tadpole(g, brush, rnd, x, y, a, r, rnd.NextDouble() < 0.75 ? r * Rand(rnd, 2.0, 6.0) : 0);
                }
            }
            for (int i = 0; i < 120; i++)
            {
                double u = rnd.NextDouble();
                double x = -0.82 + u * 1.65;
                double y = bend * u * u + RandCenter(rnd) * (0.14 + 0.55 * u);
                if (x * x + y * y > 0.9 * 0.9) continue;
                Shard(g, brush, rnd, x, y, Rand(rnd, 0, 6.28), Rand(rnd, 0.002, 0.006), Rand(rnd, 1.0, 2.0));
            }
            Save(main, path, rnd);
        }
    }
}
"@

$dir = [IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force $dir | Out-Null
for ($i = 0; $i -lt 4; $i++) {
    [ScreenBloodGen]::MakeSplat((Join-Path $dir ("splat_{0}.png" -f $i)), 5300 + $i * 41)
}
for ($i = 0; $i -lt 3; $i++) {
    [ScreenBloodGen]::MakeSpray((Join-Path $dir ("spray_{0}.png" -f $i)), 7700 + $i * 53)
}
Write-Output "saved: $dir"
