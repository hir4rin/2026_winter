# エフェクト全体の時間を k 倍にする(k=2 なら半分の速さ)
# 長さ・出る間隔・遅れ・FCurve のフレーム・フェードは ×k、速度は ÷k、加速度は ÷k²
# 使い方: time_scale.ps1 入力.xml 出力.xml k
param($In, $Out, [double]$K)
. "$PSScriptRoot/../../../tools/EfkXml/Lib.ps1"
$doc = Load-X $In
$count = @{}
function Scale-Num($el, $mul, $tag) {
  $v = 0.0
  if (-not [double]::TryParse($el.InnerText, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$v)) { return }
  $el.InnerText = ($v * $mul).ToString([Globalization.CultureInfo]::InvariantCulture)
  $count[$tag] = 1 + [int]$count[$tag]
}
function Scale-Leaves($el, $mul, $tag) {
  foreach ($e in $el.SelectNodes('.//*[not(*)]')) { Scale-Num $e $mul $tag }
}
foreach ($node in $doc.SelectNodes('//Node')) {
  $cv = $node.SelectSingleNode('CommonValues')
  if ($cv) {
    foreach ($p in 'Life', 'Generation/GenerationTime', 'Generation/GenerationTimeOffset') {
      $e = $cv.SelectSingleNode($p); if ($e) { Scale-Leaves $e $K $p }
    }
  }
  # FCurve のキー(フレームと前後の取っ手の横位置)
  foreach ($key in $node.SelectNodes('.//FCurve/Keys/*/*[starts-with(name(), "Key")]')) {
    foreach ($f in 'Frame', 'LeftX', 'RightX') { $e = $key.SelectSingleNode($f); if ($e) { Scale-Num $e $K 'FCurveFrame' } }
  }
  foreach ($p in './/FadeIn/Frame', './/FadeOut/Frame') { foreach ($e in $node.SelectNodes($p)) { Scale-Num $e $K 'Fade' } }
  foreach ($e in $node.SelectNodes('.//PVA/Velocity')) { Scale-Leaves $e (1.0 / $K) 'Velocity' }
  foreach ($e in $node.SelectNodes('.//PVA/Acceleration')) { Scale-Leaves $e (1.0 / ($K * $K)) 'Acceleration' }
  foreach ($e in $node.SelectNodes('.//UVScroll/Speed')) { Scale-Leaves $e (1.0 / $K) 'UVScroll' }
}
Save-X $doc $Out
($count.GetEnumerator() | Sort-Object Name | ForEach-Object { "$($_.Name)=$($_.Value)" }) -join ' '
