# UltBloodAbsorb を「床の血を引きはがして吸い込み、縮んで光ってはじける」形に書き換える(B 大胆な案)
# 使い方: edit_absorb_v2.ps1 <今のxml> <羽の発動演出のxml> <出力xml>
param([string]$In, [string]$Bwa, [string]$Out)
. "$PSScriptRoot\..\..\tools\EfkXml\Lib.ps1"

$d = Load-X $In
$w = Load-X $Bwa

function Del-V($node, $path) {
  $cur = $node
  foreach ($p in $path.Split('.')) {
    $next = $null
    foreach ($c in $cur.ChildNodes) { if ($c.NodeType -eq 'Element' -and $c.Name -eq $p) { $next = $c; break } }
    if (-not $next) { return }
    $cur = $next
  }
  [void]$cur.ParentNode.RemoveChild($cur)
}
function Kids($node) { return $node.SelectSingleNode('Children') }
function Set-E3($n, $base, $axis, $c, $min = $null, $max = $null) { Set-R $n "$base.$axis" $c $min $max }
function Scale-Num($n, $path, $k) {
  $e = $n.SelectSingleNode($path.Replace('.', '/'))
  if ($e -and $e.InnerText -ne '') { $e.InnerText = [string]([double]$e.InnerText * $k) }
}
# 羽(14倍)→ 溜め(20倍)の大きさ合わせ
function Scale-All($n, $k) {
  foreach ($grp in 'ScalingValues/Easing/Start', 'ScalingValues/Easing/End', 'LocationValues/Easing/Start', 'LocationValues/Easing/End', 'LocationValues/Easing/Middle', 'LocationValues/PVA/Location', 'LocationValues/PVA/Velocity') {
    $g = $n.SelectSingleNode($grp)
    if (-not $g) { continue }
    foreach ($e in $g.SelectNodes('.//*[not(*)]')) { if ($e.InnerText -match '^-?[0-9.Ee+-]+$') { $e.InnerText = [string]([double]$e.InnerText * $k) } }
  }
}

$vortex = Get-N $d 'Vortex'
$vk = Kids $vortex
$root = Root-Children $d

# ---- 空中の帯: 暗い液体の面にする(光らせない) ----
$air = Get-N $d 'Swirl'; ($air.SelectSingleNode('Name')).InnerText = 'AirSheet'
Set-V $air 'CommonValues.MaxGeneration.Value' 14
Set-R $air 'CommonValues.Generation.GenerationTime' 1.2
Set-R $air 'CommonValues.Generation.GenerationTimeOffset' 6
$glow = Get-N $d 'SwirlGlow'; [void]$glow.ParentNode.RemoveChild($glow)
$band = Get-N $d 'SwirlBand'; ($band.SelectSingleNode('Name')).InnerText = 'AirBand'
Set-V $band 'RendererCommonValues.ColorTexture' 'Texture/UBA_InkSheet_a.png'
Set-R $band 'DrawingValues.Ring.Outer_Easing.Start.X' 10
Set-R $band 'DrawingValues.Ring.Inner_Easing.Start.X' 5.2
Set-R $band 'DrawingValues.Ring.Outer_Easing.End.X' 1.2
Set-R $band 'DrawingValues.Ring.Inner_Easing.End.X' 0.3
Set-RingCol $band @(30, 0, 4, 220) @(175, 12, 30, 255) @(30, 0, 4, 220)

# ---- 床から引きはがす帯: 床に寝た状態から起き上がりながら縮む ----
$peel = Get-N $d 'FloorSwirl'; ($peel.SelectSingleNode('Name')).InnerText = 'Peel'
Set-V $peel 'CommonValues.MaxGeneration.Value' 22
Set-R $peel 'CommonValues.Generation.GenerationTime' 0.75
Set-R $peel 'CommonValues.Generation.GenerationTimeOffset' 1
Set-V $peel 'LocationValues.Type' 2
Set-R $peel 'LocationValues.Easing.Start.Y' -4.6
Set-R $peel 'LocationValues.Easing.End.Y' 0 -0.6 0.8
Set-V $peel 'LocationValues.Easing.Type' 21
Set-V $peel 'RotationValues.Type' 2
Set-R $peel 'RotationValues.Easing.Start.X' 90 86 94
Set-R $peel 'RotationValues.Easing.Start.Y' 180 0 360
Set-R $peel 'RotationValues.Easing.End.X' 0 -65 65
Set-R $peel 'RotationValues.Easing.End.Y' 300 120 480
Set-R $peel 'RotationValues.Easing.End.Z' 0 -35 35
Set-V $peel 'RotationValues.Easing.Type' 21
Set-V $peel 'RotationValues.Easing.IsRandomGroupEnabled' 'True'
$pb = Get-N $d 'FloorBand'; ($pb.SelectSingleNode('Name')).InnerText = 'PeelSheet'
Set-V $pb 'RendererCommonValues.ColorTexture' 'Texture/UBA_InkSheet2_a.png'
Set-R $pb 'DrawingValues.Ring.Outer_Easing.Start.X' 13
Set-R $pb 'DrawingValues.Ring.Inner_Easing.Start.X' 9.6
Set-R $pb 'DrawingValues.Ring.Outer_Easing.End.X' 0.9
Set-R $pb 'DrawingValues.Ring.Inner_Easing.End.X' 0.2
Set-RingCol $pb @(25, 0, 3, 230) @(160, 10, 26, 255) @(25, 0, 3, 230)

# ---- 床の血だまり: 広がってから中心へ吸い寄せられて縮む ----
$coreBlob = Get-N $d 'CoreBlob'
$pool = Import-N $d $coreBlob 'Pool' $vk
[void]$vk.InsertBefore($pool, $vk.FirstChild)
Set-V $pool 'CommonValues.MaxGeneration.Value' 8
Set-R $pool 'CommonValues.Life' 40 36 44
Set-R $pool 'CommonValues.Generation.GenerationTime' 0.6
Set-R $pool 'CommonValues.Generation.GenerationTimeOffset' 0
Set-V $pool 'LocationValues.Type' 2
Set-R $pool 'LocationValues.Easing.Start.X' 0 -10 10
Set-R $pool 'LocationValues.Easing.Start.Y' -4.7
Set-R $pool 'LocationValues.Easing.Start.Z' 0 -10 10
Set-R $pool 'LocationValues.Easing.End.Y' -4.7
Set-V $pool 'LocationValues.Easing.Type' 20
Set-R $pool 'RotationValues.PVA.Rotation.X' 90
Set-R $pool 'ScalingValues.Easing.Start.X' 4.5 3.5 5.5
Set-R $pool 'ScalingValues.Easing.Start.Y' 4.5 3.5 5.5
Set-R $pool 'ScalingValues.Easing.End.X' 0.4
Set-R $pool 'ScalingValues.Easing.End.Y' 0.4
Set-V $pool 'ScalingValues.Easing.Type' 20
Set-V $pool 'RendererCommonValues.ColorTexture' 'Texture/UBA_Pool_a.png'
Set-V $pool 'RendererCommonValues.UV' 0
Set-V $pool 'RendererCommonValues.FadeIn.Frame' 4
Set-V $pool 'DrawingValues.Sprite.Billboard' 2
Set-Col $pool 'DrawingValues.ColorAll.Fixed' 120 8 18

# ---- 暗いもや ----
$mist = Import-N $d (Get-N $w 'InMist') 'InkMist' $vk
Scale-All $mist 0.7
Set-V $mist 'CommonValues.MaxGeneration.Value' 14
Set-Col $mist 'DrawingValues.ColorAll.Fixed' 70 6 14 170

# ---- 破片を増やして暗く ----
$shard = Get-N $d 'Shard'
Set-V $shard 'CommonValues.MaxGeneration.Value' 40
Set-R $shard 'CommonValues.Generation.GenerationTime' 0.75 0.5 1.0
Set-Col $shard 'DrawingValues.ColorAll.Fixed' 110 20 28

# ---- 付け根の塊: 大きくなる → 一度縮む ----
Set-R $coreBlob 'CommonValues.Life' 34
Set-V $coreBlob 'RendererCommonValues.FadeOut.Frame' 2
Set-R $coreBlob 'ScalingValues.Easing.End.X' 2.8 2.6 3.0
Set-R $coreBlob 'ScalingValues.Easing.End.Y' 2.8 2.6 3.0
$sq = Copy-N $coreBlob 'Squeeze'
Set-V $sq 'CommonValues.MaxGeneration.Value' 1
Set-R $sq 'CommonValues.Life' 4
Set-R $sq 'CommonValues.Generation.GenerationTimeOffset' 39
Set-V $sq 'RendererCommonValues.FadeInType' 0
Set-V $sq 'RendererCommonValues.FadeOut.Frame' 1
Set-R $sq 'ScalingValues.Easing.Start.X' 2.8
Set-R $sq 'ScalingValues.Easing.Start.Y' 2.8
Set-R $sq 'ScalingValues.Easing.End.X' 0.5
Set-R $sq 'ScalingValues.Easing.End.Y' 0.5
Set-V $sq 'ScalingValues.Easing.Type' 20

# ---- 光るのは集まりきってから ----
$core = Get-N $d 'Core'
Set-R $core 'CommonValues.Life' 24
Set-R $core 'CommonValues.Generation.GenerationTimeOffset' 41
Set-R $core 'ScalingValues.SingleEasing.Start' 6
Set-R $core 'ScalingValues.SingleEasing.End' 2.2
Set-V $core 'ScalingValues.SingleEasing.StartSpeed' 20
Set-R $core 'DrawingValues.ColorAll.Easing.Start.R' 255
Set-R $core 'DrawingValues.ColorAll.Easing.Start.G' 70
Set-R $core 'DrawingValues.ColorAll.Easing.Start.B' 90
Set-R $core 'DrawingValues.ColorAll.Easing.Start.A' 230
Set-R $core 'DrawingValues.ColorAll.Easing.End.R' 200
Set-R $core 'DrawingValues.ColorAll.Easing.End.G' 10
Set-R $core 'DrawingValues.ColorAll.Easing.End.B' 30
Set-R $core 'DrawingValues.ColorAll.Easing.End.A' 90

# ---- はじける(羽の発動演出から持ってきて 0.7 倍) ----
foreach ($pair in @(@('FlashGlow', 'BurstGlow', 42), @('FlashH', 'BurstFlashH', 42), @('FlashV', 'BurstFlashV', 42), @('BurstMist', 'BurstMist', 42), @('BurstChunk', 'BurstChunk', 43), @('ChargePulse', 'AfterPulse', 46))) {
  $n = Import-N $d (Get-N $w $pair[0]) $pair[1] $root
  Scale-All $n 0.7
  Set-R $n 'CommonValues.Generation.GenerationTimeOffset' $pair[2]
}
$bc = Get-N $d 'BurstChunk'
Set-R $bc 'LocationValues.Easing.End.X' 0 -9 9
Set-R $bc 'LocationValues.Easing.End.Y' 3 -2 9
Set-R $bc 'LocationValues.Easing.End.Z' 0 -9 9
$bm = Get-N $d 'BurstMist'
Set-R $bm 'LocationValues.PVA.Location.X' 0 -1.4 1.4
Set-R $bm 'LocationValues.PVA.Velocity.X' 0 -0.12 0.12
Set-Col $bm 'DrawingValues.ColorAll.Fixed' 120 20 28 150
$ap = Get-N $d 'AfterPulse'
Set-V $ap 'CommonValues.MaxGeneration.Value' 4
Set-R $ap 'CommonValues.Generation.GenerationTime' 4

# とげ
$sp = Copy-N $bc 'BurstSpike'
Set-V $sp 'CommonValues.MaxGeneration.Value' 26
Set-R $sp 'CommonValues.Life' 12 9 15
Set-R $sp 'CommonValues.Generation.GenerationTimeOffset' 42
Set-R $sp 'LocationValues.Easing.End.X' 0 -10 10
Set-R $sp 'LocationValues.Easing.End.Y' 3 -3 10
Set-R $sp 'LocationValues.Easing.End.Z' 0 -10 10
Set-R $sp 'ScalingValues.Easing.Start.X' 0.8 0.6 1.0
Set-R $sp 'ScalingValues.Easing.Start.Y' 4 3 5
Set-R $sp 'ScalingValues.Easing.End.X' 0.3
Set-R $sp 'ScalingValues.Easing.End.Y' 1.5
Set-V $sp 'RendererCommonValues.ColorTexture' 'Texture/UBA_Spike_a.png'
Set-V $sp 'RendererCommonValues.UV' 0
Set-V $sp 'RendererCommonValues.FadeOut.Frame' 6
Set-Col $sp 'DrawingValues.ColorAll.Fixed' 190 12 30

# 床に広がる赤い光
$ff = Copy-N $core 'FloorFlash'
Set-R $ff 'CommonValues.Life' 14
Set-R $ff 'CommonValues.Generation.GenerationTimeOffset' 42
Set-V $ff 'LocationValues.Fixed.Location.Y' -4.7
Set-V $ff 'RotationValues.Fixed.Rotation.X' 90
Set-R $ff 'ScalingValues.SingleEasing.Start' 2
Set-R $ff 'ScalingValues.SingleEasing.End' 16
Set-V $ff 'RendererCommonValues.ColorTexture' 'Texture/ImpactGlow.png'
Set-V $ff 'DrawingValues.Sprite.Billboard' 2

Save-X $d $Out
Write-Output "ok"
