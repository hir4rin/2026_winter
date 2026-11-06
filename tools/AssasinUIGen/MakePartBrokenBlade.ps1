# 確殺UIの刃のPNGを作る(白一色。色はゲームの SetDrawBright で付ける)
# 実行: powershell -ExecutionPolicy Bypass -File MakePartBrokenBlade.ps1
# 出力: 2026_winter/data/UI/PartBrokenKill/blade.png
Add-Type -AssemblyName System.Drawing

$out = Join-Path $PSScriptRoot "..\..\2026_winter\data\UI\PartBrokenKill"
New-Item -ItemType Directory -Force $out | Out-Null

# 大きさは PartBrokenKillUI.cpp の kBladeImageWidth/Height と合わせる
$w = 256
$h = 40

$b = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($b)
$g.SmoothingMode = 'AntiAlias'

# 両端がとがったひし形
$white = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$points = [System.Drawing.PointF[]]@(
    (New-Object System.Drawing.PointF(2.0, 20.0)),
    (New-Object System.Drawing.PointF(128.0, 2.0)),
    (New-Object System.Drawing.PointF(254.0, 20.0)),
    (New-Object System.Drawing.PointF(128.0, 38.0))
)
$g.FillPolygon($white, $points)

$g.Dispose()
$path = Join-Path $out "blade.png"
$b.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
$b.Dispose()
Write-Host "saved $path"
