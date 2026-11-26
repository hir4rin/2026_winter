# Embed the title logo images into title_drip_mock_src.html -> title_drip_mock.html
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$img = Join-Path $here "..\..\2026_winter\data\UI\Title"
$src = [IO.File]::ReadAllText((Join-Path $here "title_drip_mock_src.html"), [Text.Encoding]::UTF8)
$map = @{ "__STEEL__" = "logo_steel.png"; "__RED__" = "logo_red.png"; "__BACK__" = "logo_back.png"; "__CUT__" = "logo_cut.png" }
foreach ($k in $map.Keys) {
	$b = [IO.File]::ReadAllBytes((Join-Path $img $map[$k]))
	$src = $src.Replace($k, "data:image/png;base64," + [Convert]::ToBase64String($b))
}
[IO.File]::WriteAllText((Join-Path $here "title_drip_mock.html"), $src, (New-Object Text.UTF8Encoding $false))
"done"
