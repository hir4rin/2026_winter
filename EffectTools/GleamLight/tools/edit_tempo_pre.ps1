# time_scale.ps1 の前に、強スキルのテンポに寄せる下ごしらえ(フレームは引き延ばす前の値)
# 使い方: edit_tempo_pre.ps1 入力.xml 出力.xml 段(1〜2)
param($In, $Out, [int]$Step)
. "$PSScriptRoot/../../../tools/EfkXml/Lib.ps1"
$doc = Load-X $In
# 入りの分身: 同じ所に5体重なっていたので減らす(3Fおき → 引き延ばすと強スキルと同じ6Fおき)
foreach ($pivot in $doc.SelectNodes("//Node[starts-with(Name, 'DashPivot')]")) {
  $g = $pivot.SelectSingleNode(".//Node[Name='DashGhost']")
  Set-V $g 'CommonValues.MaxGeneration.Value' $(if ($Step -eq 1) { 2 } else { 3 })
  Set-R $g 'CommonValues.Generation.GenerationTime' 3
}
if ($Step -eq 2) {
  # 連鎖を弱1と同じ3Fおきに(上下それぞれ6Fおき)
  Set-R (Get-N $doc 'ChainLegUp') 'CommonValues.Generation.GenerationTime' 6
  Set-R (Get-N $doc 'ChainLegUp') 'CommonValues.Generation.GenerationTimeOffset' 9
  Set-R (Get-N $doc 'ChainLegDown') 'CommonValues.Generation.GenerationTime' 6
  Set-R (Get-N $doc 'ChainLegDown') 'CommonValues.Generation.GenerationTimeOffset' 12
  # 連鎖が終わってから残像と筋
  foreach ($n in 'WhiteSilhouette', 'WhiteSilhouetteB', 'BurstRays') { Set-R (Get-N $doc $n) 'CommonValues.Generation.GenerationTimeOffset' 25 }
}
Save-X $doc $Out
