# 着地のひび割れの画像(白、形はアルファ)を作る
# 使い方: powershell -File gen_crack.ps1 出力.png [seed]
param([string]$OutPath, [int]$Seed = 3)
Add-Type -AssemblyName System.Drawing

$size = 512
$c = $size / 2
$rnd = New-Object System.Random $Seed
function Rand([double]$a, [double]$b) { return $a + $rnd.NextDouble() * ($b - $a) }

$bmp = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::FromArgb(0, 255, 255, 255))

# 太さを変えながら線を引く
function DrawCrack([double]$x, [double]$y, [double]$ang, [double]$len, [double]$w0, [double]$w1, [int]$depth) {
    $step = 20.0
    $n = [int][Math]::Ceiling($len / $step)
    for ($i = 0; $i -lt $n; $i++) {
        $t = $i / [double]$n
        $ang += (Rand -0.2 0.2)
        $nx = $x + [Math]::Cos($ang) * $step * (Rand 0.7 1.2)
        $ny = $y + [Math]::Sin($ang) * $step * (Rand 0.7 1.2)
        $w = $w0 + ($w1 - $w0) * $t
        $a = [int](255 * (1.0 - 0.35 * $t))
        $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb($a, 255, 255, 255)), ([float]$w)
        $pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
        $pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
        $g.DrawLine($pen, [float]$x, [float]$y, [float]$nx, [float]$ny)
        $pen.Dispose()
        # 枝分かれ
        if ($depth -lt 1 -and $i -gt 1 -and $rnd.NextDouble() -lt 0.14) {
            $side = if ($rnd.NextDouble() -lt 0.5) { -1.0 } else { 1.0 }
            DrawCrack $nx $ny ($ang + $side * (Rand 0.45 0.9)) ($len * (1.0 - $t) * (Rand 0.35 0.6)) ([Math]::Max(1.0, $w * 0.6)) 0.8 ($depth + 1)
        }
        $x = $nx; $y = $ny
    }
}

# 真ん中の砕けた所(短い割れを輪のように)
for ($i = 0; $i -lt 14; $i++) {
    $a = $i / 14.0 * [Math]::PI * 2 + (Rand -0.2 0.2)
    $r = Rand 14 34
    DrawCrack ($c + [Math]::Cos($a) * $r) ($c + [Math]::Sin($a) * $r) ($a + [Math]::PI / 2 * (Rand 0.6 1.4)) (Rand 18 34) 3.5 1.5 2
}
# 外へ伸びる大きな割れ
$num = 9
for ($i = 0; $i -lt $num; $i++) {
    $a = $i / [double]$num * [Math]::PI * 2 + (Rand -0.25 0.25)
    DrawCrack ($c + [Math]::Cos($a) * 10) ($c + [Math]::Sin($a) * 10) $a (Rand 150 225) 7.0 1.2 0
}

$g.Dispose()
$bmp.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "saved $OutPath"
