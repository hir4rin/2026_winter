param([string]$OutDir = (Join-Path $PSScriptRoot '..\Texture'))
# 閃華の弱 1段目のテクスチャ(赤い直線・風の帯・風のちぎれ)
$src = @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class GleamTex
{
    static float C01(float v){ return v < 0 ? 0 : (v > 1 ? 1 : v); }
    static float Smooth(float a, float b, float x){ float t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }
    static void Save(float[] r, float[] g, float[] b, float[] a, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        {
            int i = y * w + x;
            bmp.SetPixel(x, y, Color.FromArgb((int)(C01(a[i]) * 255), (int)(C01(r[i]) * 255), (int)(C01(g[i]) * 255), (int)(C01(b[i]) * 255)));
        }
        bmp.Save(path, ImageFormat.Png);
    }
    static float Hash(int n){ n = (n << 13) ^ n; return 1f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824f; }
    static float Noise1(float x, int seed){ int i = (int)Math.Floor(x); float f = x - i; float u = f * f * (3 - 2 * f); return Hash(i + seed * 131) * (1 - u) + Hash(i + 1 + seed * 131) * u; }

    // 赤い直線: 横長。真ん中が太く両端が細い。芯は明るい赤、縁は暗い血の色
    public static void RedLine(string path, int w, int h)
    {
        var r = new float[w * h]; var g = new float[w * h]; var b = new float[w * h]; var a = new float[w * h];
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        {
            float u = x / (float)(w - 1) * 2 - 1, v = (y + 0.5f) / h * 2 - 1;
            float taper = (float)Math.Pow(Math.Max(0, 1 - u * u), 0.6) * (1 - Smooth(0.85f, 0.97f, Math.Abs(u)));
            float half = 0.55f * taper + 0.0001f;
            float d = Math.Abs(v) / half;
            float body = 1 - Smooth(0.75f, 1.0f, d);
            float glow = (float)Math.Exp(-Math.Abs(v) / 0.25f) * 0.35f * taper;
            float core = 1 - Smooth(0.0f, 0.35f, d);
            int i = y * w + x;
            a[i] = Math.Max(body * taper, glow);
            r[i] = 0.55f + 0.45f * core; g[i] = 0.02f + 0.18f * core; b[i] = 0.05f + 0.15f * core;
        }
        Save(r, g, b, a, w, h, path);
    }

    // 風の帯: リボン用。U=長さ(左が尾)、V=幅。細い筋が何本も流れ、尾と幅の端で透明
    public static void WindRibbon(string path, int w, int h, int seed)
    {
        var r = new float[w * h]; var g = new float[w * h]; var b = new float[w * h]; var a = new float[w * h];
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
        {
            float u = x / (float)(w - 1), v = (y + 0.5f) / h;
            float acc = 0;
            for (int k = 0; k < 7; k++)
            {
                float c = 0.15f + 0.7f * (k / 6f) + 0.06f * Noise1(u * 3 + k * 1.7f, seed + k);
                float wid = 0.025f + 0.03f * (0.5f + 0.5f * Hash(k + seed * 7));
                float len0 = 0.15f + 0.5f * (0.5f + 0.5f * Hash(k * 3 + seed));
                float line = (float)Math.Exp(-Math.Pow((v - c) / wid, 2)) * Smooth(len0 - 0.15f, len0 + 0.25f, u);
                acc += line * (0.55f + 0.45f * (0.5f + 0.5f * Hash(k * 11 + seed)));
            }
            float soft = (float)Math.Exp(-Math.Pow((v - 0.5f) / 0.28f, 2)) * 0.25f * Smooth(0.0f, 0.6f, u);
            float edge = Smooth(0.0f, 0.12f, v) * Smooth(0.0f, 0.12f, 1 - v) * Smooth(0.0f, 0.1f, u) * Smooth(0.0f, 0.04f, 1 - u);
            int i = y * w + x;
            a[i] = C01(acc + soft) * edge;
            r[i] = 0.86f + 0.14f * acc; g[i] = 0.90f + 0.10f * acc; b[i] = 1.0f;
        }
        Save(r, g, b, a, w, h, path);
    }

    // 風のちぎれ: 小さな弧の筋(4コマ横並び)。ほどけるときに飛ぶ
    public static void WispSheet(string path, int cell, int cols, int seed)
    {
        int w = cell * cols, h = cell;
        var r = new float[w * h]; var g = new float[w * h]; var b = new float[w * h]; var a = new float[w * h];
        for (int c = 0; c < cols; c++)
        {
            float curve = 0.25f + 0.5f * (0.5f + 0.5f * Hash(c * 5 + seed));
            for (int y = 0; y < h; y++) for (int x = 0; x < cell; x++)
            {
                float u = x / (float)(cell - 1) * 2 - 1, v = (y + 0.5f) / h * 2 - 1;
                float center = curve * (u * u - 0.35f) + 0.1f * Noise1(u * 4, seed + c);
                float t = (u + 1) / 2;
                float wid = 0.04f + 0.10f * (float)Math.Sin(Math.PI * t);
                float line = (float)Math.Exp(-Math.Pow((v - center) / wid, 2));
                float edge = Smooth(0.0f, 0.15f, 1 - Math.Abs(u)) * Smooth(0.0f, 0.15f, 1 - Math.Abs(v));
                int i = y * w + c * cell + x;
                a[i] = line * edge * (0.3f + 0.7f * (float)Math.Sin(Math.PI * t));
                r[i] = 0.85f; g[i] = 0.9f; b[i] = 1.0f;
            }
        }
        Save(r, g, b, a, w, h, path);
    }

    // 確認用: 黒の上に重ねる
    public static void Preview(string src, string dst)
    {
        var s = new Bitmap(src);
        var bmp = new Bitmap(s.Width, s.Height, PixelFormat.Format32bppArgb);
        using (var gr = Graphics.FromImage(bmp)) { gr.Clear(Color.Black); gr.DrawImage(s, 0, 0, s.Width, s.Height); gr.DrawRectangle(Pens.DimGray, 0, 0, s.Width - 1, s.Height - 1); }
        bmp.Save(dst, ImageFormat.Png);
    }
}
'@
Add-Type -TypeDefinition $src -ReferencedAssemblies System.Drawing
$OutDir = (Resolve-Path $OutDir).Path
[GleamTex]::RedLine((Join-Path $OutDir 'GL_RedLine.png'), 512, 32)
[GleamTex]::WindRibbon((Join-Path $OutDir 'GL_WindRibbon.png'), 256, 64, 3)
[GleamTex]::WispSheet((Join-Path $OutDir 'GL_Wisp.png'), 128, 4, 5)
$pv = Join-Path $PSScriptRoot '..\preview'
New-Item -ItemType Directory -Force $pv | Out-Null
foreach ($n in 'GL_RedLine','GL_WindRibbon','GL_Wisp') { [GleamTex]::Preview((Join-Path $OutDir "$n.png"), (Join-Path (Resolve-Path $pv).Path "$n.png")) }
'done'
