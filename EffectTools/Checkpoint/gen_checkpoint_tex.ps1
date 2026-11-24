# チェックポイントのエフェクトの画像を作る
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File gen_checkpoint_tex.ps1 [出力フォルダ]
param([string]$OutDir = (Join-Path $PSScriptRoot "Checkpoint_v1\Texture"))

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Runtime.InteropServices;

public static class CpTex
{
    // 芯は白に近い青白、にじみは薄い青
    static readonly float[] Core = { 0.93f, 0.96f, 1.0f };
    static readonly float[] Halo = { 0.50f, 0.66f, 1.0f };

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

    // 芯(距離<0)とにじみ(距離に応じて減衰)を合成
    static void Shade(float d, float pix, float glowW, float glowA, float coreA, out float r, out float g, out float b, out float a)
    {
        float core = C01(-d / pix + 0.5f) * coreA;
        float halo = (float)Math.Exp(-Math.Max(d, 0) / glowW) * glowA;
        a = Math.Max(core, halo);
        float k = a > 0 ? core / a : 0;
        r = Halo[0] + (Core[0] - Halo[0]) * k;
        g = Halo[1] + (Core[1] - Halo[1]) * k;
        b = Halo[2] + (Core[2] - Halo[2]) * k;
    }

    static float SegDist(float x, float y, float ax, float ay, float bx, float by)
    {
        float vx = bx - ax, vy = by - ay, wx = x - ax, wy = y - ay;
        float t = C01((wx * vx + wy * vy) / (vx * vx + vy * vy));
        float dx = wx - vx * t, dy = wy - vy * t;
        return (float)Math.Sqrt(dx * dx + dy * dy);
    }

    static float Dist(float x, float y, float ax, float ay) { return (float)Math.Sqrt((x - ax) * (x - ax) + (y - ay) * (y - ay)); }

    // 端で透明にする(半径0.97〜1.0)
    static float EdgeFade(float rr) { return C01((1.0f - rr) / 0.03f); }

    delegate float Field(float x, float y);

    static void Circular(string path, int n, Field f, float glowW, float glowA)
    {
        float[] R = new float[n * n], G = new float[n * n], B = new float[n * n], A = new float[n * n];
        float pix = 2.0f / n;
        for (int py = 0; py < n; py++)
            for (int px = 0; px < n; px++)
            {
                float x = (px + 0.5f) / n * 2 - 1, y = (py + 0.5f) / n * 2 - 1;
                float r, g, b, a;
                Shade(f(x, y), pix, glowW, glowA, 1, out r, out g, out b, out a);
                int i = py * n + px;
                R[i] = r; G[i] = g; B[i] = b; A[i] = a * EdgeFade((float)Math.Sqrt(x * x + y * y));
            }
        Save(R, G, B, A, n, n, path);
    }

    // 外の輪: 太い輪＋細い輪＋目盛り32本
    public static void CrestOuter(string path, int n)
    {
        Circular(path, n, (x, y) =>
        {
            float rr = (float)Math.Sqrt(x * x + y * y), an = (float)Math.Atan2(y, x);
            float d = Math.Abs(rr - 0.90f) - 0.012f;
            d = Math.Min(d, Math.Abs(rr - 0.81f) - 0.004f);
            float step = (float)(Math.PI * 2 / 32);
            int k = (int)Math.Round(an / step);
            float ta = k * step;
            float len = (((k % 4) + 4) % 4 == 0) ? 0.11f : 0.05f;
            float cx = (float)Math.Cos(ta), cy = (float)Math.Sin(ta);
            d = Math.Min(d, SegDist(x, y, cx * 0.81f, cy * 0.81f, cx * (0.81f - len), cy * (0.81f - len)) - 0.005f);
            return d;
        }, 0.025f, 0.55f);
    }

    // 内の切れた輪: 3本の弧
    public static void CrestArcs(string path, int n)
    {
        const float r0 = 0.58f, span = 1.5f;
        Circular(path, n, (x, y) =>
        {
            float rr = (float)Math.Sqrt(x * x + y * y), an = (float)Math.Atan2(y, x);
            if (an < 0) an += (float)(Math.PI * 2);
            float best = 9;
            for (int i = 0; i < 3; i++)
            {
                float s = (float)(i * Math.PI * 2 / 3), e = s + span;
                float d;
                if (an >= s && an <= e) d = Math.Abs(rr - r0);
                else
                {
                    d = Math.Min(Dist(x, y, (float)Math.Cos(s) * r0, (float)Math.Sin(s) * r0),
                                 Dist(x, y, (float)Math.Cos(e) * r0, (float)Math.Sin(e) * r0));
                }
                best = Math.Min(best, d - 0.010f);
            }
            return best;
        }, 0.022f, 0.6f);
    }

    // 真ん中の手裏剣(4枚刃)。内側は薄く塗る
    public static void CrestStar(string path, int n)
    {
        float[] R = new float[n * n], G = new float[n * n], B = new float[n * n], A = new float[n * n];
        float pix = 2.0f / n;
        var vx = new float[8]; var vy = new float[8];
        for (int i = 0; i < 8; i++)
        {
            double an = i / 8.0 * Math.PI * 2 + Math.PI / 4;
            float rr = (i % 2 == 0) ? 0.40f : 0.11f;
            vx[i] = (float)Math.Cos(an) * rr; vy[i] = (float)Math.Sin(an) * rr;
        }
        for (int py = 0; py < n; py++)
            for (int px = 0; px < n; px++)
            {
                float x = (px + 0.5f) / n * 2 - 1, y = (py + 0.5f) / n * 2 - 1;
                float d = 9; bool inside = false;
                for (int i = 0, j = 7; i < 8; j = i++)
                {
                    d = Math.Min(d, SegDist(x, y, vx[i], vy[i], vx[j], vy[j]));
                    if (((vy[i] > y) != (vy[j] > y)) && (x < (vx[j] - vx[i]) * (y - vy[i]) / (vy[j] - vy[i]) + vx[i])) inside = !inside;
                }
                float r, g, b, a;
                Shade(d - 0.008f, pix, 0.02f, 0.6f, 1, out r, out g, out b, out a);
                float rr = (float)Math.Sqrt(x * x + y * y);
                if (inside)
                {
                    float fill = 0.22f + 0.25f * C01(1 - rr / 0.4f);
                    if (fill > a) { float k = a / fill; r = Halo[0] + (r - Halo[0]) * k; g = Halo[1] + (g - Halo[1]) * k; b = Halo[2] + (b - Halo[2]) * k; a = fill; }
                }
                // 中心の穴
                float hole = C01((rr - 0.035f) / pix);
                int idx = py * n + px;
                R[idx] = r; G[idx] = g; B[idx] = b; A[idx] = a * (0.3f + 0.7f * hole) * EdgeFade(rr);
            }
        Save(R, G, B, A, n, n, path);
    }

    // 丸いぼかし(白)。色はノードで付ける
    public static void Glow(string path, int n, float power)
    {
        float[] R = new float[n * n], G = new float[n * n], B = new float[n * n], A = new float[n * n];
        for (int py = 0; py < n; py++)
            for (int px = 0; px < n; px++)
            {
                float x = (px + 0.5f) / n * 2 - 1, y = (py + 0.5f) / n * 2 - 1;
                float rr = (float)Math.Sqrt(x * x + y * y);
                int i = py * n + px;
                R[i] = G[i] = B[i] = 1;
                A[i] = (float)Math.Pow(C01(1 - rr), power);
            }
        Save(R, G, B, A, n, n, path);
    }

    // 細い輪(白)。衝撃の輪
    public static void Ring(string path, int n)
    {
        float[] R = new float[n * n], G = new float[n * n], B = new float[n * n], A = new float[n * n];
        for (int py = 0; py < n; py++)
            for (int px = 0; px < n; px++)
            {
                float x = (px + 0.5f) / n * 2 - 1, y = (py + 0.5f) / n * 2 - 1;
                float rr = (float)Math.Sqrt(x * x + y * y);
                float d = rr - 0.88f;
                float a = d < 0 ? (float)Math.Exp(d / 0.035f) : (float)Math.Exp(-d / 0.012f);
                int i = py * n + px;
                R[i] = G[i] = B[i] = 1; A[i] = a * EdgeFade(rr);
            }
        Save(R, G, B, A, n, n, path);
    }

    // 縦の筋(白)。上下の端で0
    public static void Streak(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, v = (py + 0.5f) / h;
                float across = (float)Math.Exp(-x * x / 0.06f);
                float along = (float)Math.Pow(Math.Sin(Math.PI * v), 1.5);
                int i = py * w + px;
                R[i] = G[i] = B[i] = 1; A[i] = across * along * C01((1 - Math.Abs(x)) / 0.1f);
            }
        Save(R, G, B, A, w, h, path);
    }

    // 光の柱。芯は白、外は青。上に行くほど薄く、下の端も少し薄く
    public static void Pillar(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, v = 1 - (py + 0.5f) / h;   // v: 0=下 1=上
                float core = (float)Math.Exp(-x * x / 0.008f);
                float halo = (float)Math.Exp(-x * x / 0.12f) * 0.45f;
                float a = Math.Max(core, halo);
                float k = a > 0 ? core / a : 0;
                float along = (float)Math.Pow(C01(1 - v), 1.6) * C01(v / 0.03f);
                int i = py * w + px;
                R[i] = Halo[0] + (Core[0] - Halo[0]) * k;
                G[i] = Halo[1] + (Core[1] - Halo[1]) * k;
                B[i] = Halo[2] + (Core[2] - Halo[2]) * k;
                A[i] = a * along * C01((1 - Math.Abs(x)) / 0.08f);
            }
        Save(R, G, B, A, w, h, path);
    }

    // 「CHECKPOINT」の文字＋左右の線。文字の形は GDI+ で描き、ぼかしてにじみを付ける
    public static void Text(string path, int w, int h, string text, string fontName, float fontPx, float spacing)
    {
        var mask = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        float textW = 0;
        using (var g = Graphics.FromImage(mask))
        {
            g.Clear(Color.Black);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            var fmt = StringFormat.GenericTypographic;
            using (var font = new Font(fontName, fontPx, FontStyle.Regular, GraphicsUnit.Pixel))
            {
                var widths = new float[text.Length];
                for (int i = 0; i < text.Length; i++) { widths[i] = g.MeasureString(text[i].ToString(), font, 1000, fmt).Width; textW += widths[i]; }
                textW += spacing * (text.Length - 1);
                float x = (w - textW) / 2, y = (h - fontPx * 1.2f) / 2;
                for (int i = 0; i < text.Length; i++) { g.DrawString(text[i].ToString(), font, Brushes.White, x, y, fmt); x += widths[i] + spacing; }
            }
            // 左右の線
            float gap = 26, lineLen = 150, cy = h / 2f;
            using (var pen = new Pen(Color.White, 3))
            {
                g.DrawLine(pen, w / 2f - textW / 2 - gap - lineLen, cy, w / 2f - textW / 2 - gap, cy);
                g.DrawLine(pen, w / 2f + textW / 2 + gap, cy, w / 2f + textW / 2 + gap + lineLen, cy);
            }
            // 線の先の菱形
            float[] ends = { w / 2f - textW / 2 - gap - lineLen, w / 2f + textW / 2 + gap + lineLen };
            foreach (var ex in ends)
                g.FillPolygon(Brushes.White, new[] { new PointF(ex - 7, cy), new PointF(ex, cy - 7), new PointF(ex + 7, cy), new PointF(ex, cy + 7) });
        }
        float[] m = new float[w * h];
        var d = mask.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        Marshal.Copy(d.Scan0, buf, 0, buf.Length);
        mask.UnlockBits(d); mask.Dispose();
        for (int i = 0; i < w * h; i++) m[i] = buf[i * 4 + 1] / 255f;

        float[] blur = (float[])m.Clone();
        for (int pass = 0; pass < 4; pass++) blur = BoxBlur(blur, w, h, 7);

        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                int i = py * w + px;
                float core = m[i], halo = C01(blur[i] * 1.6f) * 0.7f;
                float a = Math.Max(core, halo), k = a > 0 ? core / a : 0;
                float ex = Math.Min(Math.Min(px, w - 1 - px), Math.Min(py, h - 1 - py));
                R[i] = Halo[0] + (Core[0] - Halo[0]) * k;
                G[i] = Halo[1] + (Core[1] - Halo[1]) * k;
                B[i] = Halo[2] + (Core[2] - Halo[2]) * k;
                A[i] = a * C01(ex / 8f);
            }
        Save(R, G, B, A, w, h, path);
    }

    static float[] BoxBlur(float[] src, int w, int h, int r)
    {
        float[] tmp = new float[w * h], dst = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float s = 0; int c = 0;
                for (int k = -r; k <= r; k++) { int xx = x + k; if (xx < 0 || xx >= w) continue; s += src[y * w + xx]; c++; }
                tmp[y * w + x] = s / c;
            }
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float s = 0; int c = 0;
                for (int k = -r; k <= r; k++) { int yy = y + k; if (yy < 0 || yy >= h) continue; s += tmp[yy * w + x]; c++; }
                dst[y * w + x] = s / c;
            }
        return dst;
    }

    // 大きさの確認用の人型(暗い色)
    public static void Humanoid(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        float pix = 2.0f / w;
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, y = (h - py - 0.5f) / w * 2;
                float f = -1;
                f = Math.Max(f, 0.17f - (float)Math.Sqrt(x * x + (y - 3.45f) * (y - 3.45f)));
                f = Math.Max(f, 0.30f - SegDist(x, y, 0, 2.55f, 0, 3.15f));
                f = Math.Max(f, 0.22f - SegDist(x, y, 0, 2.0f, 0, 2.6f));
                f = Math.Max(f, 0.10f - SegDist(x, y, -0.30f, 3.0f, -0.52f, 2.15f));
                f = Math.Max(f, 0.10f - SegDist(x, y, 0.30f, 3.0f, 0.52f, 2.15f));
                f = Math.Max(f, 0.12f - SegDist(x, y, -0.13f, 1.9f, -0.22f, 0.1f));
                f = Math.Max(f, 0.12f - SegDist(x, y, 0.13f, 1.9f, 0.22f, 0.1f));
                int i = py * w + px;
                R[i] = 0.08f; G[i] = 0.09f; B[i] = 0.12f; A[i] = C01(f / pix + 0.5f);
            }
        Save(R, G, B, A, w, h, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
$font = "Bahnschrift SemiBold"
$fams = (New-Object System.Drawing.Text.InstalledFontCollection).Families | ForEach-Object { $_.Name }
if ($fams -notcontains $font) { $font = "Bahnschrift" }
if ($fams -notcontains $font) { $font = "Segoe UI" }
Write-Host "font: $font"

[CpTex]::CrestOuter((Join-Path $OutDir "CP_CrestOuter.png"), 1024)
[CpTex]::CrestArcs((Join-Path $OutDir "CP_CrestArcs.png"), 512)
[CpTex]::CrestStar((Join-Path $OutDir "CP_CrestStar.png"), 512)
[CpTex]::Glow((Join-Path $OutDir "CP_Glow.png"), 128, 2.2)
[CpTex]::Ring((Join-Path $OutDir "CP_Ring2.png"), 512)
[CpTex]::Streak((Join-Path $OutDir "CP_Streak.png"), 32, 128)
[CpTex]::Pillar((Join-Path $OutDir "CP_Pillar.png"), 64, 512)
[CpTex]::Text((Join-Path $OutDir "CP_Text.png"), 1024, 128, "CHECKPOINT", $font, 64, 14)
[CpTex]::Humanoid((Join-Path $OutDir "Dummy_Humanoid.png"), 128, 256)
Get-ChildItem $OutDir | Select-Object Name, Length
