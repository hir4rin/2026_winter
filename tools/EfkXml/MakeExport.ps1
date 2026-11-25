param([string]$InXml, [string]$TemplateEfk, [string]$OutEfk)
. "$PSScriptRoot\Lib.ps1"
$d = Load-X $InXml
foreach ($n in @('Debug','MarkFront','MarkRight','DebugDummy')) {
  $x = $d.SelectSingleNode("//Node[Name='$n']")
  if ($x) { [void]$x.ParentNode.RemoveChild($x) }
}
$tmp = "$OutEfk.xml"
Save-X $d $tmp
Copy-Item $TemplateEfk $OutEfk -Force
& "$PSScriptRoot\Efk.ps1" set $OutEfk $tmp
Remove-Item $tmp
