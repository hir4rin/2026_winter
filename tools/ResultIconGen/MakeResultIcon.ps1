# リザルトの項目アイコン(技・殺・時・道・死)を Illustrator で作る
# 実行: powershell -ExecutionPolicy Bypass -File MakeResultIcon.ps1
# 出力: 2026_winter/data/UI/Result/icon_tech/kill/time/item/death.png、out/_preview.png・result_icon.ai
$jsx = Join-Path $PSScriptRoot "result_icon.jsx"
$ai = New-Object -ComObject Illustrator.Application
$result = $ai.DoJavaScriptFile($jsx)
Write-Host $result
