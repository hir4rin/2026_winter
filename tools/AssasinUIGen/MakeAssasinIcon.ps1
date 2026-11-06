# 暗殺UIの丸いアイコンを作る
# 実行: powershell -ExecutionPolicy Bypass -File MakeAssasinIcon.ps1
# 出力: 2026_winter/data/UI/Assasin/icon.png
Add-Type -AssemblyName System.Drawing

$out = Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\Assasin"
New-Item -ItemType Directory -Force $out | Out-Null

# 丸の中心と半径(AssasinTargetUI.cpp の kIconCenterX/Y と合わせる)
$cx = 64.0
$cy = 52.0
$radius = 46.0

$b = New-Object System.Drawing.Bitmap(128, 128, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'AntiAlias'

$red = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 200, 24, 34))
$dark = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 18, 8, 10))
$linePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 120, 18, 26), 1.5)

function Add-Circle($brush, $r) {
    $g.FillEllipse($brush, [float]($cx - $r), [float]($cy - $r), [float]($r * 2), [float]($r * 2))
}

# 丸の下から垂れる血
function Add-Drip($x, $top, $length, $neck, $bulb) {
    $p1 = New-Object System.Drawing.PointF([float]($x - $neck), [float]$top)
    $p2 = New-Object System.Drawing.PointF([float]($x + $neck), [float]$top)
    $p3 = New-Object System.Drawing.PointF([float]$x, [float]($top + $length))
    $g.FillPolygon($red, [System.Drawing.PointF[]]@($p1, $p2, $p3))
    $g.FillEllipse($red, [float]($x - $bulb), [float]($top + $length - $bulb * 1.4), [float]($bulb * 2), [float]($bulb * 2.8))
}
Add-Drip 44 86 18 7 3.0
Add-Drip 64 92 28 8 4.5
Add-Drip 82 88 14 6 2.5

# 赤い縁・黒い中身・内側の細い線
Add-Circle $red $radius
Add-Circle $dark ($radius - 6)
$g.DrawEllipse($linePen, [float]($cx - 35), [float]($cy - 35), 70.0, 70.0)

$g.Dispose()
$path = Join-Path $out "icon.png"
$b.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
$b.Dispose()
Write-Host "saved $path"
