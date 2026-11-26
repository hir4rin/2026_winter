# A_Slash2 を放出の形にする(1段目の吸引の逆)
# 始まりに中心から光がはじける / 振った輪と粒が外へ押し出される / 着地で地面に衝撃波が広がる
# 2回流すとノードが増えるので、元の版(git)から流す
param([string]$Efk = "$PSScriptRoot\A_Slash2.efkefc")
$tool = "$PSScriptRoot\..\..\tools\EfkXml"
. "$tool\Lib.ps1"
$xml = "$env:TEMP\slash2_release.xml"
& "$tool\Efk.ps1" get $Efk $xml
$doc = Load-X $xml

# 残る輪: 外へ広がって消える(速く始まって止まる)
foreach ($pair in @(@('InkLinger', 26), @('InkWallLinger', 22), @('EdgeLinger', 18), @('RedWallLinger', 18))) {
  $node = Get-N $doc $pair[0]
  Set-R $node 'CommonValues.Life' $pair[1]
  Set-V $node 'ScalingValues.Type' 2
  foreach ($ax in 'X', 'Y', 'Z') {
    Set-R $node "ScalingValues.Easing.Start.$ax" 1
    Set-R $node "ScalingValues.Easing.End.$ax" 2.0
  }
  Set-V $node 'ScalingValues.Easing.StartSpeed' 20
  Set-V $node 'ScalingValues.Easing.EndSpeed' -20
  Set-V $node 'RendererCommonValues.FadeOut.Frame' ($pair[1] - 4)
}

# 弧の先の粒: 外へ飛ばす(Head の Z が外向き)
$sm = Get-N $doc 'Smoke'
Set-R $sm 'LocationValues.PVA.Velocity.Z' 0.25 0.15 0.35
Set-R $sm 'CommonValues.Life' 34 28 40
Set-V $sm 'RendererCommonValues.FadeOut.Frame' 20

$em = Get-N $doc 'Ember'
Set-R $em 'LocationValues.PVA.Velocity.Z' 0.8 0.5 1.1
Set-R $em 'LocationValues.PVA.Velocity.Y' 0.08 0 0.16
Set-R $em 'CommonValues.Life' 18 14 22
Set-R $em 'CommonValues.Generation.GenerationTime' 0.1

$mist = Get-N $doc 'RedMist'
Set-R $mist 'LocationValues.PVA.Velocity.Z' 0.45 0.3 0.6

# 始まりに中心からはじける光と粒(1段目の Core の逆)
$imp = Get-N $doc 'ImpactRed'
$flash = Copy-N $imp 'ReleaseFlash' -Before
Set-R $flash 'CommonValues.Generation.GenerationTimeOffset' 0
Set-R $flash 'CommonValues.Life' 12
Set-V $flash 'LocationValues.Fixed.Location.Y' 5.5
Set-V $flash 'LocationValues.Fixed.Location.Z' 0
Set-R $flash 'ScalingValues.Easing.Start.X' 8; Set-R $flash 'ScalingValues.Easing.Start.Y' 8
Set-R $flash 'ScalingValues.Easing.End.X' 60; Set-R $flash 'ScalingValues.Easing.End.Y' 60
Set-V $flash 'RendererCommonValues.FadeOut.Frame' 9
Set-Col $flash 'DrawingValues.ColorAll.Fixed' 255 90 80 255

$head = Get-N $doc 'Head'
$burst = Copy-N $head 'BurstHead'
Set-R $burst 'CommonValues.Generation.GenerationTimeOffset' 0
Set-R $burst 'CommonValues.Life' 5
Set-R $burst 'RotationValues.Easing.Start.Y' 0
Set-R $burst 'RotationValues.Easing.End.Y' -1080
foreach ($c in @($burst.SelectNodes('Children/Node'))) {
  if ($c.SelectSingleNode('Name').InnerText -ne 'Ember') { [void]$c.ParentNode.RemoveChild($c) }
}
$be = $burst.SelectSingleNode("Children/Node[Name='Ember']")
Set-V $be 'Name' 'BurstEmber'
Set-R $be 'LocationValues.PVA.Location.Z' 2 1 3
Set-R $be 'LocationValues.PVA.Location.Y' 5.5 3 8
Set-R $be 'LocationValues.PVA.Velocity.Z' 1.4 1.0 1.8
Set-R $be 'LocationValues.PVA.Velocity.Y' 0 -0.1 0.1
Set-R $be 'CommonValues.Life' 14 11 17
Set-R $be 'CommonValues.Generation.GenerationTime' 0.03
Set-R $be 'ScalingValues.SinglePVA.Scale' 1.4 1.0 1.8
Set-Col $be 'DrawingValues.ColorAll.Fixed' 255 110 90 255

# 着地の光: 縮むのをやめて外へ広がる
Set-R $imp 'ScalingValues.Easing.Start.X' 8; Set-R $imp 'ScalingValues.Easing.Start.Y' 8
Set-R $imp 'ScalingValues.Easing.End.X' 30; Set-R $imp 'ScalingValues.Easing.End.Y' 30

# 着地の衝撃波: 地面の輪が外へ広がる
$sw = Copy-N (Get-N $doc 'EdgeLinger') 'Shockwave'
Set-R $sw 'CommonValues.Generation.GenerationTimeOffset' 21
Set-R $sw 'CommonValues.Life' 16
Set-V $sw 'LocationValues.Fixed.Location.Y' 0.3
Set-V $sw 'DrawingValues.Ring.RingShape.Type' 0
Set-V $sw 'DrawingValues.Ring.Outer_Fixed.Location.Y' 0
Set-V $sw 'DrawingValues.Ring.Inner_Fixed.Location.Y' 0
Set-V $sw 'DrawingValues.Ring.Outer_Fixed.Location.X' 10
Set-V $sw 'DrawingValues.Ring.Inner_Fixed.Location.X' 7.8
foreach ($ax in 'X', 'Y', 'Z') {
  Set-R $sw "ScalingValues.Easing.Start.$ax" 0.15
  Set-R $sw "ScalingValues.Easing.End.$ax" 2.6
}
Set-V $sw 'RendererCommonValues.FadeOut.Frame' 12

Save-X $doc $xml
& "$tool\Efk.ps1" set $Efk $xml
