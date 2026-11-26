# 必殺技ゲージの画像を、ゲームで描く大きさにきれいに縮める(GPUで大きく縮めると縄や字の縁がギザギザになるため)
# 元: tools/HudGen/ult_src/ の大きい画像 → 出力: 2026_winter/data/UI/HUD/
# ゲームでは等倍で描く(画面は 1280x720 固定)(PlayerHUD.cpp の kUlt〜Scale)。描く大きさを変えたら下の倍率も直す
param([string]$OutDir = "")
Add-Type -AssemblyName System.Drawing
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Join-Path $here "ult_src"
if ($OutDir -eq "") { $OutDir = Join-Path $here "..\..\2026_winter\data\UI\HUD" }
$OutDir = (Resolve-Path $OutDir).Path

# 名前と倍率(ゲームで描く大きさ ÷ 元の大きさ)
$ringRate = 150.0 / 910.0
$parts = @(
    @{ Name = "UltGauge";        Rate = $ringRate },
    @{ Name = "UltGaugeColor";   Rate = $ringRate },
    @{ Name = "hud_ultglow";     Rate = $ringRate },
    @{ Name = "UltGaugeBase";    Rate = 115.0 / 453.0 },
    @{ Name = "UltGauge_circle"; Rate = 160.0 / 436.0 },
    @{ Name = "hud_spark";       Rate = 24.0 / 128.0 }
)
foreach ($p in $parts) {
    $img = [System.Drawing.Image]::FromFile((Join-Path $src ($p.Name + ".png")))
    $w = [int][Math]::Round($img.Width * $p.Rate)
    $h = [int][Math]::Round($img.Height * $p.Rate)
    $bmp = New-Object System.Drawing.Bitmap $w, $h, ([System.Drawing.Imaging.PixelFormat]::Format32bppPArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.CompositingMode = 'SourceCopy'
    $g.InterpolationMode = 'HighQualityBicubic'
    $g.PixelOffsetMode = 'HighQuality'
    $attr = New-Object System.Drawing.Imaging.ImageAttributes
    $attr.SetWrapMode([System.Drawing.Drawing2D.WrapMode]::TileFlipXY)
    $g.DrawImage($img, (New-Object System.Drawing.Rectangle 0, 0, $w, $h), 0, 0, $img.Width, $img.Height, ([System.Drawing.GraphicsUnit]::Pixel), $attr)
    $g.Dispose()
    $img.Dispose()
    $out = Join-Path $OutDir ($p.Name + ".png")
    $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output ("{0} {1}x{2}" -f $p.Name, $w, $h)
}
