// 死亡演出の「死」の字を作る(Illustrator の ExtendScript)
// MakeDeathText.ps1 から実行する。
// 出力: 2026_winter/data/UI/Death/shi.png(字・にじみ・飛沫・墨) / shi_drip.png(垂れだけ。同じ大きさ)
//       out/_preview.png(GAME_STYLE を暗い背景と灰色の背景で)・out/_ink.png(墨の入れ方の比べ)・death_text.ai
#target illustrator

(function () {
    var here = File($.fileName).parent;
    var outDir = new Folder(here.fsName + "/out");
    if (!outDir.exists) outDir.create();
    var gameDir = new Folder(here.parent.parent.fsName + "/2026_winter/data/UI/Death");
    if (!gameDir.exists) gameDir.create();

    var FONT = "KsoKokuryuSou";
    var SIZE = 512;
    var KANJI_H = 360, KANJI_CY = 300;     // 字の高さと中心(下から)。下は垂れの場所
    var RED = [200, 24, 40], DARK_RED = [110, 8, 20], INK = [14, 10, 12], INK_RED = [42, 6, 12];
    var BG = [14, 8, 16], BG_GRAY = [58, 56, 62], BG_GAME = [44, 42, 48];
    var HALO_W = 6, HALO_OPACITY = 32;
    var SPLAT_NUM = 22, DRIP_NUM = 4, DRIP_LEN = [[90, 130], [55, 85], [30, 50], [14, 24]], DRIP_W = [[7, 9], [4.5, 6.5], [4, 6], [5, 7]];

    // 墨の入れ方: none / blot(飛沫) / halo(にじみ) / double(墨の字と重ねる) / grad(血に墨が混ざる) / band(墨のひと払い)
    var GAME_STYLE = "blot";
    var INK_STYLES = [["none", "なし(今)"], ["blot", "A 墨の飛沫"], ["halo", "B 墨のにじみ"], ["double", "C 墨の字と重ねる"], ["grad", "D 血に墨が混ざる"], ["band", "E 墨のひと払い"]];

    function rgb(c) { var o = new RGBColor(); o.red = c[0]; o.green = c[1]; o.blue = c[2]; return o; }
    var seed = 1, seed2 = 1;
    function rnd() { seed = (seed * 16807) % 2147483647; return (seed - 1) / 2147483646; }
    function range(a, b) { return a + (b - a) * rnd(); }
    function rnd2() { seed2 = (seed2 * 16807) % 2147483647; return (seed2 - 1) / 2147483646; }
    function range2(a, b) { return a + (b - a) * rnd2(); }
    function font(name) {
        try { return app.textFonts.getByName(name); } catch (e) { return app.textFonts.getByName("YuMincho-Demibold"); }
    }
    function fill(p, color, opacity) {
        p.stroked = false; p.filled = true; p.fillColor = rgb(color);
        if (opacity !== undefined) p.opacity = opacity;
        return p;
    }
    function circle(parent, x, y, r, color) { return fill(parent.pathItems.ellipse(y + r, x - r, r * 2, r * 2), color); }
    function ellipseRot(parent, x, y, rw, rh, deg, color) {
        var e = circle(parent, x, y, 1, color);
        e.resize(rw * 100, rh * 100);
        e.rotate(deg);
        return e;
    }
    function poly(parent, pts, color, opacity) {
        var p = parent.pathItems.add();
        p.setEntirePath(pts);
        p.closed = true;
        return fill(p, color, opacity);
    }
    function eachPath(item, fn) {
        var i;
        if (item.typename == "PathItem") fn(item);
        else if (item.typename == "CompoundPathItem") for (i = 0; i < item.pathItems.length; i++) fn(item.pathItems[i]);
        else if (item.typename == "GroupItem") for (i = 0; i < item.pageItems.length; i++) eachPath(item.pageItems[i], fn);
    }
    function makeNoise(rf) {
        var w = [];
        for (var i = 0; i < 5; i++) w.push([rf(2, 9) * (i + 1), rf(0, 6.28), rf(0.3, 1.0) / (i + 1)]);
        return function (t) {
            var v = 0;
            for (var i = 0; i < w.length; i++) v += Math.sin(t * w[i][0] + w[i][1]) * w[i][2];
            return v;
        };
    }
    function paint(item, color, strokeW, opacity) {
        eachPath(item, function (p) {
            p.filled = true; p.fillColor = rgb(color);
            if (strokeW > 0) { p.stroked = true; p.strokeColor = rgb(color); p.strokeWidth = strokeW; p.strokeJoin = StrokeJoin.ROUNDENDJOIN; }
            else p.stroked = false;
        });
        item.opacity = opacity;
        return item;
    }

    // 墨(字の後ろに敷くもの)
    function inkBlot(g, cx, cy) {
        var n = makeNoise(range2), pts = [], N = 120;
        for (var i = 0; i < N; i++) {
            var a = i / N * Math.PI * 2, r = 150 * (1 + 0.22 * n(a));
            if (rnd2() < 0.05) r += range2(12, 36);
            pts.push([cx + Math.cos(a) * r * 1.15, cy + Math.sin(a) * r * 0.95]);
        }
        poly(g, pts, INK, 92);
        for (var k = 0; k < 8; k++) {
            var a = range2(0, 6.28), d = range2(170, 215);
            ellipseRot(g, cx + Math.cos(a) * d * 1.1, cy + Math.sin(a) * d * 0.9, range2(16, 34), range2(3, 6), a * 180 / Math.PI, INK);
        }
        for (var k = 0; k < 30; k++) {
            var a = range2(0, 6.28), d = range2(175, 245);
            circle(g, cx + Math.cos(a) * d * 1.1, cy + Math.sin(a) * d * 0.9, range2(1.5, 7), INK);
        }
    }
    function inkBand(g, cx, cy) {
        var nT = makeNoise(range2), nB = makeNoise(range2), x0 = cx - 235, x1 = cx + 240, hh = 80, N = 160;
        function half(t) { return hh * (t < 0.06 ? 0.75 + 0.25 * t / 0.06 : (t > 0.75 ? Math.max(0.15, 1 - (t - 0.75) / 0.25 * 0.85) : 1)); }
        function P(t, off) { return [x0 + (x1 - x0) * t, cy + 10 - 22 * t + off]; }
        var cp = g.compoundPathItems.add(), body = cp.pathItems.add(), top = [], bot = [];
        for (var i = 0; i <= N; i++) {
            var t = i / N, h = half(t);
            top.push(P(t, h + nT(t * 6) * 6));
            bot.unshift(P(t, -h + nB(t * 6) * 6));
        }
        body.setEntirePath(top.concat(bot)); body.closed = true;
        for (var k = 0; k < 16; k++) {
            var s = range2(0.3, 0.85), e = Math.min(1, s + range2(0.1, 0.35)), lane = range2(-0.8, 0.8), gw = range2(3, 9);
            var pts = [], M = 40;
            for (var i = 0; i <= M * 2 + 1; i++) {
                var j = i <= M ? i : M * 2 + 1 - i, sign = i <= M ? 1 : -1, t = s + (e - s) * j / M;
                pts.push(P(t, lane * half(t) + sign * gw * Math.sin(Math.PI * j / M) / 2));
            }
            var h = cp.pathItems.add(); h.setEntirePath(pts); h.closed = true;
        }
        for (var q = 0; q < cp.pathItems.length; q++) {
            fill(cp.pathItems[q], INK);
            cp.pathItems[q].polarity = q == 0 ? PolarityValues.POSITIVE : PolarityValues.NEGATIVE;
        }
        cp.opacity = 94;
        for (var k = 0; k < 10; k++) {
            var y = range2(-0.5, 0.5) * hh, s = P(1, y);
            poly(g, [[s[0] - 40, s[1] + 1.5], [s[0] + range2(10, 50), s[1] - range2(-4, 4)], [s[0] - 40, s[1] - 1.5]], INK, 90);
        }
        for (var k = 0; k < 7; k++) circle(g, x0 + range2(-6, 14), cy + 10 + range2(-0.8, 0.8) * hh, range2(4, 10), INK).opacity = 94;   // 置いたところの墨のはみ出し
    }
    function gradientFill(doc, item, top, bottom) {
        var gr = doc.gradients.add();
        gr.type = GradientType.LINEAR;
        gr.gradientStops[0].rampPoint = 0; gr.gradientStops[0].color = rgb(top);
        gr.gradientStops[1].rampPoint = 100; gr.gradientStops[1].color = rgb(bottom);
        gr.gradientStops[0].midPoint = 62;
        var gc = new GradientColor(); gc.gradient = gr;
        eachPath(item, function (p) { p.fillColor = gc; });
        eachPath(item, function (p) { p.rotate(-90, false, false, true, false); });
    }

    // 1つ分(字・墨・飛沫・垂れ)を作る。ox, oy はずらし
    function build(doc, style, ox, oy) {
        seed = 4242; seed2 = 777;
        var root = doc.groupItems.add();
        var back = root.groupItems.add();
        var main = root.groupItems.add();
        var drip = root.groupItems.add();

        var tf = doc.textFrames.add();
        tf.contents = "死";
        tf.textRange.characterAttributes.textFont = font(FONT);
        tf.textRange.characterAttributes.size = 300;
        tf.textRange.characterAttributes.fillColor = rgb(RED);
        var glyph = tf.createOutline();
        var b = glyph.geometricBounds;
        var sc = KANJI_H / (b[1] - b[3]) * 100;
        glyph.resize(sc, sc);
        b = glyph.geometricBounds;
        glyph.translate(ox + SIZE / 2 - (b[0] + b[2]) / 2, oy + KANJI_CY - (b[1] + b[3]) / 2);
        b = glyph.geometricBounds;   // [左, 上, 右, 下]
        var cx = (b[0] + b[2]) / 2, cy = (b[1] + b[3]) / 2;
        glyph.move(main, ElementPlacement.PLACEATEND);

        var anchors = [];
        eachPath(glyph, function (p) {
            for (var i = 0; i < p.pathPoints.length; i++) anchors.push(p.pathPoints[i].anchor);
        });

        // 墨
        if (style == "blot") inkBlot(back, cx + 8, cy - 4);
        if (style == "band") inkBand(back, cx, cy);
        if (style == "halo") {
            // 字のふちに大きさの違う墨の粒を重ねて、にじみをムラにする
            paint(glyph.duplicate(back, ElementPlacement.PLACEATEND), INK, 8, 60);
            for (var k = 0; k < 140; k++) {
                var a = anchors[Math.floor(rnd2() * anchors.length)];
                var dx = a[0] - cx, dy = a[1] - cy, d = Math.sqrt(dx * dx + dy * dy) || 1, o = range2(-2, 12);
                circle(back, a[0] + dx / d * o, a[1] + dy / d * o, range2(5, 17), INK).opacity = range2(25, 55);
            }
            for (var k = 0; k < 26; k++) {
                var a = anchors[Math.floor(rnd2() * anchors.length)];
                var dx = a[0] - cx, dy = a[1] - cy, d = Math.sqrt(dx * dx + dy * dy) || 1, o = range2(20, 38);
                circle(back, a[0] + dx / d * o, a[1] + dy / d * o, range2(1.5, 5), INK).opacity = 80;
            }
        }
        if (style == "double") {
            var ink = paint(glyph.duplicate(back, ElementPlacement.PLACEATEND), INK, 4, 94);
            ink.resize(104, 104);
            ink.translate(-13, -10);
        }

        // にじみ: 字のふちに暗い赤の太い線を後ろに敷く
        var halo = glyph.duplicate(main, ElementPlacement.PLACEATEND);
        paint(halo, DARK_RED, HALO_W, HALO_OPACITY);
        if (style == "grad") gradientFill(doc, glyph, RED, INK_RED);

        // 飛沫: 字のふちの点から外へ
        var splatColor = style == "double" || style == "halo" ? INK : RED;
        for (var k = 0; k < SPLAT_NUM; k++) {
            var a = anchors[Math.floor(rnd() * anchors.length)];
            var dx = a[0] - cx, dy = a[1] - cy, d = Math.sqrt(dx * dx + dy * dy) || 1;
            var o = range(8, 34), r = k < 5 ? range(3, 6) : range(1.2, 3.2);
            var px = a[0] + dx / d * o, py = a[1] + dy / d * o;
            var c = k % 3 == 0 ? splatColor : RED;
            if (rnd() < 0.4) ellipseRot(main, px, py, r * 2.6, r, Math.atan2(dy, dx) * 180 / Math.PI, c);
            else circle(main, px, py, r, c);
        }

        // 垂れ: 字の下側(横を区切った中で一番下の点)から下へ
        var BINS = 9, bins = [];
        for (var i = 0; i < BINS; i++) bins.push(null);
        for (var i = 0; i < anchors.length; i++) {
            var bi = Math.floor((anchors[i][0] - b[0]) / (b[2] - b[0]) * BINS);
            bi = Math.max(0, Math.min(BINS - 1, bi));
            if (!bins[bi] || anchors[i][1] < bins[bi][1]) bins[bi] = anchors[i];
        }
        var order = [];
        for (var i = 1; i < BINS - 1; i++) if (bins[i]) order.push(bins[i]);
        for (var i = order.length - 1; i > 0; i--) { var j = Math.floor(rnd() * (i + 1)); var tmp = order[i]; order[i] = order[j]; order[j] = tmp; }
        var dripTop = 0, dripColor = style == "grad" ? INK_RED : RED;
        for (var k = 0; k < Math.min(DRIP_NUM, order.length); k++) {
            var top = [order[k][0], order[k][1] + 10];
            var len = range(DRIP_LEN[k][0], DRIP_LEN[k][1]), dw = range(DRIP_W[k][0], DRIP_W[k][1]);   // 1本目が長く太い
            if (top[1] - len < oy + 14) len = top[1] - oy - 14;
            var pts = [], M = 20, u;
            for (var i = 0; i <= M; i++) { u = i / M; pts.push([top[0] + dw * (1 - 0.5 * u) * (1 - 0.15 * Math.sin(u * 9)), top[1] - len * u]); }
            for (var i = M; i >= 0; i--) { u = i / M; pts.push([top[0] - dw * (1 - 0.5 * u) * (1 - 0.15 * Math.sin(u * 9)), top[1] - len * u]); }
            poly(drip, pts, dripColor);
            circle(drip, top[0], top[1] - len, dw * 0.85, dripColor);
            dripTop = Math.max(dripTop, top[1] - oy);
        }
        return { root: root, main: main, back: back, drip: drip, dripTop: dripTop, bottom: b[3] - oy };
    }

    function exportPng(doc, file, transparent) {
        var o = new ExportOptionsPNG24();
        o.transparency = transparent; o.artBoardClipping = true; o.antiAliasing = true;
        doc.exportFile(file, ExportType.PNG24, o);
    }
    function bgRect(doc, x, y, w, h, c) {
        var r = doc.pathItems.rectangle(y + h, x, w, h); fill(r, c); r.zOrder(ZOrderMethod.SENDTOBACK); return r;
    }

    // ゲーム用
    var doc = app.documents.add(DocumentColorSpace.RGB, SIZE, SIZE);
    doc.artboards[0].artboardRect = [0, SIZE, SIZE, 0];
    var s = build(doc, GAME_STYLE, 0, 0);
    s.drip.hidden = true;
    exportPng(doc, new File(gameDir.fsName + "/shi.png"), true);
    s.drip.hidden = false; s.main.hidden = true; s.back.hidden = true;
    exportPng(doc, new File(gameDir.fsName + "/shi_drip.png"), true);
    s.main.hidden = false; s.back.hidden = false;

    // 確認用: 暗い背景と、色が抜けたステージくらいの灰色の背景に並べる
    var copy = s.root.duplicate();
    copy.translate(SIZE, 0);
    doc.artboards[0].artboardRect = [0, SIZE, SIZE * 2, 0];
    bgRect(doc, 0, 0, SIZE, SIZE, BG);
    bgRect(doc, SIZE, 0, SIZE, SIZE, BG_GRAY);
    exportPng(doc, new File(outDir.fsName + "/_preview.png"), false);
    doc.saveAs(new File(outDir.fsName + "/death_text.ai"));
    doc.close(SaveOptions.DONOTSAVECHANGES);

    // 墨の入れ方の比べ(3列×2行。ゲームで「死」が出るときくらいの暗い灰色)
    var COLS = 3, ROWS = Math.ceil(INK_STYLES.length / COLS), LH = 40;
    var CH = (SIZE + LH) * ROWS;
    var cmp = app.documents.add(DocumentColorSpace.RGB, SIZE * COLS, CH);
    cmp.artboards[0].artboardRect = [0, CH, SIZE * COLS, 0];
    for (var i = 0; i < INK_STYLES.length; i++) {
        var c = i % COLS, r = Math.floor(i / COLS), ox = SIZE * c, oy = CH - (SIZE + LH) * (r + 1) + LH;
        build(cmp, INK_STYLES[i][0], ox, oy);
        var lb = cmp.textFrames.add();
        lb.contents = INK_STYLES[i][1];
        lb.textRange.characterAttributes.textFont = app.textFonts.getByName("YuGothic-Medium");
        lb.textRange.characterAttributes.size = 20;
        lb.textRange.characterAttributes.fillColor = rgb([210, 204, 196]);
        lb.position = [ox + SIZE / 2 - lb.width / 2, oy - 8];
    }
    bgRect(cmp, 0, 0, SIZE * COLS, CH, BG_GAME);
    exportPng(cmp, new File(outDir.fsName + "/_ink.png"), false);
    cmp.close(SaveOptions.DONOTSAVECHANGES);

    // 垂れの始まり(画像の上からのピクセル)と、字の下の端
    return "ok style=" + GAME_STYLE + " dripTopY=" + Math.round(SIZE - s.dripTop) + " glyphBottomY=" + Math.round(SIZE - s.bottom) + " font=" + font(FONT).name;
})();
