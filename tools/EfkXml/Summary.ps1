param([string]$XmlPath)
# print a compact summary of nodes
[xml]$d = Get-Content -Raw $XmlPath
function Flat($n, $prefix) {
  $out = @()
  foreach ($c in $n.ChildNodes) {
    if ($c.NodeType -ne 'Element') { continue }
    if ($c.Name -eq 'Children' -or $c.Name -eq 'Name') { continue }
    $hasEl = $false
    foreach ($k in $c.ChildNodes) { if ($k.NodeType -eq 'Element') { $hasEl = $true } }
    if ($hasEl) { $out += Flat $c "$prefix$($c.Name)." }
    elseif ($c.InnerText -ne '') { $out += "$prefix$($c.Name)=$($c.InnerText)" }
  }
  return $out
}
function Walk($children, $path) {
  foreach ($node in $children.SelectNodes('Node')) {
    $name = $node.SelectSingleNode('Name').InnerText
    "== $path$name"
    (Flat $node '') -join '  '
    $ch = $node.SelectSingleNode('Children')
    if ($ch) { Walk $ch "$path$name/" }
  }
}
Walk $d.EffekseerProject.Root.SelectSingleNode('Children') ''
