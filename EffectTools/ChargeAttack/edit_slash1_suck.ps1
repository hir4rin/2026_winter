# A_Slash1 を吸引の形にする(edit_slash1_blue.ps1 のあとに流す)
# 振り終わりの輪が中心へ縮んで消える / 粒が外から内へ流れる / 最後に中心で光が縮む
param([string]$Efk = "$PSScriptRoot\A_Slash1.efkefc")
$tool = "$PSScriptRoot\..\..\tools\EfkXml"
. "$tool\Lib.ps1"
Add-Type -Path "$tool\BinXml.cs" -ReferencedAssemblies System.Xml
$xml = "$env:TEMP\slash1_suck.xml"
$xml2 = "$env:TEMP\slash1_suck_src2.xml"
& "$tool\Efk.ps1" get $Efk $xml
& "$tool\Efk.ps1" get "$PSScriptRoot\A_Slash2.efkefc" $xml2
$doc = Load-X $xml
$doc2 = Load-X $xml2

$collapseStart = 18
$collapseLife = 20

# 残る輪: 中心へ縮む(ゆっくり始まって速く吸い込まれる)
foreach ($n in 'InkLinger', 'InkWallLinger', 'EdgeLinger', 'RedWallLinger') {
  $node = Get-N $doc $n
  Set-R $node 'CommonValues.Life' $collapseLife
  Set-V $node 'ScalingValues.Type' 2
  foreach ($ax in 'X', 'Y', 'Z') {
    Set-R $node "ScalingValues.Easing.Start.$ax" 1
    Set-R $node "ScalingValues.Easing.End.$ax" 0.08
  }
  Set-V $node 'ScalingValues.Easing.StartSpeed' -20
  Set-V $node 'ScalingValues.Easing.EndSpeed' 20
  Set-V $node 'RendererCommonValues.FadeOut.Frame' 6
}

# 弧の先の粒: 外側に出して内へ流す(Head の Z が外向き)
$sm = Get-N $doc 'Smoke'
Set-R $sm 'LocationValues.PVA.Location.Z' 14 12 16
Set-R $sm 'LocationValues.PVA.Velocity.Y' 0 -0.01 0.01
Set-R $sm 'LocationValues.PVA.Velocity.Z' -0.3 -0.4 -0.2
Set-R $sm 'CommonValues.Life' 30 26 34
Set-V $sm 'RendererCommonValues.FadeOut.Frame' 14
Set-R $sm 'ScalingValues.SinglePVA.Velocity' -0.04 -0.05 -0.03

$em = Get-N $doc 'Ember'
Set-R $em 'LocationValues.PVA.Location.Z' 16 13 19
Set-R $em 'LocationValues.PVA.Location.Y' 5.5 4 7
Set-R $em 'LocationValues.PVA.Velocity.Y' 0 -0.02 0.02
Set-R $em 'LocationValues.PVA.Velocity.Z' -0.7 -0.9 -0.5
Set-R $em 'CommonValues.Life' 16 13 19
Set-R $em 'CommonValues.Generation.GenerationTime' 0.12
Set-V $em 'RendererCommonValues.FadeOut.Frame' 5

$mist = Get-N $doc 'RedMist'
Set-V $mist 'Name' 'BlueMist'
Set-R $mist 'LocationValues.PVA.Location.Z' 13 11 15
Set-R $mist 'LocationValues.PVA.Velocity.Y' 0
Set-R $mist 'LocationValues.PVA.Velocity.Z' -0.45 -0.55 -0.35
Set-R $mist 'CommonValues.Life' 18 15 21

# 外から吸い寄せられる粒: 弧と別に速く回る None から、遠くに出して内へ
$head = Get-N $doc 'Head'
$pull = Copy-N $head 'PullHead'
Set-R $pull 'CommonValues.Life' 26
Set-R $pull 'RotationValues.Easing.Start.Y' 0
Set-R $pull 'RotationValues.Easing.End.Y' -900
foreach ($c in @($pull.SelectNodes('Children/Node'))) {
  if ($c.SelectSingleNode('Name').InnerText -ne 'Ember') { [void]$c.ParentNode.RemoveChild($c) }
}
$pe = $pull.SelectSingleNode("Children/Node[Name='Ember']")
Set-V $pe 'Name' 'PullEmber'
Set-R $pe 'LocationValues.PVA.Location.Z' 22 19 25
Set-R $pe 'LocationValues.PVA.Location.Y' 5.5 2 9
Set-R $pe 'LocationValues.PVA.Velocity.Z' -1.1 -1.3 -0.9
Set-R $pe 'LocationValues.PVA.Velocity.Y' 0
Set-R $pe 'CommonValues.Life' 17 15 19
Set-R $pe 'CommonValues.Generation.GenerationTime' 0.12
Set-R $pe 'ScalingValues.SinglePVA.Scale' 0.8 0.6 1.0
Set-Col $pe 'DrawingValues.ColorAll.Fixed' 120 190 255 255

# 吸い込み終わりに中心で光が縮む(2段目の着地の光を持ってくる)
$core = Import-N $doc (Get-N $doc2 'ImpactRed') 'Core' (Root-Children $doc)
Set-R $core 'CommonValues.Generation.GenerationTimeOffset' ($collapseStart + $collapseLife - 6)
Set-V $core 'LocationValues.Fixed.Location.Y' 5.5
Set-V $core 'LocationValues.Fixed.Location.Z' 0
Set-R $core 'ScalingValues.Easing.Start.X' 28; Set-R $core 'ScalingValues.Easing.Start.Y' 28
Set-R $core 'ScalingValues.Easing.End.X' 3; Set-R $core 'ScalingValues.Easing.End.Y' 3
Set-Col $core 'DrawingValues.ColorAll.Fixed' 150 200 255 255

Save-X $doc $xml
& "$tool\Efk.ps1" set $Efk $xml
