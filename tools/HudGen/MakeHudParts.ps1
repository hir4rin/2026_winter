# プレイヤーのHUDの画像を hud_parts.html から作る(Edge のヘッドレスで撮る)
# 出力: 2026_winter/data/UI/HUD/hud_frame.png / hud_fill.png / hud_band.png / hud_tassel.png / hud_ultglow.png / hud_spark.png
# -HpColor "暗,中,明"(#なし)でHPの中身の色を変える。-Parts fill のように一部だけ作れる
param([string]$OutDir = "", [string]$HpColor = "", [string[]]$Parts = @("frame", "fill", "band", "tassel", "ultglow", "spark"))
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($OutDir -eq "") { $OutDir = Join-Path $here "..\..\2026_winter\data\UI\HUD" }
$OutDir = (Resolve-Path $OutDir).Path
$edge = "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
if (-not (Test-Path $edge)) { $edge = "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe" }
$html = (Resolve-Path (Join-Path $here "hud_parts.html")).Path
$url = "file:///" + ($html -replace "\\", "/")

$q = ""
if ($HpColor -ne "") { $q = "&hp=" + $HpColor }
$partDefs = @(
    @{ Name = "frame";  W = 1024; H = 256 },
    @{ Name = "fill";   W = 1024; H = 256 },
    @{ Name = "band";   W = 1024; H = 256 },
    @{ Name = "tassel"; W = 64;   H = 96 },
    @{ Name = "ultglow"; W = 1100; H = 1142 },
    @{ Name = "spark";  W = 128;  H = 128 }
)
foreach ($p in ($partDefs | Where-Object { $Parts -contains $_.Name })) {
    $out = Join-Path $OutDir ("hud_" + $p.Name + ".png")
    $argList = @("--headless=new", "--disable-gpu", "--hide-scrollbars", "--force-device-scale-factor=1",
        "--default-background-color=00000000", "--window-size=$($p.W),$($p.H)", "--virtual-time-budget=1000",
        "--screenshot=$out", "$url`?part=$($p.Name)$q")
    Start-Process -FilePath $edge -ArgumentList $argList -Wait | Out-Null
    Write-Output ("{0} {1}" -f $p.Name, (Test-Path $out))
}
