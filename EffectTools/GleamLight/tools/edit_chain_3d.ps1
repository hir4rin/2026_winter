# 連鎖(ChainLeg)を立体的にする(案A)
# 上向きの線(ChainLegUp)と下向きの線(ChainLegDown)を交互に出し、線を少し曲げる
# 使い方: edit_chain_3d.ps1 入力.xml 出力.xml 数 間隔(フレーム)
param($In, $Out, [int]$Count, [double]$Interval)
. "$PSScriptRoot/../../../tools/EfkXml/Lib.ps1"
$doc = Load-X $In
$up = Get-N $doc 'ChainLeg'
$up.SelectSingleNode('Name').InnerText = 'ChainLegUp'
$down = Copy-N $up 'ChainLegDown'

$pitchMin = 25; $pitchMax = 40   # 傾き(度)。大きいと線の端が床に潜る
$half = 8                        # 線の半分の長さ(前は10)
$bend = 2.5                      # 線の真ん中のふくらみ

function Set-Leg($leg, $sign, $count, $offset) {
  Set-V $leg 'CommonValues.MaxGeneration.Value' $count
  Set-R $leg 'CommonValues.Generation.GenerationTime' ($Interval * 2)
  Set-R $leg 'CommonValues.Generation.GenerationTimeOffset' $offset
  $c = ($pitchMin + $pitchMax) / 2 * $sign
  $a = $pitchMin * $sign; $b = $pitchMax * $sign
  Set-R $leg 'RotationValues.PVA.Rotation.X' $c ([Math]::Min($a, $b)) ([Math]::Max($a, $b))

  $mover = $leg.SelectSingleNode(".//Node[Name='LegMover']")
  $keys = $mover.SelectSingleNode('LocationValues/LocationFCurve/FCurve/Keys')
  foreach ($k in @(@('Key0', -$half), @('Key1', $half))) {
    $e = $keys.SelectSingleNode("Z/$($k[0])")
    foreach ($f in 'Value', 'LeftY', 'RightY') { $e.SelectSingleNode($f).InnerText = $k[1] }
  }
  # Y を 0 → ふくらみ → 0(Z と同じ4Fで、真ん中でいちばん外へ)
  $old = $keys.SelectSingleNode('Y'); if ($old) { [void]$keys.RemoveChild($old) }
  $y = $doc.CreateElement('Y')
  $i = 0
  foreach ($kv in @(@(0, 0), @(2, ($bend * $sign)), @(4, 0))) {
    $k = $doc.CreateElement("Key$i"); $i++
    foreach ($pair in @(@('Frame', $kv[0]), @('Value', $kv[1]), @('LeftX', $kv[0]), @('LeftY', $kv[1]), @('RightX', $kv[0]), @('RightY', $kv[1]), @('InterpolationType', 1))) {
      $el = $doc.CreateElement($pair[0]); $el.InnerText = [string]$pair[1]; [void]$k.AppendChild($el)
    }
    [void]$y.AppendChild($k)
  }
  [void]$keys.InsertBefore($y, $keys.SelectSingleNode('Z'))
}
$upCount = [Math]::Ceiling($Count / 2); $downCount = [Math]::Floor($Count / 2)
Set-Leg $up 1 $upCount 9
Set-Leg $down -1 $downCount (9 + $Interval)
Save-X $doc $Out
Write-Output "Up $upCount / Down $downCount, 間隔 $($Interval * 2)F"
