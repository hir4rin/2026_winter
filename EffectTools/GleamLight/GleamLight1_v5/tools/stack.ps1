param([string]$Out, [string]$List)
$Items = $List.Split(";")
Add-Type -AssemblyName System.Drawing
$imgs = @(); $labels = @()
foreach ($it in $Items) { $p = $it.Split('|'); $labels += $p[0]; $imgs += [System.Drawing.Image]::FromFile((Resolve-Path $p[1]).Path) }
$s = 0.5; $w = [int]($imgs[0].Width * $s); $h = [int]($imgs[0].Height * $s); $lh = 26
$bmp = New-Object System.Drawing.Bitmap ($w, (($h + $lh) * $imgs.Count))
$g = [System.Drawing.Graphics]::FromImage($bmp); $g.Clear([System.Drawing.Color]::Black)
$f = New-Object System.Drawing.Font ('Yu Gothic UI', 13, [System.Drawing.FontStyle]::Bold)
for ($i = 0; $i -lt $imgs.Count; $i++) { $y = $i * ($h + $lh); $g.DrawString($labels[$i], $f, [System.Drawing.Brushes]::White, 6, $y + 2); $g.DrawImage($imgs[$i], 0, $y + $lh, $w, $h) }
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
