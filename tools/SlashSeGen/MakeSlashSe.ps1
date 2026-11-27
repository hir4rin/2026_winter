# 刀で敵を斬ったときの SE を合成して WAV に書き出す
# NG4 のトレーニングの録画を測った形に合わせた版(out/ に書き出す)
# 層: 斬った頭(高域の破裂) / チッ(下がる高音) / 高域の擦れ / 肉 / 遅れて来る低音 / 血しぶき(ブシャッ)
param([string]$OutDir = (Join-Path $PSScriptRoot "out"))

$src = @"
using System;
using System.IO;

public static class SlashSynth
{
    const int SR = 44100;

    public class P
    {
        public int Seed = 11;
        public double Pre = 0.01;
        public double Crack = 1.0;        // 斬った頭
        public double CrackDecay = 0.012;
        public double CrackLowCut = 1500;
        public double Chirp = 0.5;        // チッ
        public double ChirpFrom = 9000;
        public double ChirpTo = 1200;
        public double ChirpTime = 0.012;
        public double Hiss = 0.45;        // 高域の擦れ
        public double HissDecay = 0.05;
        public double Flesh = 0.5;        // 肉
        public double FleshDecay = 0.03;
        public double Body = 0.9;         // 遅れて来る低音
        public double BodyHz = 75;
        public double BodyDelay = 0.025;
        public double BodyDecay = 0.04;
        public double Spray = 0.8;        // 血しぶき
        public double SprayDelay = 0.06;
        public double SprayLen = 0.16;
        public double SprayGrain = 0.6;
        public double SprayLowCut = 1500;
        public double Drive = 1.6;
        public double Length = 0.4;
    }

    class Svf
    {
        double lo, bp;
        public double Lp, Bp, Hp;
        public void Run(double x, double fc, double q)
        {
            double f = 2.0 * Math.Sin(Math.PI * Math.Min(fc, SR * 0.24) / SR);
            Hp = x - lo - bp / q;
            bp += f * Hp;
            lo += f * bp;
            Lp = lo; Bp = bp;
        }
    }

    public static float[] Make(P p)
    {
        int n = (int)(p.Length * SR);
        double[] o = new double[n];
        double[] lo = new double[n];
        Random r = new Random(p.Seed);
        int hit = (int)(p.Pre * SR);

        // 斬った頭: 高域のノイズの破裂
        Svf cf = new Svf();
        for (int i = hit; i < n; i++)
        {
            double t = (double)(i - hit) / SR;
            if (t > p.CrackDecay * 8) break;
            double x = r.NextDouble() * 2 - 1;
            cf.Run(x, p.CrackLowCut, 0.7);
            o[i] += (cf.Hp + x * 0.15) * Math.Min(t / 0.0003, 1) * Math.Exp(-t / p.CrackDecay) * p.Crack * 1.6;
        }

        // チッ: 一気に下がる高音
        double ph = 0;
        for (int i = hit; i < n; i++)
        {
            double t = (double)(i - hit) / SR;
            if (t > p.ChirpTime * 6) break;
            double k = Math.Min(t / p.ChirpTime, 1);
            ph += 2 * Math.PI * p.ChirpFrom * Math.Pow(p.ChirpTo / p.ChirpFrom, k) / SR;
            o[i] += Math.Sin(ph) * Math.Exp(-t / (p.ChirpTime * 0.8)) * p.Chirp * 0.6;
        }

        // 高域の擦れ
        Svf hf = new Svf();
        for (int i = hit; i < n; i++)
        {
            double t = (double)(i - hit) / SR;
            hf.Run(r.NextDouble() * 2 - 1, 6000, 0.7);
            o[i] += hf.Hp * Math.Min(t / 0.001, 1) * Math.Exp(-t / p.HissDecay) * p.Hiss * 0.5;
        }

        // 肉: 中域の短い詰まり
        Svf ff = new Svf();
        for (int i = hit; i < n; i++)
        {
            double t = (double)(i - hit) / SR;
            ff.Run(r.NextDouble() * 2 - 1, 450, 1.0);
            o[i] += ff.Bp * Math.Min(t / 0.002, 1) * Math.Exp(-t / p.FleshDecay) * p.Flesh * 1.2;
        }

        // 遅れて来る低音(歪ませない)
        Svf bf = new Svf();
        double bp2 = 0;
        int b0 = hit + (int)(p.BodyDelay * SR);
        for (int i = b0; i < n; i++)
        {
            double t = (double)(i - b0) / SR;
            bp2 += 2 * Math.PI * p.BodyHz * (1 + 0.5 * Math.Exp(-t * 40)) / SR;
            bf.Run(r.NextDouble() * 2 - 1, 200, 0.7);
            lo[i] += (Math.Sin(bp2) + bf.Lp * 0.6) * Math.Min(t / 0.004, 1) * Math.Exp(-t / p.BodyDecay) * p.Body;
        }

        // 血しぶき: 粒の荒い高域のノイズ
        Svf sf = new Svf();
        Svf sb = new Svf();
        double grain = 0;
        int s0 = hit + (int)(p.SprayDelay * SR);
        double hold = p.SprayLen * 0.5;
        for (int i = s0; i < n; i++)
        {
            double t = (double)(i - s0) / SR;
            double env = Math.Min(t / 0.015, 1) * (t < hold ? 1 : Math.Exp(-(t - hold) / (p.SprayLen * 0.25)));
            grain += 0.008 * (r.NextDouble() - grain);
            double g = (1 - p.SprayGrain) + p.SprayGrain * Math.Abs(grain - 0.5) * 4;
            double x = r.NextDouble() * 2 - 1;
            sf.Run(x, p.SprayLowCut, 0.7);
            sb.Run(x, 4000, 1.5);
            o[i] += (sf.Hp * 0.8 + sb.Bp * 0.6) * env * g * p.Spray;
        }

        double peak = 0;
        for (int i = 0; i < n; i++)
        {
            o[i] = Math.Tanh(o[i] * p.Drive) + lo[i] * 0.35;
            peak = Math.Max(peak, Math.Abs(o[i]));
        }
        float[] res = new float[n];
        int fade = (int)(0.03 * SR);
        for (int i = 0; i < n; i++)
        {
            double g = i > n - fade ? (double)(n - i) / fade : 1;
            res[i] = (float)(Math.Tanh(o[i] / peak * 1.1) / Math.Tanh(1.1) * 0.89 * g);
        }
        return res;
    }

    public static void Save(string path, float[] s)
    {
        using (BinaryWriter b = new BinaryWriter(File.Create(path)))
        {
            int bytes = s.Length * 2;
            b.Write(new char[] { 'R', 'I', 'F', 'F' }); b.Write(36 + bytes);
            b.Write(new char[] { 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' });
            b.Write(16); b.Write((short)1); b.Write((short)1);
            b.Write(SR); b.Write(SR * 2); b.Write((short)2); b.Write((short)16);
            b.Write(new char[] { 'd', 'a', 't', 'a' }); b.Write(bytes);
            foreach (float v in s) b.Write((short)(Math.Max(-1, Math.Min(1, v)) * 32767));
        }
    }
}
"@
Add-Type -TypeDefinition $src -Language CSharp

New-Item -ItemType Directory -Force $OutDir | Out-Null

function New-Se([string]$name, [hashtable]$h) {
    $p = New-Object SlashSynth+P
    foreach ($k in $h.Keys) { $p.$k = $h[$k] }
    [SlashSynth]::Save((Join-Path $OutDir "$name.wav"), [SlashSynth]::Make($p))
    Write-Host "書き出し: $name.wav"
}

# 通常の斬撃(コンボ用に3つ)
New-Se "slash_hit_01" @{ Seed = 11 }
New-Se "slash_hit_02" @{ Seed = 23; ChirpFrom = 10000; BodyHz = 80 }
New-Se "slash_hit_03" @{ Seed = 37; ChirpFrom = 8000; BodyHz = 70; SprayDelay = 0.05 }
# 軽い(弱攻撃の早い段)
New-Se "slash_hit_light" @{ Seed = 5; Spray = 0.35; SprayDelay = 0.04; SprayLen = 0.08; Body = 0.6; BodyDecay = 0.035; Length = 0.25 }
# 重い(強攻撃・トドメ。遅れて大きな血しぶき)
New-Se "slash_hit_heavy" @{ Seed = 71; Crack = 1.1; Body = 1.2; BodyHz = 62; BodyDecay = 0.055; Spray = 1.1; SprayDelay = 0.18; SprayLen = 0.25; Length = 0.6 }
