# リザルトのランクの紋章(C/B/A/S/SSS)の PNG を作る
# rank_emblem_gen.html を Edge のヘッドレスで開き、描いた結果を data/UI/Result/ に保存する
# フォント(Manufacturing Consent / Yuji Boku)は Google Fonts から読むので、ネットにつながっている必要がある
param(
    [int]$Size = 768,
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($OutDir -eq "") { $OutDir = Join-Path $here "..\..\2026_winter\data\UI\Result" }
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force $OutDir | Out-Null

$edge = @(
    "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
    "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $edge) { throw "Edge が見つかりません" }

$page = Join-Path $here "rank_emblem_gen.html"
$url = "file:///" + ($page -replace '\\', '/') + "?size=$Size"
# Edge は標準エラーに関係ないログを出すので、ここだけ止めないようにする
$ErrorActionPreference = "Continue"
$dom = & $edge --headless=new --disable-gpu --allow-file-access-from-files --virtual-time-budget=15000 --dump-dom $url 2>$null | Out-String
$ErrorActionPreference = "Stop"

if ($dom -notmatch "FONTS_OK") { throw "フォントが読めませんでした(ネットの接続を確認)" }

$count = 0
foreach ($m in [regex]::Matches($dom, "PNG (\S+) data:image/png;base64,([A-Za-z0-9+/=]+)")) {
    $path = Join-Path $OutDir ($m.Groups[1].Value + ".png")
    [System.IO.File]::WriteAllBytes($path, [Convert]::FromBase64String($m.Groups[2].Value))
    Write-Host "保存: $path"
    $count++
}
if ($count -ne 5) { throw "書き出せた紋章が $count 枚でした(5枚のはず)" }
