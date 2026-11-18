param([string]$OutDir = ".", [string]$Suffix = "a", [int]$Seed = 7, [double]$Cup = 0.6, [switch]$ModelOnly, [int]$Count = 70, [double]$ThickMul = 1.0, [double]$IntenMul = 1.0)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class SlashGen
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Smooth(float a, float b, float x) { float t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    static void Save(float[] R, float[] G, float[] B, float[] A, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        var buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            buf[i * 4 + 0] = (byte)(C01(B[i]) * 255 + 0.5f);
            buf[i * 4 + 1] = (byte)(C01(G[i]) * 255 + 0.5f);
            buf[i * 4 + 2] = (byte)(C01(R[i]) * 255 + 0.5f);
            buf[i * 4 + 3] = (byte)(C01(A[i]) * 255 + 0.5f);
        }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    static void White(float[] R, float[] G, float[] B)
    {
        for (int i = 0; i < R.Length; i++) { R[i] = 0.93f; G[i] = 0.96f; B[i] = 1.0f; }
    }

    static float BorderFade(int px, int py, int w, int h, int m)
    {
        int d = Math.Min(Math.Min(px, w - 1 - px), Math.Min(py, h - 1 - py));
        return C01((float)d / m);
    }

    static float ValueNoise(float x, int seed)
    {
        int i = (int)Math.Floor(x); float f = x - i;
        float a = Hash(i, seed), b = Hash(i + 1, seed);
        float t = f * f * (3 - 2 * f);
        return a + (b - a) * t;
    }
    static float Hash(int i, int seed)
    {
        uint h = (uint)(i * 374761393 + seed * 668265263);
        h = (h ^ (h >> 13)) * 1274126177u;
        return (h & 0xFFFFFF) / 16777215.0f;
    }

    // 外側ほど先まで伸びる(三日月の形)
    static float Lead(float v) { return 0.5f + 0.45f * v; }

    // u: 0=後ろ(振り始め) 1=前(刃先の進む側)、v: 0=内側 1=外側
    public static void Streaks(string path, int w, int h, int seed, int count, float thickMul, float intenMul)
    {
        var rnd = new Random(seed);
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        White(R, G, B);
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float u = (px + 0.5f) / w, v = (py + 0.5f) / h;
                float lead = Lead(v);
                float env = (float)Math.Pow(u, 1.3) * (1 - Smooth(lead - 0.15f, lead, u)) * Smooth(0.15f, 0.55f, v) * (1 - Smooth(0.9f, 1.0f, v));
                float n = 0.6f + 0.4f * ValueNoise(v * 18, seed + 3);
                A[py * w + px] = 0.10f * env * n;
            }
        for (int k = 0; k < count; k++)
        {
            float v0 = 0.18f + 0.78f * (float)Math.Sqrt(rnd.NextDouble());
            float uEnd = Lead(v0) - 0.25f * (float)rnd.NextDouble();
            float len = 0.2f + 0.7f * (float)rnd.NextDouble();
            float uStart = Math.Max(0.02f, uEnd - len);
            float thick = (0.5f + 1.6f * (float)rnd.NextDouble() * (0.5f + 0.5f * v0)) * thickMul;
            float inten = (0.25f + 0.65f * (float)rnd.NextDouble()) * (0.45f + 0.55f * v0) * intenMul;
            float drift = ((float)rnd.NextDouble() - 0.5f) * 0.06f;
            int x0 = (int)(uStart * w), x1 = (int)(uEnd * w);
            for (int px = x0; px <= x1 && px < w; px++)
            {
                float u = (px + 0.5f) / w;
                float s = (u - uStart) / (uEnd - uStart);
                float prof = (float)Math.Pow(C01(s), 1.6) * (1 - Smooth(0.93f, 1.0f, s)) * (1 - Smooth(0.9f, 1.0f, u));
                float vc = (v0 + drift * (u - uEnd)) * h;
                int y0 = (int)(vc - thick - 2), y1 = (int)(vc + thick + 2);
                for (int py = Math.Max(0, y0); py <= Math.Min(h - 1, y1); py++)
                {
                    float d = Math.Abs(py + 0.5f - vc);
                    float a = C01(thick - d + 0.5f) * prof * inten;
                    int idx = py * w + px;
                    A[idx] = 1 - (1 - A[idx]) * (1 - a);
                }
            }
        }
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
                A[py * w + px] *= BorderFade(px, py, w, h, 6);
        Save(R, G, B, A, w, h, path);
    }

    // 先のふちの三日月: 前ほど太く明るい
    public static void Edge(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        White(R, G, B);
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float u = (px + 0.5f) / w, v = (py + 0.5f) / h;
                float vc = 0.86f;
                float th = 0.012f + 0.07f * u * u;
                float d = (v - vc) / th;
                float core = (float)Math.Exp(-d * d * 2.0);
                float lenEnv = (float)Math.Pow(u, 1.4) * (1 - Smooth(Lead(v) - 0.08f, Lead(v), u));
                float d2 = (v - 0.72f) / (0.006f + 0.02f * u);
                float sub = 0.35f * (float)Math.Exp(-d2 * d2 * 2.0) * (float)Math.Pow(u, 2.5);
                A[py * w + px] = C01((core + sub) * lenEnv) * BorderFade(px, py, w, h, 4);
            }
        Save(R, G, B, A, w, h, path);
    }

    public static void Haze(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        White(R, G, B);
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float u = (px + 0.5f) / w, v = (py + 0.5f) / h;
                float a = u * (float)Math.Pow(Math.Sin(Math.PI * v), 1.5) * (1 - Smooth(Lead(v) - 0.25f, Lead(v), u));
                A[py * w + px] = 0.55f * a * BorderFade(px, py, w, h, 3);
            }
        Save(R, G, B, A, w, h, path);
    }

    static float Capsule(float x, float y, float ax, float ay, float bx, float by, float r)
    {
        float pax = x - ax, pay = y - ay, bax = bx - ax, bay = by - ay;
        float t = C01((pax * bax + pay * bay) / (bax * bax + bay * bay));
        float dx = pax - bax * t, dy = pay - bay * t;
        return r - (float)Math.Sqrt(dx * dx + dy * dy);
    }

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
                f = Math.Max(f, Capsule(x, y, 0, 2.55f, 0, 3.15f, 0.30f));
                f = Math.Max(f, Capsule(x, y, 0, 2.0f, 0, 2.6f, 0.22f));
                f = Math.Max(f, Capsule(x, y, -0.30f, 3.0f, -0.52f, 2.15f, 0.10f));
                f = Math.Max(f, Capsule(x, y, 0.30f, 3.0f, 0.52f, 2.15f, 0.10f));
                f = Math.Max(f, Capsule(x, y, -0.13f, 1.9f, -0.22f, 0.1f, 0.12f));
                f = Math.Max(f, Capsule(x, y, 0.13f, 1.9f, 0.22f, 0.1f, 0.12f));
                int idx = py * w + px;
                R[idx] = 0.30f; G[idx] = 0.32f; B[idx] = 0.40f; A[idx] = C01(f / pix + 0.5f);
            }
        Save(R, G, B, A, w, h, path);
    }

    // 黒い背景に重ねた確認用の画像
    public static void Preview(string src, string dst)
    {
        var b = new Bitmap(src);
        var o = new Bitmap(b.Width, b.Height, PixelFormat.Format24bppRgb);
        for (int y = 0; y < b.Height; y++)
            for (int x = 0; x < b.Width; x++)
            {
                var c = b.GetPixel(x, y); float a = c.A / 255f;
                int bg = 20;
                o.SetPixel(x, y, Color.FromArgb((int)(bg + (c.R - bg) * a), (int)(bg + (c.G - bg) * a), (int)(bg + (c.B - bg) * a)));
            }
        o.Save(dst, ImageFormat.Png); b.Dispose(); o.Dispose();
    }
}
'@

function Write-ArcModel([string]$path, [double]$rIn, [double]$rOut, [double]$spanDeg, [double]$cup, [int]$segU, [int]$segV) {
  $verts = New-Object System.Collections.Generic.List[double[]]
  $span = $spanDeg * [Math]::PI / 180
  for ($j = 0; $j -le $segV; $j++) {
    $v = $j / $segV; $r = $rIn + ($rOut - $rIn) * $v
    $y = -$cup * ($r - $rIn) * ($r - $rIn) / (($rOut - $rIn) * ($rOut - $rIn))
    $fade = [Math]::Min(1.0, $v / 0.25)
    for ($i = 0; $i -le $segU; $i++) {
      $u = $i / $segU; $th = -$span / 2 + $span * $u
      $x = $r * [Math]::Sin($th); $z = $r * [Math]::Cos($th)
      $verts.Add([double[]]@($x, $y, $z, $u, $v, $fade, $th, $r))
    }
  }
  $ms = New-Object System.IO.MemoryStream
  $bw = New-Object System.IO.BinaryWriter $ms
  $bw.Write([int]5); $bw.Write([single]1.0); $bw.Write([int]1); $bw.Write([int]1)
  $bw.Write([int]$verts.Count)
  $dydr = 2 * $cup / (($rOut - $rIn) * ($rOut - $rIn))
  foreach ($p in $verts) {
    $th = $p[6]; $r = $p[7]
    $slope = - $dydr * ($r - $rIn)
    $nx = - $slope * [Math]::Sin($th); $nz = - $slope * [Math]::Cos($th); $ny = 1.0
    $l = [Math]::Sqrt($nx * $nx + $ny * $ny + $nz * $nz); $nx /= $l; $ny /= $l; $nz /= $l
    $tx = [Math]::Cos($th); $tz = - [Math]::Sin($th)
    $bx = $ny * $tz; $by = $nz * $tx - $nx * $tz; $bz = - $ny * $tx
    $bw.Write([single]$p[0]); $bw.Write([single]$p[1]); $bw.Write([single]$p[2])
    $bw.Write([single]$nx); $bw.Write([single]$ny); $bw.Write([single]$nz)
    $bw.Write([single]$bx); $bw.Write([single]$by); $bw.Write([single]$bz)
    $bw.Write([single]$tx); $bw.Write([single]0); $bw.Write([single]$tz)
    $bw.Write([single]$p[3]); $bw.Write([single]$p[4])
    $bw.Write([byte]255); $bw.Write([byte]255); $bw.Write([byte]255); $bw.Write([byte]([Math]::Round(255 * $p[5])))
  }
  $bw.Write([int]($segU * $segV * 2))
  for ($j = 0; $j -lt $segV; $j++) {
    for ($i = 0; $i -lt $segU; $i++) {
      $a = $j * ($segU + 1) + $i; $b = $a + 1; $c = $a + $segU + 2; $d = $a + $segU + 1
      $bw.Write([int]$a); $bw.Write([int]$c); $bw.Write([int]$b)
      $bw.Write([int]$a); $bw.Write([int]$d); $bw.Write([int]$c)
    }
  }
  $bw.Flush()
  [System.IO.File]::WriteAllBytes($path, $ms.ToArray())
}

$OutDir = (New-Item -ItemType Directory -Force $OutDir).FullName
$tex = (New-Item -ItemType Directory -Force (Join-Path $OutDir "Texture")).FullName
$mdl = (New-Item -ItemType Directory -Force (Join-Path $OutDir "Model")).FullName
$prev = (New-Item -ItemType Directory -Force (Join-Path $OutDir "preview")).FullName

if (-not $ModelOnly) {
  [SlashGen]::Streaks((Join-Path $tex "ST_Streaks_$Suffix.png"), 1024, 256, $Seed, $Count, $ThickMul, $IntenMul)
  [SlashGen]::Edge((Join-Path $tex "ST_Edge_$Suffix.png"), 1024, 256)
  [SlashGen]::Haze((Join-Path $tex "ST_Haze_$Suffix.png"), 256, 64)
  [SlashGen]::Humanoid((Join-Path $tex "ST_Dummy.png"), 128, 256)
  foreach ($n in @("ST_Streaks_$Suffix", "ST_Edge_$Suffix", "ST_Haze_$Suffix")) {
    [SlashGen]::Preview((Join-Path $tex "$n.png"), (Join-Path $prev "$n.png"))
  }
}
Write-ArcModel (Join-Path $mdl "SlashArc_$Suffix.efkmodel") 2.0 8.0 150 $Cup 48 6
Get-ChildItem $tex, $mdl | Select-Object Name, Length
