# ArseneWing 用の羽根テクスチャ(4x2 シート)を生成する。実行: powershell -NoProfile -ExecutionPolicy Bypass -File gen_arsene_feather.ps1
# アルセーヌ風の羽根: 先が数本に裂けた炎状の羽根（セル調）。テクスチャ上で付け根=下、先端=上。
$code = @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class Feather
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Smooth(float a, float b, float x) { float t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    // 1本の羽根 = 付け根から少しずつ開いて伸びる細い槍(strand)の和集合
    // cx,cy: セル内の付け根(0-1)、len: 長さ(セル高さ比)、curl: 全体の反り
    public static void Sheet(string path, int cw, int ch, int cols, int rows, int seed)
    {
        int W = cw * cols, H = ch * rows;
        var bmp = new Bitmap(W, H, PixelFormat.Format32bppArgb);
        var rnd = new Random(seed);
        for (int cell = 0; cell < cols * rows; cell++)
        {
            int ox = (cell % cols) * cw, oy = (cell / cols) * ch;
            int n = 2 + rnd.Next(3);                    // 先端の指の本数 2-4
            float curl = (float)(rnd.NextDouble() * 0.05 - 0.025);
            float wb = 0.060f + (float)rnd.NextDouble() * 0.015f;    // 本体の最大半幅
            float bodyEnd = 0.55f + (float)rnd.NextDouble() * 0.12f; // 本体が尖って終わる高さ
            var fx0 = new float[n]; var fy0 = new float[n]; var fx1 = new float[n]; var fy1 = new float[n]; var fw = new float[n];
            for (int i = 0; i < n; i++)
            {
                float k = n == 1 ? 0 : (i / (float)(n - 1)) * 2 - 1;
                fy0[i] = 0.25f + (float)rnd.NextDouble() * 0.12f;
                fx0[i] = k * wb * 0.30f;
                fy1[i] = (Math.Abs(k) < 0.5f ? 0.93f : 0.72f) + (float)rnd.NextDouble() * 0.06f;
                fx1[i] = k * (0.065f + (float)rnd.NextDouble() * 0.03f);
                fw[i] = wb * (0.55f + 0.2f * (1 - Math.Abs(k))) * (n > 3 ? 0.8f : 1f);
            }
            for (int py = 0; py < ch; py++)
            for (int px = 0; px < cw; px++)
            {
                float y = 1f - (py + 0.5f) / ch;
                y = (y - 0.03f) / 0.94f;
                float x = ((px + 0.5f) - cw * 0.5f) / ch - curl * y * y;
                float best = -1;
                float bu = 0, bs = C01(y);
                if (y > 0 && y < 1)
                {
                    // 本体: 細い付け根 -> 0.3 付近で最大 -> bodyEnd で尖る
                    float pk = 0.3f;
                    if (y < bodyEnd)
                    {
                        float prof = y < pk ? (float)Math.Pow(y / pk, 0.9) : (float)Math.Pow((bodyEnd - y) / (bodyEnd - pk), 0.8);
                        float hw = Math.Max(wb * prof, 0.003f);
                        best = hw - Math.Abs(x); bu = x / hw;
                    }
                    // 指: 本体の途中から外へ開いて伸びる槍
                    for (int i = 0; i < n; i++)
                    {
                        float s = (y - fy0[i]) / (fy1[i] - fy0[i]);
                        if (s < 0 || s > 1) continue;
                        float xc = fx0[i] + (fx1[i] - fx0[i]) * (float)Math.Pow(s, 1.4);
                        float hw = fw[i] * (float)Math.Pow(1 - s, 0.85) * (float)Math.Sqrt(Math.Min(1f, s * 5f));
                        float d = hw - Math.Abs(x - xc);
                        if (d > best) { best = d; bu = (x - xc) / Math.Max(hw, 1e-4f); }
                    }
                }                float pix = 1f / ch;
                float a = C01(best / pix + 0.5f);
                // 画像の端は必ず alpha 0
                float ex = Math.Min(px, cw - 1 - px), ey = Math.Min(py, ch - 1 - py);
                a *= C01(Math.Min(ex, ey) / 3f);
                // セル調の陰影: 暗部(黒) / 中間(灰) / ハイライト(淡い紫白)を段で
                float yy = C01(y);
                float shade = 0.06f;
                float band = Smooth(0.25f, 0.05f, bu) * Smooth(0.95f, 0.55f, yy) * Smooth(0.0f, 0.15f, yy);   // 片側(左)に灰の帯
                shade += 0.20f * Smooth(0.25f, 0.6f, band);
                float rim = Smooth(-0.70f, -0.92f, bu) * Smooth(0.05f, 0.25f, bs) * Smooth(1.0f, 0.75f, bs);   // 左の縁にハイライト
                float barb = 0.5f + 0.5f * (float)Math.Sin((bu * 2.2f - yy * 26f) * 3.1416f);                // 羽枝の斜めの筋
                shade *= 0.93f + 0.07f * barb;
                float r = shade, g = shade, b = shade * 1.05f;
                r += rim * 0.40f; g += rim * 0.36f; b += rim * 0.48f;
                int ia = (int)(C01(a) * 255), ir = (int)(C01(r) * 255), ig = (int)(C01(g) * 255), ib = (int)(C01(b) * 255);
                bmp.SetPixel(ox + px, oy + py, Color.FromArgb(ia, ir, ig, ib));
            }
        }
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    // 確認用: 指定色の背景に合成し、セル境界線を引く
    public static void Preview(string src, string dst, int cw, int ch, int bgR, int bgG, int bgB)
    {
        var s = new Bitmap(src);
        var d = new Bitmap(s.Width, s.Height, PixelFormat.Format24bppRgb);
        for (int y = 0; y < s.Height; y++)
        for (int x = 0; x < s.Width; x++)
        {
            Color c = s.GetPixel(x, y); float a = c.A / 255f;
            bool line = (x % cw == 0) || (y % ch == 0);
            int r = line ? 255 : (int)(c.R * a + bgR * (1 - a));
            int g = line ? 0 : (int)(c.G * a + bgG * (1 - a));
            int b = line ? 0 : (int)(c.B * a + bgB * (1 - a));
            d.SetPixel(x, y, Color.FromArgb(r, g, b));
        }
        d.Save(dst, ImageFormat.Png); s.Dispose(); d.Dispose();
    }
}
"@
Add-Type -TypeDefinition $code -ReferencedAssemblies System.Drawing
$TexDir = Join-Path $PSScriptRoot "..\2026_winter\data\Effect\Skill\Texture"
$tex = Join-Path $TexDir "ArseneFeather2.png"
[Feather]::Sheet($tex, 256, 1024, 4, 2, 7)
[Feather]::Preview($tex, (Join-Path $env:TEMP "ArseneFeather_preview.png"), 256, 1024, 70, 80, 100)
"done"