# タイトルの斬撃の閃光(Effekseer)のテクスチャを作る
# 実行: powershell -NoProfile -ExecutionPolicy Bypass -File gen_slash_tex.ps1 [-OutDir 出力先] [-Suffix _v1]
# 出力: TS_Core / TS_Glow / TS_Head (白。色はノード側で付ける。端は alpha 0)
param([string]$OutDir = (Join-Path $PSScriptRoot "Texture"), [string]$Suffix = "")
New-Item -ItemType Directory -Force $OutDir | Out-Null
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System; using System.Drawing; using System.Drawing.Imaging;
public static class SlashTex {
    static double C01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
    // 刃の半分の太さ(u: -1〜1)。右(進む側)ほど鋭く、端の手前で 0
    static double Blade(double u) {
        double t = (u + 1) * 0.5;                 // 0〜1
        if (t <= 0.04 || t >= 0.96) return 0;
        double a = (t - 0.04) / 0.92;
        return Math.Pow(Math.Sin(Math.PI * Math.Pow(a, 0.8)), 1.6);
    }
    static void Save(Bitmap b, string path) { b.Save(path, ImageFormat.Png); b.Dispose(); }
    public static void Core(string path, int w, int h) {
        var b = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int x = 0; x < w; x++) {
            double u = (x + 0.5) / w * 2 - 1, hw = Blade(u) * 0.42 * h;
            for (int y = 0; y < h; y++) {
                double v = Math.Abs(y + 0.5 - h * 0.5);
                double a = C01(hw - v + 0.5);
                b.SetPixel(x, y, Color.FromArgb((int)(255 * a), 255, 255, 255));
            }
        }
        Save(b, path);
    }
    public static void Glow(string path, int w, int h) {
        var b = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int x = 0; x < w; x++) {
            double u = (x + 0.5) / w * 2 - 1, k = Blade(u);
            double env = C01((1 - Math.Abs(u)) / 0.12);
            for (int y = 0; y < h; y++) {
                double v = (y + 0.5 - h * 0.5) / (h * 0.5);
                double s = 0.08 + 0.32 * k;
                double a = Math.Exp(-(v * v) / (s * s)) * (0.25 + 0.75 * k) * env;
                if (Math.Abs(v) > 0.92) a *= C01((1 - Math.Abs(v)) / 0.08);
                b.SetPixel(x, y, Color.FromArgb((int)(255 * C01(a)), 255, 255, 255));
            }
        }
        Save(b, path);
    }
    public static void Head(string path, int w, int h) {
        var b = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int x = 0; x < w; x++) {
            double u = (x + 0.5) / w * 2 - 1;
            for (int y = 0; y < h; y++) {
                double v = (y + 0.5) / h * 2 - 1;
                double r2 = u * u + (v * 3) * (v * 3);
                double a = Math.Exp(-r2 * 2.5) + 0.6 * Math.Exp(-(v * v) * 120) * Math.Exp(-u * u * 3);
                double edge = C01((1 - Math.Abs(u)) / 0.1) * C01((1 - Math.Abs(v)) / 0.1);
                b.SetPixel(x, y, Color.FromArgb((int)(255 * C01(a) * edge), 255, 255, 255));
            }
        }
        Save(b, path);
    }
}
"@
[SlashTex]::Core((Join-Path $OutDir "TS_Core$Suffix.png"), 512, 64)
[SlashTex]::Glow((Join-Path $OutDir "TS_Glow$Suffix.png"), 512, 128)
[SlashTex]::Head((Join-Path $OutDir "TS_Head$Suffix.png"), 256, 128)
"done $OutDir"
