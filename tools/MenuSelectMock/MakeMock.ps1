# menu_select_mock_src.html に data/UI の画像を埋め込んで menu_select_mock.html を作る
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$data = Join-Path $here "..\..\2026_winter\data\UI"
$files = [ordered]@{
  frame = "Menu\frame9.png"; selectBase = "Menu\select_base.png"; halftone = "Menu\halftone.png"
  glow = "Menu\glow.png"; bracket = "Menu\bracket.png"; lineFade = "Menu\line_fade.png"
  grid = "Menu\grid.png"; rain = "Menu\rain.png"; vignette = "Menu\vignette.png"; btnA = "button\A.png"
}
$parts = foreach ($k in $files.Keys) {
  $b64 = [Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $data $files[$k])))
  "  ${k}: 'data:image/png;base64,$b64'"
}
$obj = "{`n" + ($parts -join ",`n") + "`n}"
$src = [IO.File]::ReadAllText((Join-Path $here "menu_select_mock_src.html"), [Text.Encoding]::UTF8)
$out = $src.Replace("/*SRC*/{}/*SRC*/", $obj)
[IO.File]::WriteAllText((Join-Path $here "menu_select_mock.html"), $out, (New-Object Text.UTF8Encoding $false))
"done"
