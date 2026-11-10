# 操作説明(パッド)の画像を作る
# 実行: powershell -ExecutionPolicy Bypass -File MakeControlGuide.ps1
# 出力: 進捗/操作説明_pad.png
Add-Type -AssemblyName System.Drawing

$out = Join-Path $PSScriptRoot "..\..\進捗\操作説明_pad.png"
$W = 1920; $H = 1080

$bmp = New-Object System.Drawing.Bitmap($W, $H, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = 'AntiAlias'
$g.TextRenderingHint = 'AntiAliasGridFit'

function C($r, $gg, $b, $a = 255) { [System.Drawing.Color]::FromArgb($a, $r, $gg, $b) }
function Brush($c) { New-Object System.Drawing.SolidBrush($c) }
function PenC($c, $w) { $p = New-Object System.Drawing.Pen($c, $w); $p.StartCap = 'Round'; $p.EndCap = 'Round'; $p.LineJoin = 'Round'; $p }
function Font($size, $bold = $true) {
    $name = if ($bold) { "Noto Sans JP Medium" } else { "Noto Sans JP" }
    New-Object System.Drawing.Font($name, $size, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
}

$red = C 220 40 50
$white = C 240 240 244
$gray = C 160 160 170
$bodyC = C 44 45 54
$edgeC = C 92 94 108
$partC = C 62 63 74

# 背景(上が紺、下が赤黒)
$bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush((New-Object System.Drawing.Point(0, 0)), (New-Object System.Drawing.Point(0, $H)), (C 16 17 26), (C 40 14 20))
$g.FillRectangle($bg, 0, 0, $W, $H)

# タイトル
$g.FillRectangle((Brush $red), 80, 58, 8, 52)
$g.DrawString("操作説明", (Font 46), (Brush $white), 100, 50)
$g.DrawString("GAMEPAD", (Font 20 $false), (Brush $gray), 304, 78)
$g.DrawLine((PenC (C 220 40 50 160) 2), 80, 130, 1840, 130)

# ---- コントローラー ----
$cx = 960

# トリガー・バンパー
function RoundRect($x, $y, $w, $h, $r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $r * 2
    $p.AddArc($x, $y, $d, $d, 180, 90)
    $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    $p
}
function Part($x, $y, $w, $h, $r, $label) {
    $p = RoundRect $x $y $w $h $r
    $g.FillPath((Brush $partC), $p)
    $g.DrawPath((PenC $edgeC 2), $p)
    $f = Font 20
    $sz = $g.MeasureString($label, $f)
    $g.DrawString($label, $f, (Brush $white), $x + ($w - $sz.Width) / 2, $y + 5)
}
Part 670 292 110 70 16 "LT"
Part 1140 292 110 70 16 "RT"
Part 650 334 170 60 22 "LB"
Part 1100 334 170 60 22 "RB"

# 本体
$body = New-Object System.Drawing.Drawing2D.GraphicsPath
$body.AddBezier(700, 372, 820, 362, 1100, 362, 1220, 372)
$body.AddBezier(1220, 372, 1330, 380, 1372, 470, 1370, 650)
$body.AddBezier(1370, 650, 1368, 800, 1290, 820, 1240, 760)
$body.AddBezier(1240, 760, 1210, 725, 1195, 690, 1180, 680)
$body.AddBezier(1180, 680, 1050, 662, 870, 662, 740, 680)
$body.AddBezier(740, 680, 725, 690, 710, 725, 680, 760)
$body.AddBezier(680, 760, 630, 820, 552, 800, 550, 650)
$body.AddBezier(550, 650, 548, 470, 590, 380, 700, 372)
$body.CloseFigure()
$g.FillPath((Brush $bodyC), $body)
$g.DrawPath((PenC $edgeC 3), $body)

# スティック
function Stick($x, $y, $label) {
    $g.FillEllipse((Brush (C 30 31 38)), $x - 52, $y - 52, 104, 104)
    $g.FillEllipse((Brush (C 74 75 88)), $x - 38, $y - 38, 76, 76)
    $g.DrawEllipse((PenC $edgeC 2), $x - 38, $y - 38, 76, 76)
    $f = Font 15
    $sz = $g.MeasureString($label, $f)
    $g.DrawString($label, $f, (Brush $gray), $x - $sz.Width / 2, $y - $sz.Height / 2)
}
Stick 725 490 "L"
Stick 1080 615 "R"

# 十字キー
$dx = 840; $dy = 615
$g.FillEllipse((Brush (C 30 31 38)), $dx - 52, $dy - 52, 104, 104)
$g.FillRectangle((Brush (C 74 75 88)), $dx - 14, $dy - 40, 28, 80)
$g.FillRectangle((Brush (C 74 75 88)), $dx - 40, $dy - 14, 80, 28)

# 中央のボタン(ビュー / ホーム / メニュー)
$g.FillEllipse((Brush $partC), 898 - 17, 490 - 17, 34, 34)
$g.FillEllipse((Brush $partC), 960 - 26, 427 - 26, 52, 52)
$g.DrawEllipse((PenC $edgeC 2), 960 - 26, 427 - 26, 52, 52)
$mx = 1022; $my = 490
$g.FillEllipse((Brush $partC), $mx - 17, $my - 17, 34, 34)
foreach ($o in -6, 0, 6) { $g.DrawLine((PenC $white 2.5), $mx - 8, $my + $o, $mx + 8, $my + $o) }

# ABXY
function Face($x, $y, $col, $label) {
    $g.FillEllipse((Brush $col), $x - 24, $y - 24, 48, 48)
    $f = Font 22
    $sz = $g.MeasureString($label, $f)
    $g.DrawString($label, $f, (Brush $white), $x - $sz.Width / 2 + 1, $y - $sz.Height / 2)
}
Face 1195 438 (C 232 190 40) "Y"
Face 1143 490 (C 60 120 230) "X"
Face 1247 490 (C 220 50 55) "B"
Face 1195 543 (C 70 185 80) "A"

# ---- 引き出し線と説明 ----
$linePen = PenC (C 220 40 50 230) 2
function Dot($x, $y) { $g.FillEllipse((Brush $red), $x - 6, $y - 6, 12, 12) }
function Lead($pts) {
    for ($i = 0; $i -lt $pts.Count - 1; $i++) {
        $g.DrawLine($linePen, $pts[$i][0], $pts[$i][1], $pts[$i + 1][0], $pts[$i + 1][1])
    }
    Dot $pts[0][0] $pts[0][1]
}
$fMain = Font 30
$fSub = Font 19 $false
# 左寄せ(右側の説明)
function LabelR($x, $y, $key, $text, $sub) {
    $g.DrawString($key, $fMain, (Brush $white), $x, $y - 22)
    $kw = $g.MeasureString($key, $fMain).Width
    $g.DrawString($text, $fMain, (Brush $white), $x + [Math]::Max($kw, 40) + 14, $y - 22)
    if ($sub) { $g.DrawString($sub, $fSub, (Brush $gray), $x + 2, $y + 22) }
}
# 右寄せ(左側の説明)
function LabelL($x, $y, $key, $text, $sub) {
    $tw = $g.MeasureString($text, $fMain).Width
    $g.DrawString($text, $fMain, (Brush $white), $x - $tw, $y - 22)
    $kw = $g.MeasureString($key, $fMain).Width
    $g.DrawString($key, $fMain, (Brush $white), $x - $tw - 18 - $kw, $y - 22)
    if ($sub) {
        $sw = $g.MeasureString($sub, $fSub).Width
        $g.DrawString($sub, $fSub, (Brush $gray), $x - $sw - 2, $y + 22)
    }
}

# 左側
$lx = 560; $ltx = 540
Lead @(@(672, 318), @(600, 230), @($lx, 230));   LabelL $ltx 230 "LT" "必殺技モード" "モード中にもう一度押すと 決殺"
Lead @(@(652, 362), @(600, 370), @($lx, 370));   LabelL $ltx 370 "LB" "スキル攻撃" "空中でも出せる"
Lead @(@(676, 490), @(600, 520), @($lx, 520));   LabelL $ltx 520 "左スティック" "移動" "走り続けるとダッシュ"
Lead @(@(692, 533), @(600, 650), @($lx, 650));   LabelL $ltx 650 "L3（押し込み）" "ダッシュ" $null
Lead @(@(790, 615), @(600, 780), @($lx, 780));   LabelL $ltx 780 "十字キー" "メニュー選択" $null

# 上(メニュー)
Lead @(@($mx, 472), @($mx, 230))
$mt = "メニュー　ポーズ"
$g.DrawString($mt, $fMain, (Brush $white), $mx - $g.MeasureString($mt, $fMain).Width / 2, 180)

# 右側
$rx = 1360; $rtx = 1385
Lead @(@(1268, 360), @(1320, 290), @($rx, 290));  LabelR $rtx 290 "RB" "ロックオン" "もう一度押すと解除"
Lead @(@(1143, 466), @(1143, 395), @(1320, 395), @($rx, 395)); LabelR $rtx 395 "X" "弱攻撃" $null
Lead @(@(1219, 438), @(1320, 480), @($rx, 480));  LabelR $rtx 480 "Y" "強攻撃" "条件を満たすと 暗殺 / 確殺 / ガード崩し"
Lead @(@(1271, 490), @(1320, 580), @($rx, 580));  LabelR $rtx 580 "B" "回避" $null
Lead @(@(1219, 543), @(1320, 680), @($rx, 680));  LabelR $rtx 680 "A" "ジャンプ" "壁の前で 壁走り / 壁蹴り"
Lead @(@(1132, 615), @(1320, 820), @($rx, 820));  LabelR $rtx 820 "右スティック" "カメラ操作" "ロックオン中は左右でターゲット切替"

$g.Dispose()
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host "saved $out"
