# 弱3の着地の余韻を伸ばす(残像・地面の輪・破片・放射の筋を長く残す)
# 使い方: edit_afterglow.ps1 入力.xml 出力.xml 書き出し用.xml(DebugEnemyを消したもの)
param($In, $Out, $OutExport)
. "$PSScriptRoot/../../../tools/EfkXml/Lib.ps1"
$doc = Load-X $In

# 着地の白い残像: 30F → 60F(溶けていく速さも半分)
$sil = Get-N $doc 'WhiteSilhouette'
Set-R $sil 'CommonValues.Life' 60

# 地面の輪: 18F → 30F でゆっくり広がる
$ring = Get-N $doc 'ImpactRing'
Set-R $ring 'CommonValues.Life' 30
Set-V $ring 'RendererCommonValues.FadeOut.Frame' 22

# 破片: 20〜30F → 35〜50F。長く浮くように初速と重力を弱める(床に落ちるのは約53F)
$sh = Get-N $doc 'ImpactShards'
Set-R $sh 'CommonValues.Life' 42 35 50
Set-R $sh 'LocationValues.PVA.Velocity.Y' 0.5 0.2 0.8
Set-R $sh 'LocationValues.PVA.Acceleration.Y' -0.02
Set-V $sh 'RendererCommonValues.FadeOut.Frame' 18

# 放射の筋: 10〜15F → 16〜22F
$ray = Get-N $doc 'BurstRays'
Set-R $ray 'CommonValues.Life' 19 16 22
Set-V $ray 'RendererCommonValues.FadeOut.Frame' 14

Save-X $doc $Out

# 書き出し用: 確認用の敵を消す
$dbg = $doc.SelectSingleNode("//Node[Name='DebugEnemy']")
if ($dbg) { [void]$dbg.ParentNode.RemoveChild($dbg) }
Save-X $doc $OutExport
