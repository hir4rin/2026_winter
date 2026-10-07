# Composes a labelled comparison grid from capture images (same camera / same frame shots).
# MCP captures (%LOCALAPPDATA%\EffekseerMcp\captures) are cleaned up after a while, so run
# this right after capturing and keep the result in the work folder.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File compose_compare.ps1 -Out compare_v1_v2.png `
#     -ColLabels "正面,45°,真上,真下" `
#     -Rows "v1|a.png|b.png|c.png|d.png,v2|e.png|f.png|g.png|h.png"
# Rows are separated by "," and each row is "label|image1|image2|...".
# Missing images are drawn as an empty frame (with a warning).
param(
    [Parameter(Mandatory = $true)][string]$Out,
    [Parameter(Mandatory = $true)][string[]]$Rows,
    [string[]]$ColLabels = @(),
    [int]$Cell = 300,
    [int]$Gap = 10,
    [int]$LabelWidth = 90,
    [int]$HeaderHeight = 36
)
Add-Type -AssemblyName System.Drawing

# "powershell -File" passes a,b,c as ONE string: split it back into items
if ($Rows.Count -eq 1 -and $Rows[0].Contains(',')) { $Rows = $Rows[0] -split ',' }
if ($ColLabels.Count -eq 1 -and $ColLabels[0].Contains(',')) { $ColLabels = $ColLabels[0] -split ',' }

$parsed = New-Object System.Collections.Generic.List[object]
foreach ($r in $Rows) { $parsed.Add([string[]]($r.Trim() -split '\|')) }
$cols = 0
foreach ($row in $parsed) { if ($row.Length - 1 -gt $cols) { $cols = $row.Length - 1 } }
$w = $LabelWidth + $cols * ($Cell + $Gap)
$h = $HeaderHeight + $parsed.Count * ($Cell + $Gap)

$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.Clear([System.Drawing.Color]::Black)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$font = New-Object System.Drawing.Font "Meiryo", 14
$brush = [System.Drawing.Brushes]::White
$fmt = New-Object System.Drawing.StringFormat; $fmt.Alignment = [System.Drawing.StringAlignment]::Center

for ($j = 0; $j -lt $ColLabels.Count -and $j -lt $cols; $j++) {
    $rect = New-Object System.Drawing.RectangleF ($LabelWidth + $j * ($Cell + $Gap)), 6, $Cell, ($HeaderHeight - 6)
    $g.DrawString($ColLabels[$j], $font, $brush, $rect, $fmt)
}
for ($r = 0; $r -lt $parsed.Count; $r++) {
    $row = $parsed[$r]; $y = $HeaderHeight + $r * ($Cell + $Gap)
    $g.DrawString($row[0], $font, $brush, 10, $y + $Cell / 2 - 12)
    for ($j = 1; $j -lt $row.Length; $j++) {
        $x = $LabelWidth + ($j - 1) * ($Cell + $Gap)
        $path = $row[$j]
        if ($path -and (Test-Path -LiteralPath $path)) {
            $img = [System.Drawing.Image]::FromFile((Resolve-Path -LiteralPath $path).Path)
            $g.DrawImage($img, $x, $y, $Cell, $Cell); $img.Dispose()
        } else {
            $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(80, 255, 255, 255))
            $g.DrawRectangle($pen, $x, $y, $Cell - 1, $Cell - 1)
            if ($path) { Write-Warning "missing image: $path" }
        }
    }
}
if ([System.IO.Path]::IsPathRooted($Out)) { $outFull = $Out } else { $outFull = Join-Path (Get-Location).Path $Out }
$outFull = [System.IO.Path]::GetFullPath($outFull)
$bmp.Save($outFull, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
"wrote $outFull"
