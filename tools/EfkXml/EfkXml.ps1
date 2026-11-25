param([string]$Mode, [string]$Efk, [string]$Xml)
# efkefc の EDIT チャンク(zlib の XML)を取り出す / 戻す
Add-Type -AssemblyName System.IO.Compression

function Read-Chunks([byte[]]$b) {
    $list = @()
    $p = 8
    while ($p -lt $b.Length) {
        $id = [Text.Encoding]::ASCII.GetString($b, $p, 4)
        $size = [BitConverter]::ToInt32($b, $p + 4)
        $data = New-Object byte[] $size
        [Array]::Copy($b, $p + 8, $data, 0, $size)
        $list += ,@($id, $data)
        $p += 8 + $size
    }
    return ,$list
}

$bytes = [IO.File]::ReadAllBytes($Efk)
$chunks = Read-Chunks $bytes

if ($Mode -eq 'get') {
    foreach ($c in $chunks) {
        if ($c[0] -eq 'EDIT') {
            $d = $c[1]
            $ms = New-Object IO.MemoryStream(,$d)
            $ms.Position = 2
            $ds = New-Object IO.Compression.DeflateStream($ms, [IO.Compression.CompressionMode]::Decompress)
            $out = New-Object IO.MemoryStream
            $ds.CopyTo($out)
            [IO.File]::WriteAllBytes($Xml, $out.ToArray())
        }
    }
}
elseif ($Mode -eq 'set') {
    $raw = [IO.File]::ReadAllBytes($Xml)
    $out = New-Object IO.MemoryStream
    $out.WriteByte(0x78); $out.WriteByte(0x9C)
    $ds = New-Object IO.Compression.DeflateStream($out, [IO.Compression.CompressionLevel]::Optimal, $true)
    $ds.Write($raw, 0, $raw.Length)
    $ds.Close()
    [uint32]$a = 1; [uint32]$bb = 0
    foreach ($x in $raw) { $a = ($a + $x) % 65521; $bb = ($bb + $a) % 65521 }
    $ad = [uint32](($bb -shl 16) -bor $a)
    $out.WriteByte([byte](($ad -shr 24) -band 0xFF)); $out.WriteByte([byte](($ad -shr 16) -band 0xFF))
    $out.WriteByte([byte](($ad -shr 8) -band 0xFF)); $out.WriteByte([byte]($ad -band 0xFF))
    $comp = $out.ToArray()

    $res = New-Object IO.MemoryStream
    $res.Write($bytes, 0, 8)
    foreach ($c in $chunks) {
        $data = $c[1]
        if ($c[0] -eq 'EDIT') { $data = $comp }
        $idb = [Text.Encoding]::ASCII.GetBytes($c[0])
        $res.Write($idb, 0, 4)
        $res.Write([BitConverter]::GetBytes([int]$data.Length), 0, 4)
        $res.Write($data, 0, $data.Length)
    }
    [IO.File]::WriteAllBytes($Efk, $res.ToArray())
}
