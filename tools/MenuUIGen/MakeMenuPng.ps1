# メニューUIの仮素材(白ベース)を作る
# 実行: powershell -ExecutionPolicy Bypass -File MakeMenuPng.ps1
# 出力: 2026_winter/data/UI/Menu/
Add-Type -AssemblyName System.Drawing

$out = Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\Menu"
New-Item -ItemType Directory -Force $out | Out-Null

function New-Bmp($w, $h) {
    $b = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    return $b
}
function Save($b, $name) {
    $b.Save((Join-Path $out $name), [System.Drawing.Imaging.ImageFormat]::Png)
    $b.Dispose()
    Write-Host "saved $name"
}
function White($a) { return [System.Drawing.Color]::FromArgb([int][Math]::Max(0, [Math]::Min(255, $a)), 255, 255, 255) }

# 枠(9スライス 48x48、角16)
$b = New-Bmp 48 48
$g = [System.Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'AntiAlias'
$pen = New-Object System.Drawing.Pen((White 255), 1.5)
$g.DrawRectangle($pen, 4, 4, 39, 39)
$br = New-Object System.Drawing.SolidBrush((White 255))
foreach ($p in @(@(1, 1), @(41, 1), @(1, 41), @(41, 41))) {
    $g.DrawEllipse($pen, $p[0], $p[1], 5, 5)
}
$g.Dispose(); Save $b "frame9.png"

# 選択帯のベース(左が透明→右が不透明)
$b = New-Bmp 512 64
for ($x = 0; $x -lt 512; $x++) {
    $t = $x / 511.0
    $a = [int](255 * [Math]::Pow($t, 1.6))
    for ($y = 0; $y -lt 64; $y++) {
        $edge = if ($y -lt 2 -or $y -gt 61) { 1.0 } else { 0.75 }
        $b.SetPixel($x, $y, (White ($a * $edge)))
    }
}
Save $b "select_base.png"

# 網点(8x8、繰り返し用)
$b = New-Bmp 8 8
for ($x = 0; $x -lt 8; $x++) { for ($y = 0; $y -lt 8; $y++) {
    $d = [Math]::Sqrt([Math]::Pow($x - 3.5, 2) + [Math]::Pow($y - 3.5, 2))
    $a = [int](255 * [Math]::Max(0.0, [Math]::Min(1.0, 2.2 - $d)))
    $b.SetPixel($x, $y, (White $a))
} }
Save $b "halftone.png"

# 光(黒背景・加算用)
$b = New-Bmp 256 64
for ($x = 0; $x -lt 256; $x++) { for ($y = 0; $y -lt 64; $y++) {
    $dx = ($x - 127.5) / 128.0; $dy = ($y - 31.5) / 32.0
    $v = [Math]::Max(0.0, 1 - ($dx * $dx + $dy * $dy))
    $c = [int](255 * [Math]::Pow($v, 2))
    $b.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $c, $c, $c))
} }
Save $b "glow.png"

# 未選択のブラケット(縦線＋両端の丸)
$b = New-Bmp 16 64
$g = [System.Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'AntiAlias'
$pen = New-Object System.Drawing.Pen((White 255), 1.5)
$g.DrawLine($pen, 8, 8, 8, 56)
$g.DrawEllipse($pen, 5, 1, 6, 6)
$g.DrawEllipse($pen, 5, 57, 6, 6)
$g.DrawLine($pen, 8, 4, 13, 4)
$g.Dispose(); Save $b "bracket.png"

# 両端がフェードする線
$b = New-Bmp 512 4
for ($x = 0; $x -lt 512; $x++) {
    $t = [Math]::Sin([Math]::PI * $x / 511.0)
    for ($y = 0; $y -lt 4; $y++) {
        $a = if ($y -eq 1 -or $y -eq 2) { 255 * $t } else { 80 * $t }
        $b.SetPixel($x, $y, (White $a))
    }
}
Save $b "line_fade.png"

# 斜めの格子(64x64、繰り返し用)
$b = New-Bmp 64 64
for ($x = 0; $x -lt 64; $x++) { for ($y = 0; $y -lt 64; $y++) {
    $a = 0
    if ((($x + $y) % 64) -eq 0 -or ((($x - $y) + 64) % 64) -eq 0) { $a = 255 }
    $b.SetPixel($x, $y, (White $a))
} }
Save $b "grid.png"

# 縦に流れるドットの列(256x256、繰り返し用)
$b = New-Bmp 256 256
$rnd = New-Object System.Random(7)
foreach ($col in 0..9) {
    $cx = $rnd.Next(0, 64) * 4 + 1
    $bright = $rnd.Next(60, 256)
    $phase = $rnd.Next(0, 256)
    for ($y = 0; $y -lt 256; $y += 6) {
        $t = (($y + $phase) % 256) / 256.0
        $a = [int]($bright * [Math]::Pow($t, 2))
        if ($rnd.Next(0, 5) -eq 0) { continue }
        foreach ($o in @(@(0, 0), @(1, 0), @(0, 1), @(1, 1))) {
            $b.SetPixel(($cx + $o[0]) % 256, ($y + $o[1]) % 256, (White $a))
        }
    }
}
Save $b "rain.png"

# 周辺減光(乗算用。中心が白、端ほど暗い)
$b = New-Bmp 256 256
for ($x = 0; $x -lt 256; $x++) { for ($y = 0; $y -lt 256; $y++) {
    $dx = ($x - 127.5) / 128.0; $dy = ($y - 127.5) / 128.0
    $d = [Math]::Sqrt($dx * $dx + $dy * $dy)
    $t = [Math]::Max(0.0, [Math]::Min(1.0, ($d - 0.5) / 0.9))
    $c = [int](255 * (1 - 0.55 * $t * $t * (3 - 2 * $t)))
    $b.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $c, $c, $c))
} }
Save $b "vignette.png"
