# 分身が前へ進む量をなくす(進む距離は ComboChain01.csv の移動で出す)
# 使い方: edit_follow.ps1 入力.xml 出力.xml 段(1〜3)
param($In, $Out, [int]$Step)
. "$PSScriptRoot/../../../tools/EfkXml/Lib.ps1"
$doc = Load-X $In

# 前の敵の位置 Z15 → Z5、抜けた先 Z30 → 弱1は0(プレイヤーの所)・弱2は10(X字が敵の位置で交わる)
$zMap = @{ '15' = '5'; '30' = $(if ($Step -eq 2) { '10' } else { '0' }); '-13' = '-10'; '13' = '10'; '-14' = '-10'; '14' = '10' }
# 弱2の左右の幅 ±11 → ±6
$xMap = @{ '11' = '6'; '-11' = '-6' }

function Edit-Value($el, $map, $log) {
  $v = $el.InnerText
  if ($map.ContainsKey($v)) { $log.Add("$($el.ParentNode.Name)/$($el.Name): $v -> $($map[$v])"); $el.InnerText = $map[$v] }
}
foreach ($node in $doc.SelectNodes('//Node')) {
  $name = $node.SelectSingleNode('Name').InnerText
  $log = New-Object Collections.Generic.List[string]
  $loc = $node.SelectSingleNode('LocationValues')
  if (-not $loc) { continue }
  foreach ($z in $loc.SelectNodes('Fixed/Location/Z')) { Edit-Value $z $zMap $log }
  # 弱3の破片(PVA で Z15±2 に出る)
  foreach ($z in $loc.SelectNodes('PVA/Location/Z/*')) { Edit-Value $z @{ '15' = '5'; '13' = '3'; '17' = '7' } $log }
  # 分身のモデルのずらし(12 / -3 / -5)は表にないので変わらない
  foreach ($k in $loc.SelectNodes('LocationFCurve/FCurve/Keys/Z/*')) {
    foreach ($e in $k.SelectNodes('Value|LeftY|RightY')) { Edit-Value $e $zMap $log }
  }
  if ($Step -eq 2) {
    foreach ($x in $loc.SelectNodes('Fixed/Location/X')) { Edit-Value $x $xMap $log }
    foreach ($k in $loc.SelectNodes('LocationFCurve/FCurve/Keys/X/*')) {
      foreach ($e in $k.SelectNodes('Value|LeftY|RightY')) { Edit-Value $e $xMap $log }
    }
  }
  if ($log.Count -gt 0) { Write-Output "== $name"; $log | ForEach-Object { Write-Output "  $_" } }
}
Save-X $doc $Out
