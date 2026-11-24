# lowhp_mock_src.html に画像を埋め込んで lowhp_mock.html を作る
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Get-Content -Raw -Encoding UTF8 (Join-Path $dir "lowhp_mock_src.html")
$bg = [Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $dir "bg_game.jpg")))
$inkPath = Join-Path $dir "..\..\2026_winter\data\UI\bloodUI\ink_vignette.png"
$ink = [Convert]::ToBase64String([IO.File]::ReadAllBytes($inkPath))
$out = $src.Replace("__BG__", "data:image/jpeg;base64,$bg").Replace("__INK__", "data:image/png;base64,$ink")
[IO.File]::WriteAllText((Join-Path $dir "lowhp_mock.html"), $out, (New-Object System.Text.UTF8Encoding $false))
"lowhp_mock.html を作った"
