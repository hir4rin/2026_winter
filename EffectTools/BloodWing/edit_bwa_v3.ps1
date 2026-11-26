# 羽の発動演出(BloodWingActivate)の吸い込みを、必殺技の溜め(UltBloodAbsorb v3)の液体に置き換える
# 使い方: edit_bwa_v3.ps1 <羽のxml> <溜めのxml> <出力xml>
param([string]$In, [string]$Uba, [string]$Out)
. "$PSScriptRoot\..\..\tools\EfkXml\Lib.ps1"

$d = Load-X $In
$u = Load-X $Uba
$k = 20.0 / 14.0  # 溜めは20倍、羽は14倍で再生する
$floorY = -9.75

function Scale-All($n, $k) {
  foreach ($grp in 'ScalingValues/Easing/Start', 'ScalingValues/Easing/End', 'LocationValues/Easing/Start', 'LocationValues/Easing/End', 'LocationValues/Easing/Middle', 'LocationValues/PVA/Location', 'LocationValues/PVA/Velocity') {
    $g = $n.SelectSingleNode($grp)
    if (-not $g) { continue }
    foreach ($e in $g.SelectNodes('.//*[not(*)]')) { if ($e.InnerText -match '^-?[0-9.Ee+-]+$') { $e.InnerText = [string]([double]$e.InnerText * $k) } }
  }
}
function Scale-Ring($n, $k) {
  foreach ($p in 'DrawingValues/Ring/Outer_Easing', 'DrawingValues/Ring/Inner_Easing') {
    $g = $n.SelectSingleNode($p)
    foreach ($e in $g.SelectNodes('Start//*[not(*)] | End//*[not(*)]')) { if ($e.InnerText -match '^-?[0-9.Ee+-]+$') { $e.InnerText = [string]([double]$e.InnerText * $k) } }
  }
}

$vortex = Get-N $d 'Vortex'
$vk = $vortex.SelectSingleNode('Children')

# 三日月の帯と血の塊は消す
foreach ($name in 'Swirl', 'BloodClot', 'FloorClot') {
  $n = $vortex.SelectSingleNode("Children/Node[Name='$name']")
  if ($n) { [void]$vk.RemoveChild($n) }
}

# 溜めの液体を持ってくる
$pool = Import-N $d (Get-N $u 'Pool') 'Pool' $vk
Scale-All $pool $k
Set-R $pool 'LocationValues.Easing.Start.Y' $floorY
Set-R $pool 'LocationValues.Easing.End.Y' $floorY

$peel = Import-N $d (Get-N $u 'Peel') 'Peel' $vk
Scale-All $peel $k
Set-R $peel 'LocationValues.Easing.Start.Y' ($floorY + 0.15)
Scale-Ring (Get-N $d 'PeelSheet') $k

$air = Import-N $d (Get-N $u 'AirSheet') 'AirSheet' $vk
Scale-All $air $k
Scale-Ring (Get-N $d 'AirBand') $k

$shard = Import-N $d (Get-N $u 'Shard') 'Strand' $vk
Scale-All $shard $k

# もやは暗く薄く(光らせない)
$mist = Get-N $d 'InMist'
Set-Col $mist 'DrawingValues.ColorAll.Fixed' 90 8 16 80
$cc = Get-N $d 'ChargeCore'
Set-Col $cc 'DrawingValues.ColorAll.Fixed' 110 12 22 255
# 光るのは集まりきる直前から
Set-R (Get-N $d 'ChargePulse') 'CommonValues.Generation.GenerationTimeOffset' 34

# はじける滴(溜めの BurstSpike)
$sp = Import-N $d (Get-N $u 'BurstSpike') 'BurstSpike' (Root-Children $d)
Scale-All $sp $k

Save-X $d $Out
Write-Output "ok"
