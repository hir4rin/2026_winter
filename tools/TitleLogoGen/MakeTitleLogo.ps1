# タイトルロゴを Illustrator で作る
# 実行: powershell -ExecutionPolicy Bypass -File MakeTitleLogo.ps1
# 出力: このフォルダの out/<案の名前>.png(透過)・_preview.png(暗い背景つき)・.ai(手直し用)
#       決定版(Shinobu_vertical)は 2026_winter/data/UI/Title/logo.png にも書き出す
$jsx = Join-Path $PSScriptRoot "title_logo.jsx"
$ai = New-Object -ComObject Illustrator.Application
$result = $ai.DoJavaScriptFile($jsx)
Write-Host $result
