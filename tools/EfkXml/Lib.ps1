# helpers to edit decoded effekseer xml
function Get-N($doc, $name) {
  $n = $doc.SelectSingleNode("//Node[Name='$name']")
  if (-not $n) { throw "node not found: $name" }
  return $n
}
function Set-V($node, $path, $val) {
  $cur = $node
  foreach ($p in $path.Split('.')) {
    $next = $null
    foreach ($c in $cur.ChildNodes) { if ($c.NodeType -eq 'Element' -and $c.Name -eq $p) { $next = $c; break } }
    if (-not $next) {
      if ($cur.HasAttribute('_empty')) { $cur.RemoveAttribute('_empty') }
      if ($cur.ChildNodes.Count -gt 0 -and $cur.FirstChild.NodeType -eq 'Text') { $cur.InnerText = '' }
      $next = $cur.OwnerDocument.CreateElement($p)
      [void]$cur.AppendChild($next)
    }
    $cur = $next
  }
  $cur.InnerText = [string]$val
}
# random range: c / min / max
function Set-R($node, $path, $c, $min = $null, $max = $null) {
  if ($null -eq $min) { $min = $c }
  if ($null -eq $max) { $max = $c }
  Set-V $node "$path.Center" $c
  Set-V $node "$path.Min" $min
  Set-V $node "$path.Max" $max
}
function Set-Col($node, $path, $r, $g, $b, $a = 255) {
  Set-V $node "$path.R" $r; Set-V $node "$path.G" $g; Set-V $node "$path.B" $b; Set-V $node "$path.A" $a
}
function Set-Ring($node, $ox, $oy, $ix, $iy) {
  Set-V $node 'DrawingValues.Ring.Outer_Fixed.Location.X' $ox
  Set-V $node 'DrawingValues.Ring.Outer_Fixed.Location.Y' $oy
  Set-V $node 'DrawingValues.Ring.Inner_Fixed.Location.X' $ix
  Set-V $node 'DrawingValues.Ring.Inner_Fixed.Location.Y' $iy
}
function Set-RingCol($node, $o, $c, $i) {
  Set-Col $node 'DrawingValues.Ring.OuterColor_Fixed' $o[0] $o[1] $o[2] $o[3]
  Set-Col $node 'DrawingValues.Ring.CenterColor_Fixed' $c[0] $c[1] $c[2] $c[3]
  Set-Col $node 'DrawingValues.Ring.InnerColor_Fixed' $i[0] $i[1] $i[2] $i[3]
}
# copy a node (with children); -Before puts it before the original
function Copy-N($src, $newName, [switch]$Before, $parentChildren = $null, $refNode = $null) {
  $c = $src.CloneNode($true)
  foreach ($k in $c.ChildNodes) { if ($k.Name -eq 'Name') { $k.InnerText = $newName } }
  if ($parentChildren) {
    if ($refNode) { [void]$parentChildren.InsertAfter($c, $refNode) } else { [void]$parentChildren.AppendChild($c) }
  }
  elseif ($Before) { [void]$src.ParentNode.InsertBefore($c, $src) }
  else { [void]$src.ParentNode.InsertAfter($c, $src) }
  return $c
}
# import a node from another document
function Import-N($doc, $srcNode, $newName, $parentChildren) {
  $c = $doc.ImportNode($srcNode, $true)
  foreach ($k in $c.ChildNodes) { if ($k.Name -eq 'Name') { $k.InnerText = $newName } }
  if ($parentChildren.HasAttribute('_empty')) { $parentChildren.RemoveAttribute('_empty') }
  [void]$parentChildren.AppendChild($c)
  return $c
}
function Root-Children($doc) { return $doc.EffekseerProject.Root.SelectSingleNode('Children') }
function Load-X($path) { $d = New-Object Xml.XmlDocument; $d.PreserveWhitespace = $false; $d.Load($path); return $d }
function Save-X($doc, $path) {
  $ws = New-Object Xml.XmlWriterSettings; $ws.Indent = $true; $ws.Encoding = New-Object Text.UTF8Encoding($false)
  $w = [Xml.XmlWriter]::Create($path, $ws); $doc.Save($w); $w.Close()
}
