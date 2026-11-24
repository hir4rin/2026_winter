# 死亡演出の「死」の字を Illustrator で作る
# 実行: powershell -ExecutionPolicy Bypass -File MakeDeathText.ps1
# 出力: 2026_winter/data/UI/Death/shi.png(字) / shi_drip.png(垂れ)、out/_preview.png・death_text.ai
$jsx = Join-Path $PSScriptRoot "death_text.jsx"
$ai = New-Object -ComObject Illustrator.Application
$result = $ai.DoJavaScriptFile($jsx)
Write-Host $result
