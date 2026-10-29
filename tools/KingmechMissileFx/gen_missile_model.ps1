# ミサイルの .efkmodel(先端が y=0、尾が +Y。落ちるときはそのまま下を向く)
param([string]$Out = "$PSScriptRoot\..\..\2026_winter\data\Effect\Kingmech\Model\Missile.efkmodel")

$L = @(0.6, 0.35, 0.72); $ln = [Math]::Sqrt($L[0]*$L[0]+$L[1]*$L[1]+$L[2]*$L[2]); $L = $L | ForEach-Object { $_ / $ln }
function Shade($nx, $ny, $nz) { 0.45 + 0.55 * [Math]::Max(0.0, $nx*$L[0] + $ny*$L[1] + $nz*$L[2]) }
function BodyColor($y) {
  if ($y -lt 1.0) { return @(200, 45, 30) }
  if ($y -lt 1.25) { return @(230, 190, 40) }
  if ($y -gt 4.45) { return @(55, 55, 60) }
  return @(165, 170, 175)
}

$verts = New-Object System.Collections.Generic.List[object]
$faces = New-Object System.Collections.Generic.List[int[]]
function AddVert($p, $n, $uv, $c) {
  $s = Shade $n[0] $n[1] $n[2]
  $verts.Add(@($p[0],$p[1],$p[2],$n[0],$n[1],$n[2],$uv[0],$uv[1],
    [byte][Math]::Min(255,$c[0]*$s),[byte][Math]::Min(255,$c[1]*$s),[byte][Math]::Min(255,$c[2]*$s)))
  return $verts.Count - 1
}

# 胴体(回転体)
$prof = @(@(0.0,0.0),@(0.12,0.1),@(0.3,0.19),@(0.6,0.28),@(1.0,0.33),@(1.25,0.33),@(4.3,0.33),@(4.48,0.27),@(4.5,0.22),@(4.8,0.27))
$seg = 24
$base = $verts.Count
for ($j = 0; $j -lt $prof.Count; $j++) {
  $y = $prof[$j][0]; $r = $prof[$j][1]
  $j0 = [Math]::Max(0, $j-1); $j1 = [Math]::Min($prof.Count-1, $j+1)
  $dy = $prof[$j1][0] - $prof[$j0][0]; $dr = $prof[$j1][1] - $prof[$j0][1]
  $len = [Math]::Sqrt($dy*$dy + $dr*$dr); $nr = $dy/$len; $ny = -$dr/$len
  for ($i = 0; $i -le $seg; $i++) {
    $a = 2*[Math]::PI*$i/$seg; $c = [Math]::Cos($a); $s = [Math]::Sin($a)
    [void](AddVert @(($r*$c),$y,($r*$s)) @(($nr*$c),$ny,($nr*$s)) @(($i/$seg),($y/4.8)) (BodyColor $y))
  }
}
for ($j = 0; $j -lt $prof.Count-1; $j++) {
  for ($i = 0; $i -lt $seg; $i++) {
    $a = $base + $j*($seg+1) + $i; $b = $a+1; $c = $a+$seg+2; $d = $a+$seg+1
    $faces.Add(@($a,$b,$c)); $faces.Add(@($a,$c,$d))
  }
}

# 尾翼4枚(両面)
for ($k = 0; $k -lt 4; $k++) {
  $ang = [Math]::PI/4 + $k*[Math]::PI/2; $dx = [Math]::Cos($ang); $dz = [Math]::Sin($ang)
  $tx = -$dz; $tz = $dx
  $pts = @(@(0.3,3.3),@(0.3,4.5),@(0.9,4.55),@(0.9,4.0))
  foreach ($side in 1,-1) {
    $n = @(($tx*$side),0,($tz*$side)); $ids = @()
    foreach ($p in $pts) { $ids += AddVert @(($dx*$p[0]),$p[1],($dz*$p[0])) $n @(0,0) @(120,125,130) }
    if ($side -eq 1) { $faces.Add(@($ids[0],$ids[1],$ids[2])); $faces.Add(@($ids[0],$ids[2],$ids[3])) }
    else { $faces.Add(@($ids[0],$ids[2],$ids[1])); $faces.Add(@($ids[0],$ids[3],$ids[2])) }
  }
}

$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter $ms
$bw.Write([int]5); $bw.Write([single]1.0); $bw.Write([int]1); $bw.Write([int]1)
$bw.Write([int]$verts.Count)
foreach ($v in $verts) {
  $bw.Write([single]$v[0]); $bw.Write([single]$v[1]); $bw.Write([single]$v[2])
  $bw.Write([single]$v[3]); $bw.Write([single]$v[4]); $bw.Write([single]$v[5])
  $bw.Write([single]0); $bw.Write([single]1); $bw.Write([single]0)
  $bw.Write([single](-$v[5])); $bw.Write([single]0); $bw.Write([single]$v[3])
  $bw.Write([single]$v[6]); $bw.Write([single]$v[7])
  $bw.Write([byte]$v[8]); $bw.Write([byte]$v[9]); $bw.Write([byte]$v[10]); $bw.Write([byte]255)
}
$bw.Write([int]$faces.Count)
foreach ($f in $faces) { $bw.Write([int]$f[0]); $bw.Write([int]$f[1]); $bw.Write([int]$f[2]) }
$bw.Flush()
[System.IO.File]::WriteAllBytes([System.IO.Path]::GetFullPath($Out), $ms.ToArray())
"wrote $Out ($($verts.Count) verts, $($faces.Count) faces)"
