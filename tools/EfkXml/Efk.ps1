param([string]$Mode, [string]$Efk, [string]$Xml)
# efkefc <-> text xml
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
Add-Type -Path "$dir\BinXml.cs" -ReferencedAssemblies System.Xml
$tmp = "$Xml.bin"
if ($Mode -eq 'get') {
  & "$dir\EfkXml.ps1" get $Efk $tmp
  [IO.File]::WriteAllText($Xml, [BinXml]::ToText([IO.File]::ReadAllBytes($tmp)), (New-Object Text.UTF8Encoding($false)))
} else {
  [IO.File]::WriteAllBytes($tmp, [BinXml]::FromText([IO.File]::ReadAllText($Xml)))
  & "$dir\EfkXml.ps1" set $Efk $tmp
}
Remove-Item $tmp
