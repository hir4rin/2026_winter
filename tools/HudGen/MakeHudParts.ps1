# プレイヤーのHUDの画像を hud_parts.html から作る(Edge のヘッドレスで撮る)
# 出力: 2026_winter/data/UI/HUD/hud_frame.png / hud_fill.png / hud_band.png / hud_tassel.png / hud_ultglow.png / hud_spark.png / hud_markon.png / hud_markoff.png
# -HpColor "暗,中,明"(#なし)でHPの中身の色を変える。-Parts fill のように一部だけ作れる
# ultglow / spark は ult_src/ に書く。そのあと ShrinkUltParts.ps1 でゲーム用に縮める
param([string]$OutDir = "", [string]$HpColor = "", [string[]]$Parts = @("frame", "fill", "band", "tassel", "ultglow", "spark", "markon", "markoff"))
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
    @{ Name = "spark";  W = 128;  H = 128 },
    @{ Name = "markon";  W = 64;  H = 80 },
    @{ Name = "markoff"; W = 64;  H = 80 }
)
foreach ($p in ($partDefs | Where-Object { $Parts -contains $_.Name })) {
    $dir = $OutDir
    if ($p.Name -eq "ultglow" -or $p.Name -eq "spark") { $dir = Join-Path $here "ult_src" }
    $out = Join-Path $dir ("hud_" + $p.Name + ".png")
    $argList = @("--headless=new", "--disable-gpu", "--hide-scrollbars", "--force-device-scale-factor=1",
        "--default-background-color=00000000", "--user-data-dir=$env:TEMP\hudgen_edge", "--window-size=$($p.W),$($p.H)", "--virtual-time-budget=1000",
        "--screenshot=$out", "$url`?part=$($p.Name)$q")
    Start-Process -FilePath $edge -ArgumentList $argList -Wait | Out-Null
    Write-Output ("{0} {1}" -f $p.Name, (Test-Path $out))
}
