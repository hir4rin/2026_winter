# 「死」の字の書体の比べを Illustrator で作る
# 実行: powershell -ExecutionPolicy Bypass -File MakeFontCompare.ps1
# 出力: out/_fonts.png(並べる書体は font_compare.jsx の FONTS)
$jsx = Join-Path $PSScriptRoot "font_compare.jsx"
$ai = New-Object -ComObject Illustrator.Application
$result = $ai.DoJavaScriptFile($jsx)
Write-Host $result
