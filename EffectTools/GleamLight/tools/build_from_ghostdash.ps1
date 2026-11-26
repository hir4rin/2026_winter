# 強スキルの分身(GhostDash3D)をもとに弱1・弱2を作る
# 回し役(Spinner)の向きをキーで決め、分身は強スキルと同じく6Fおきに出て40F残る
# 使い方: build_from_ghostdash.ps1 GhostDash3D.xml 出力.xml 段(1〜2)
param($In, $Out, [int]$Step)
. "$PSScriptRoot/../../../tools/EfkXml/Lib.ps1"
$doc = Load-X $In
$spinner = Get-N $doc 'Spinner'
$ghost = Get-N $doc 'Ghost'
$keys = $spinner.SelectSingleNode('RotationValues/RotationFCurve/FCurve/Keys')

function Set-Keys($axis, $list) {
  $node = $keys.SelectSingleNode($axis)
  foreach ($k in @($node.SelectNodes('*[starts-with(name(), "Key")]'))) { [void]$node.RemoveChild($k) }
  $i = 0
  foreach ($kv in $list) {
    $k = $doc.CreateElement("Key$i"); $i++
    foreach ($pair in @(@('Frame', $kv[0]), @('Value', $kv[1]), @('LeftX', $kv[0]), @('LeftY', $kv[1]), @('RightX', $kv[0]), @('RightY', $kv[1]), @('InterpolationType', 1))) {
      $el = $doc.CreateElement($pair[0]); $el.InnerText = [string]$pair[1]; [void]$k.AppendChild($el)
    }
    [void]$node.AppendChild($k)
  }
}

if ($Step -eq 1) {
  # 1段目: 強スキルと同じく敵の周りを回りながら斬る。回る向きを逆にして1体ごとに-150°(4体)
  # 地上の技なので傾けない(水平)。腰より少し下(1.5)を通る
  Set-Keys 'X' @(@(0, 0), @(28, 0))
  Set-Keys 'Y' @(@(0, 0), @(120, -3000))
  Set-V $spinner 'LocationValues.Fixed.Location.Y' 1.5
  Set-V $ghost 'CommonValues.MaxGeneration.Value' 4
}
else {
  # 2段目: 空中スキルと同じ6体。1体ごとに150°回る(6方向がばらける)。上から下へ斬り下ろす向きだけにする(-10〜-20°)
  # 高さ1.5(2.5だと高く見えた)。抜けた先は床の少し上
  Set-Keys 'X' @(@(0, -20), @(5, -20), @(6, -10), @(11, -10), @(12, -20), @(17, -20), @(18, -10), @(23, -10), @(24, -20), @(29, -20), @(30, -10), @(36, -10))
  Set-Keys 'Y' @(@(0, 0), @(120, 3000))
  Set-V $spinner 'LocationValues.Fixed.Location.Y' 1.5
  Set-V $ghost 'CommonValues.MaxGeneration.Value' 6
}
Save-X $doc $Out
