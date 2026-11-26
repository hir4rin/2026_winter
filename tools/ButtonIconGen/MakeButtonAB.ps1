# Photoshop で A・B のボタン画像を作る(出力は out/)
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$out = Join-Path $here "out"
New-Item -ItemType Directory -Force $out | Out-Null
$js = [IO.File]::ReadAllText((Join-Path $here "MakeButtonAB.jsx"))
$js = $js.Replace('$.getenv("BTN_OUT") || "C:/temp"', '"' + ($out -replace '\\','/') + '"')
$ps = New-Object -ComObject Photoshop.Application
$ps.DoJavaScript($js)