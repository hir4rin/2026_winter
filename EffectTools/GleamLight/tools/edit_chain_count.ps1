# 連鎖(ChainLeg)の数と間隔を変える
# 使い方: edit_chain_count.ps1 入力.xml 出力.xml 数 間隔(フレーム)
param($In, $Out, [int]$Count, [double]$Interval)
. "$PSScriptRoot/../../../tools/EfkXml/Lib.ps1"
$doc = Load-X $In
$n = Get-N $doc 'ChainLeg'
Set-V $n 'CommonValues.MaxGeneration.Value' $Count
Set-R $n 'CommonValues.Generation.GenerationTime' $Interval
Save-X $doc $Out
Write-Output "ChainLeg: $Count 回 / $Interval F おき"
