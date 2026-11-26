# A_Slash1 の赤を青に置き換える(1段目=吸引)
# 赤→青: R と B を入れ替え、G を少し上げて藍〜空色にする
param([string]$Efk = "$PSScriptRoot\A_Slash1.efkefc")
$tool = "$PSScriptRoot\..\..\tools\EfkXml"
$xml = "$env:TEMP\slash1_blue.xml"
& "$tool\Efk.ps1" get $Efk $xml
$doc = New-Object Xml.XmlDocument
$doc.PreserveWhitespace = $true
$doc.Load($xml)

function Get-C($col, $k) {
  $e = $col.SelectSingleNode($k)
  if ($e -and $e.InnerText -ne '') { return [int]$e.InnerText }
  return 255
}
function Set-C($col, $k, $v) {
  $e = $col.SelectSingleNode($k)
  if (-not $e) {
    if ($col.HasAttribute('_empty')) { $col.RemoveAttribute('_empty') }
    $e = $doc.CreateElement($k); [void]$col.AppendChild($e)
  }
  $e.InnerText = [string]$v
}

$paths = 'DrawingValues/Ring/OuterColor_Fixed', 'DrawingValues/Ring/CenterColor_Fixed', 'DrawingValues/Ring/InnerColor_Fixed', 'DrawingValues/ColorAll/Fixed'
foreach ($node in $doc.SelectNodes('//Node')) {
  $name = $node.SelectSingleNode('Name').InnerText
  if ($name -like 'Debug*' -or $name -like 'Mark*') { continue }
  foreach ($p in $paths) {
    $col = $node.SelectSingleNode($p)
    if (-not $col) { continue }
    $r = Get-C $col 'R'; $g = Get-C $col 'G'; $b = Get-C $col 'B'
    if ($r -le $b) { continue }
    $nr = $b
    $ng = [Math]::Max($g, [int][Math]::Round($r * 0.45))
    $nb = $r
    Set-C $col 'R' $nr; Set-C $col 'G' $ng; Set-C $col 'B' $nb
    Write-Output ("{0} {1}: ({2},{3},{4}) -> ({5},{6},{7})" -f $name, $p, $r, $g, $b, $nr, $ng, $nb)
  }
}
$doc.Save($xml)
& "$tool\Efk.ps1" set $Efk $xml
