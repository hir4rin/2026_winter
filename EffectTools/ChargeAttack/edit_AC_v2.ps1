# A / C: stronger red, bigger, walls so it reads from the side
param([string]$InDir, [string]$OutDir)
. "$PSScriptRoot\..\..\tools\EfkXml\Lib.ps1"

# ---------- A slashes ----------
function Edit-ASlash($doc, $y, $r) {
  # $r: outer radius of the flat ink ring
  foreach ($n in 'InkSweep', 'InkLinger') {
    $x = Get-N $doc $n
    Set-Ring $x $r (-0.15 * $r) 3.8 0
    Set-RingCol $x @(110, 8, 20, 240) @(45, 4, 11, 235) @(22, 3, 8, 90)
  }
  foreach ($n in 'EdgeSweep', 'EdgeLinger') {
    $x = Get-N $doc $n
    Set-Ring $x ($r + 0.8) (-0.165 * ($r + 0.8)) ($r - 0.9) (-0.15 * ($r - 0.9))
    Set-RingCol $x @(255, 30, 40, 255) @(255, 130, 110, 255) @(255, 30, 40, 255)
  }
  # ink wall (tall, smoky)
  foreach ($pair in @(@('InkSweep', 'InkWallSweep'), @('InkLinger', 'InkWallLinger'))) {
    $w = Copy-N (Get-N $doc $pair[0]) $pair[1]
    Set-Ring $w ($r + 0.6) -3.0 ($r - 0.4) 2.6
    Set-RingCol $w @(30, 3, 8, 0) @(32, 3, 9, 215) @(20, 2, 6, 0)
    if ($pair[1] -eq 'InkWallLinger') {
      Set-R $w 'CommonValues.Life' 30
      Set-V $w 'RendererCommonValues.FadeOut.Frame' 28
    }
  }
  # red glow wall (additive)
  foreach ($pair in @(@('EdgeSweep', 'RedWallSweep'), @('EdgeLinger', 'RedWallLinger'))) {
    $w = Copy-N (Get-N $doc $pair[0]) $pair[1]
    Set-V $w 'RendererCommonValues.ColorTexture' 'Texture/CA_SmokeArc.png'
    Set-Ring $w ($r + 0.9) -2.1 ($r + 0.2) 1.5
    Set-RingCol $w @(255, 20, 30, 0) @(255, 40, 48, 185) @(255, 20, 30, 0)
  }
  # head particles
  $sm = Get-N $doc 'Smoke'
  Set-Col $sm 'DrawingValues.ColorAll.Fixed' 70 6 15 175
  Set-R $sm 'LocationValues.PVA.Location.Z' ($r * 0.84) ($r * 0.6) ($r * 1.08)
  Set-R $sm 'ScalingValues.SinglePVA.Scale' 2.3 1.7 2.9
  $em = Get-N $doc 'Ember'
  Set-R $em 'LocationValues.PVA.Location.Z' ($r * 0.98) ($r * 0.88) ($r * 1.08)
  Set-R $em 'CommonValues.Generation.GenerationTime' 0.18
  Set-R $em 'ScalingValues.SinglePVA.Scale' 0.5 0.35 0.65
  Set-Col $em 'DrawingValues.ColorAll.Fixed' 255 60 50 255
  # red mist along the arc (billboard: reads from any angle)
  $mist = Copy-N $em 'RedMist'
  Set-R $mist 'CommonValues.Life' 16 12 20
  Set-R $mist 'CommonValues.Generation.GenerationTime' 0.35
  Set-R $mist 'LocationValues.PVA.Location.Y' $y ($y - 1.5) ($y + 1.5)
  Set-R $mist 'LocationValues.PVA.Location.Z' ($r * 0.98) ($r * 0.88) ($r * 1.08)
  Set-R $mist 'LocationValues.PVA.Velocity.Y' 0.02 0 0.04
  Set-R $mist 'LocationValues.PVA.Velocity.Z' 0.02 0 0.04
  Set-R $mist 'LocationValues.PVA.Velocity.X' 0 -0.02 0.02
  Set-R $mist 'ScalingValues.SinglePVA.Scale' 2.6 2.0 3.2
  Set-Col $mist 'DrawingValues.ColorAll.Fixed' 255 25 35 95
  Set-V $mist 'RendererCommonValues.FadeInType' 1
  Set-V $mist 'RendererCommonValues.FadeIn.Frame' 2
}

# ---------- C slashes ----------
function Edit-CSlash($doc, $y, $r) {
  foreach ($n in 'InkSweep', 'InkLinger') {
    $x = Get-N $doc $n
    Set-Ring $x $r (-0.15 * $r) 3.6 0
  }
  # vermilion brush under the ink
  foreach ($pair in @(@('InkSweep', 'RedSweep'), @('InkLinger', 'RedLinger'))) {
    $w = Copy-N (Get-N $doc $pair[0]) $pair[1] -Before
    Set-Ring $w ($r + 1.0) (-0.155 * ($r + 1.0)) ($r - 2.6) (-0.1 * ($r - 2.6))
    Set-RingCol $w @(175, 10, 24, 240) @(160, 9, 22, 240) @(150, 8, 20, 200)
  }
  # ink wall
  foreach ($pair in @(@('InkSweep', 'InkWallSweep'), @('InkLinger', 'InkWallLinger'))) {
    $w = Copy-N (Get-N $doc $pair[0]) $pair[1]
    Set-Ring $w ($r + 0.4) -2.8 ($r - 0.4) 2.4
    Set-RingCol $w @(14, 5, 9, 0) @(16, 5, 10, 225) @(16, 5, 10, 50)
    if ($pair[1] -eq 'InkWallLinger') {
      Set-R $w 'CommonValues.Life' 34
      Set-V $w 'RendererCommonValues.FadeOut.Frame' 30
    }
  }
  # red wall (additive)
  foreach ($pair in @(@('InkSweep', 'RedWallSweep'), @('InkLinger', 'RedWallLinger'))) {
    $w = Copy-N (Get-N $doc $pair[0]) $pair[1]
    Set-V $w 'RendererCommonValues.ColorTexture' 'Texture/CA_SmokeArc.png'
    Set-V $w 'RendererCommonValues.AlphaBlend' 2
    Set-Ring $w ($r + 0.8) -1.9 ($r + 0.2) 1.3
    Set-RingCol $w @(255, 20, 30, 0) @(255, 40, 45, 210) @(255, 20, 30, 0)
    if ($pair[1] -eq 'RedWallLinger') {
      Set-R $w 'CommonValues.Life' 26
      Set-V $w 'RendererCommonValues.FadeOut.Frame' 20
      Set-V $w 'RendererCommonValues.FadeOut.StartSpeed' 0
    }
  }
  # drips: dark red + bright red
  $dr = Get-N $doc 'Drip'
  Set-Col $dr 'DrawingValues.ColorAll.Fixed' 55 4 10 255
  Set-R $dr 'LocationValues.PVA.Location.Z' ($r * 0.8) ($r * 0.55) ($r * 1.05)
  Set-R $dr 'ScalingValues.SinglePVA.Scale' 0.38 0.25 0.5
  $rd = Copy-N $dr 'RedDrip'
  Set-Col $rd 'DrawingValues.ColorAll.Fixed' 200 14 26 255
  Set-R $rd 'CommonValues.Generation.GenerationTime' 0.45
}

function Edit-CImpact($doc, $scale) {
  $ink = Get-N $doc 'ImpactInk'
  Set-R $ink 'ScalingValues.Easing.Start.X' (4 * $scale); Set-R $ink 'ScalingValues.Easing.Start.Y' (4 * $scale)
  Set-R $ink 'ScalingValues.Easing.End.X' (15 * $scale); Set-R $ink 'ScalingValues.Easing.End.Y' (15 * $scale)
  $red = Copy-N $ink 'ImpactRedInk' -Before
  Set-R $red 'ScalingValues.Easing.Start.X' (5 * $scale); Set-R $red 'ScalingValues.Easing.Start.Y' (5 * $scale)
  Set-R $red 'ScalingValues.Easing.End.X' (19 * $scale); Set-R $red 'ScalingValues.Easing.End.Y' (19 * $scale)
  Set-Col $red 'DrawingValues.ColorAll.Fixed' 175 10 24 235
  $dp = Get-N $doc 'ImpactDrops'
  Set-R $dp 'GenerationLocationValues.Circle.Radius' (2.5 * $scale) (1.5 * $scale) (3.5 * $scale)
  Set-R $dp 'LocationValues.PVA.Velocity.X' 0 -0.3 0.3
  Set-R $dp 'LocationValues.PVA.Velocity.Z' 0 -0.3 0.3
  $rdp = Copy-N $dp 'ImpactDropsRed'
  Set-Col $rdp 'DrawingValues.ColorAll.Fixed' 200 14 26 255
  Set-R $rdp 'LocationValues.PVA.Velocity.Y' 0.26 0.16 0.36
}

# ---------- A charge ----------
function Edit-ACharge($doc) {
  $g = Get-N $doc 'BladeGlow'
  Set-Col $g 'DrawingValues.ColorAll.Fixed' 255 25 35 255
  Set-R $g 'ScalingValues.SinglePVA.Scale' 1.25
  $s = Get-N $doc 'BladeSmoke'
  Set-Col $s 'DrawingValues.ColorAll.Fixed' 70 6 15 190
  $e = Get-N $doc 'BladeEmber'
  Set-R $e 'CommonValues.Generation.GenerationTime' 0.6
  Set-R $e 'ScalingValues.SinglePVA.Scale' 0.35 0.25 0.45
  Set-Col $e 'DrawingValues.ColorAll.Fixed' 255 50 40 255
  $t = Get-N $doc 'TipStar'
  Set-R $t 'ScalingValues.Easing.Start.X' 16; Set-R $t 'ScalingValues.Easing.Start.Y' 16
  Set-R $t 'ScalingValues.Easing.End.X' 4; Set-R $t 'ScalingValues.Easing.End.Y' 4
  Set-Col $t 'DrawingValues.ColorAll.Fixed' 255 80 70 255
  # red aura rising around the body after the charge completes
  $a = Copy-N $e 'AuraRise'
  Set-V $a 'GenerationLocationValues.Type' 3
  Set-V $a 'GenerationLocationValues.Circle.AxisDirection' 1
  Set-R $a 'GenerationLocationValues.Circle.Radius' 2.6 2.0 3.2
  Set-R $a 'CommonValues.Generation.GenerationTimeOffset' 15
  Set-R $a 'CommonValues.Generation.GenerationTime' 0.8
  Set-R $a 'CommonValues.Life' 34 28 40
  Set-R $a 'LocationValues.PVA.Location.Y' 0.5 0.2 1.5
  Set-R $a 'LocationValues.PVA.Velocity.Y' 0.11 0.08 0.14
  Set-R $a 'LocationValues.PVA.Velocity.X' 0 -0.01 0.01
  Set-R $a 'LocationValues.PVA.Velocity.Z' 0 -0.01 0.01
  Set-R $a 'ScalingValues.SinglePVA.Scale' 1.6 1.2 2.1
  Set-Col $a 'DrawingValues.ColorAll.Fixed' 255 25 35 80
  Set-V $a 'RendererCommonValues.FadeIn.Frame' 6
  Set-V $a 'RendererCommonValues.FadeOut.Frame' 20
}

# ---------- C charge ----------
function Edit-CCharge($doc, $aDoc) {
  $en = Get-N $doc 'Enso'
  Set-V $en 'ScalingValues.Fixed.Scale.X' 13; Set-V $en 'ScalingValues.Fixed.Scale.Y' 13
  $red = Copy-N $en 'EnsoRed' -Before
  Set-V $red 'ScalingValues.Fixed.Scale.X' 14.2; Set-V $red 'ScalingValues.Fixed.Scale.Y' 14.2
  Set-Col $red 'DrawingValues.ColorAll.Fixed' 180 10 24 240
  Set-Col $red 'AdvancedRendererCommonValuesValues.AlphaCutoffParam.EdgeParam.EdgeColor' 200 30 40 255
  $sp = Get-N $doc 'EnsoSplash'
  Set-R $sp 'GenerationLocationValues.Circle.Radius' 5.0 4.6 5.4
  $rsp = Copy-N $sp 'EnsoSplashRed'
  Set-Col $rsp 'DrawingValues.ColorAll.Fixed' 200 14 26 255
  # rising ink-red wisps on the circle (reads from the side)
  $rise = Copy-N $sp 'EnsoRise'
  Set-V $rise 'RendererCommonValues.ColorTexture' 'Texture/CA_Smoke.png'
  Set-V $rise 'CommonValues.MaxGeneration.Infinite' 'True'
  Set-R $rise "CommonValues.Generation.GenerationTime" 0.6
  Set-R $rise 'CommonValues.Life' 40 34 46
  Set-R $rise 'LocationValues.PVA.Velocity.Y' 0.07 0.05 0.09
  Set-R $rise 'LocationValues.PVA.Velocity.X' 0 -0.01 0.01
  Set-R $rise 'LocationValues.PVA.Velocity.Z' 0 -0.01 0.01
  Set-R $rise 'LocationValues.PVA.Acceleration.Y' 0
  Set-R $rise "ScalingValues.SinglePVA.Scale" 2.1 1.6 2.6
  Set-Col $rise 'DrawingValues.ColorAll.Fixed' 120 8 18 150
  Set-V $rise 'RendererCommonValues.FadeInType' 1
  Set-V $rise 'RendererCommonValues.FadeIn.Frame' 6
  Set-V $rise 'RendererCommonValues.FadeOut.Frame' 24
  # red on the blade at full charge (from A)
  $g = Import-N $doc (Get-N $aDoc 'BladeGlow') 'BladeGlow' (Root-Children $doc)
}

$enc = New-Object Text.UTF8Encoding($false)
$a1 = Load-X "$InDir\A_Slash1.xml"; Edit-ASlash $a1 5.5 10.6; Save-X $a1 "$OutDir\A_Slash1.xml"
$a2 = Load-X "$InDir\A_Slash2.xml"; Edit-ASlash $a2 6.0 11.9
$ir = Get-N $a2 'ImpactRed'
Set-R $ir 'ScalingValues.Easing.Start.X' 22; Set-R $ir 'ScalingValues.Easing.Start.Y' 22
Set-R $ir 'ScalingValues.Easing.End.X' 7; Set-R $ir 'ScalingValues.Easing.End.Y' 7
Set-Col $ir 'DrawingValues.ColorAll.Fixed' 255 40 40 255
Save-X $a2 "$OutDir\A_Slash2.xml"
$c1 = Load-X "$InDir\C_Slash1.xml"; Edit-CSlash $c1 5.5 8.8; Save-X $c1 "$OutDir\C_Slash1.xml"
$c2 = Load-X "$InDir\C_Slash2.xml"; Edit-CSlash $c2 6.0 9.6; Edit-CImpact $c2 1.3; Save-X $c2 "$OutDir\C_Slash2.xml"
$ac = Load-X "$InDir\A_Charge.xml"; Edit-ACharge $ac; Save-X $ac "$OutDir\A_Charge.xml"
$cc = Load-X "$InDir\C_Charge.xml"; Edit-CCharge $cc $ac; Save-X $cc "$OutDir\C_Charge.xml"
"edited"
