param([string]$In, [string]$Out)
# 先頭の Material ブロックのうち、同じ名前の2個目以降を消す
$l = [IO.File]::ReadAllLines($In)
$o = New-Object System.Collections.Generic.List[string]
$seen = @{}
$i = 0
while ($i -lt $l.Length) {
    $t = $l[$i]
    if ($t -match '^Material (\S+) \{') {
        $name = $Matches[1]
        if ($seen.ContainsKey($name)) {
            $i++
            while ($l[$i] -ne '}') { $i++ }
            $i++
            continue
        }
        $seen[$name] = 1
    }
    $o.Add($t)
    $i++
}
[IO.File]::WriteAllLines($Out, $o)
"$Out materials=$($seen.Count) : $($seen.Keys -join ', ')"
