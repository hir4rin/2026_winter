# guardbreak_steal_mock_src.html の画像の場所に、ゲームの血殺の文字の画像を埋め込んで guardbreak_steal_mock.html を作る
$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$ui = Join-Path $here '..\..\2026_winter\data\UI\bloodUI'
function Get-DataUri($path) { 'data:image/png;base64,' + [Convert]::ToBase64String([IO.File]::ReadAllBytes($path)) }
$src = [IO.File]::ReadAllText((Join-Path $here 'guardbreak_steal_mock_src.html'), [Text.Encoding]::UTF8)
$src = $src.Replace('__BLOOD__', (Get-DataUri (Join-Path $ui 'blood_UI.png')))
$src = $src.Replace('__SATSU__', (Get-DataUri (Join-Path $ui 'satu_UI.png')))
$src = $src.Replace('__SPLASH__', (Get-DataUri (Join-Path $ui 'splash2_UI.png')))
[IO.File]::WriteAllText((Join-Path $here 'guardbreak_steal_mock.html'), $src, (New-Object Text.UTF8Encoding $false))
'ok'
