// リザルトの項目アイコン(技・殺・時・道・死)を筆の円相で作る(Illustrator の ExtendScript)
// MakeResultIcon.ps1 から実行する。
// 出力: 2026_winter/data/UI/Result/icon_*.png(透過)、out/_preview.png、out/_brushes.png(筆の型の比べ)
#target illustrator

(function () {
    var here = File($.fileName).parent;
    var outDir = new Folder(here.fsName + "/out");
    if (!outDir.exists) outDir.create();
    var gameDir = new Folder(here.parent.parent.fsName + "/2026_winter/data/UI/Result");

    var ICONS = [["技", "tech"], ["殺", "kill"], ["時", "time"], ["道", "item"], ["死", "death"]];
    var FONT = "KsoKokuryuSou";
    var GAME_STYLE = "F";
    var SIZE = 256, C = SIZE / 2;
    var RED = [230, 45, 70], DARK_RED = [150, 20, 40], WHITE = [232, 226, 214], BG = [14, 8, 16];
    var KANJI_H = 116;

    // 筆の型
    var BASE = {
        ringR: 100, discR: 94, startDeg: 62, sweep: 316, wStart: 32, wMid: 18, taper: 0.65,
        rough: 3.2, gapNum: 11, gapW: [0.6, 2.2], hairNum: 9, splatNum: 14, splatR: 1.0,
        flick: 0, drips: 0, second: false
    };
    var STYLES = {
        A: {},                                                          // 円相(今の)
        B: { wMid: 21, taper: 0.45, rough: 4.2, gapNum: 22, gapW: [1.4, 4.0], hairNum: 18 },       // かすれ
        C: { ringR: 94, discR: 84, wStart: 46, wMid: 28, sweep: 290, rough: 4.5, splatNum: 24, splatR: 1.7 }, // 太い一筆
        D: { wStart: 24, wMid: 13, sweep: 300, second: true },         // 二度書き
        E: { ringR: 92, startDeg: 215, sweep: 250, wStart: 36, wMid: 20, taper: 0.55, flick: 26 },  // 払い
        F: { ringR: 88, discR: 82, drips: 5 }                          // 垂れ
    };
    var STYLE_ORDER = ["A", "B", "C", "D", "E", "F"];

    function rgb(c) { var o = new RGBColor(); o.red = c[0]; o.green = c[1]; o.blue = c[2]; return o; }
    function styleOf(key) {
        var s = {}, k;
        for (k in BASE) s[k] = BASE[k];
        for (k in STYLES[key]) s[k] = STYLES[key][k];
        return s;
    }

    var seed = 1;
    function rnd() { seed = (seed * 16807) % 2147483647; return (seed - 1) / 2147483646; }
    function range(a, b) { return a + (b - a) * rnd(); }

    function font(name) {
        try { return app.textFonts.getByName(name); } catch (e) { return app.textFonts.getByName("YuMincho-Demibold"); }
    }
    function fill(p, color, opacity) {
        p.stroked = false;
        p.filled = true;
        p.fillColor = rgb(color);
        if (opacity !== undefined) p.opacity = opacity;
        return p;
    }
    function circle(parent, x, y, r, color, opacity) {
        return fill(parent.pathItems.ellipse(y + r, x - r, r * 2, r * 2), color, opacity);
    }
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
    function makeNoise() {
        var w = [];
        for (var i = 0; i < 5; i++) w.push([range(4, 40) * (i + 1), range(0, 6.28), range(0.3, 1.0) / (i + 1)]);
        return function (t) {
            var v = 0;
            for (var i = 0; i < w.length; i++) v += Math.sin(t * w[i][0] + w[i][1]) * w[i][2];
            return v;
        };
    }

    // 1本の筆の線(輪)を描く
    function stroke(g, st, color, opacity) {
        var startDeg = st.startDeg + range(-8, 8), sweep = st.sweep + range(-10, 6);
        var wobblePhase = range(0, 6.28);
        var nOut = makeNoise(), nIn = makeNoise();

        function width(t) {
            var w;
            if (t < 0.04) w = st.wStart * (0.7 + 0.3 * Math.sqrt(t / 0.04));
            else if (t < st.taper) w = st.wStart + (st.wMid - st.wStart) * ((t - 0.04) / (st.taper - 0.04));
            else w = st.wMid * Math.pow(Math.max(0, 1 - (t - st.taper) / (1 - st.taper)), 1.2);
            return w * (1 + 0.12 * Math.sin(t * 23 + wobblePhase));
        }
        function pt(t, offset) {
            var a = (startDeg - sweep * t) * Math.PI / 180;
            var f = st.flick * Math.pow(Math.max(0, (t - 0.7) / 0.3), 2);
            var r = st.ringR + 3 * Math.sin(t * 7 + wobblePhase) + f + offset;
            return [C + Math.cos(a) * r, C + Math.sin(a) * r];
        }
        function band(from, to, wFunc, offset, rough) {
            var N = 220, outer = [], inner = [];
            for (var i = 0; i <= N; i++) {
                var t = from + (to - from) * i / N, w = wFunc(t) / 2;
                var k = Math.min(1, w / 6) * st.rough;
                var jo = rough ? (nOut(t) + range(-0.35, 0.35)) * k : 0;
                var ji = rough ? (nIn(t) + range(-0.35, 0.35)) * k : 0;
                outer.push(pt(t, offset + w + jo));
                inner.unshift(pt(t, offset - w - ji));
            }
            return outer.concat(inner);
        }

        // 本体: かすれの筋を穴にした複合パス
        var cp = g.compoundPathItems.add();
        var body = cp.pathItems.add();
        body.setEntirePath(band(0, 1, width, 0, true));
        body.closed = true;
        for (var k = 0; k < st.gapNum; k++) {
            var s = range(0.25, 0.9), e = Math.min(0.995, s + range(0.08, 0.3));
            var lane = range(-0.36, 0.36), gw = range(st.gapW[0], st.gapW[1]);
            var pts = [], N2 = 60, i, t, w, hw, off;
            for (i = 0; i <= N2 * 2 + 1; i++) {
                var j = i <= N2 ? i : N2 * 2 + 1 - i, sign = i <= N2 ? 1 : -1;
                t = s + (e - s) * j / N2;
                w = width(t);
                hw = gw * Math.sin(Math.PI * j / N2) / 2;
                off = lane * w;
                if (Math.abs(off) + hw > w / 2 - 1.2) hw = Math.max(0, w / 2 - 1.2 - Math.abs(off));
                pts.push(pt(t, off + sign * hw));
            }
            var h = cp.pathItems.add();
            h.setEntirePath(pts);
            h.closed = true;
        }
        for (var q = 0; q < cp.pathItems.length; q++) {
            fill(cp.pathItems[q], color);
            cp.pathItems[q].polarity = q == 0 ? PolarityValues.POSITIVE : PolarityValues.NEGATIVE;
        }
        if (opacity !== undefined) cp.opacity = opacity;

        // 抜けのあたりのはぐれた毛
        for (var k = 0; k < st.hairNum; k++) {
            var s = range(st.taper - 0.1, 0.85), e = Math.min(1.04, s + range(0.12, 0.3));
            var off = range(-1, 1) * st.wMid * 0.75, hw = range(0.7, 1.8);
            poly(g, band(s, e, (function (s, e, hw) {
                return function (t) { return hw * Math.sin(Math.PI * (t - s) / (e - s)); };
            })(s, e, hw), off, false), color, range(45, 90));
        }

        // 置いたところの塊
        var s0 = pt(0, 0);
        for (var k = 0; k < 4; k++) circle(g, s0[0] + range(-5, 5), s0[1] + range(-5, 5), st.wStart * range(0.3, 0.48), color);

        // 飛び散り
        for (var k = 0; k < st.splatNum; k++) {
            var t = k < st.splatNum * 0.55 ? range(0, 0.08) : range(0.1, 0.7);
            var side = rnd() < 0.7 ? 1 : -1;
            var p = pt(t, side * (width(t) / 2 + range(3, 16) * st.splatR));
            var r = range(0.8, k < 4 ? 4.2 : 2.6) * st.splatR;
            if (rnd() < 0.35) ellipseRot(g, p[0], p[1], r * 2.4, r, Math.atan2(p[1] - C, p[0] - C) * 180 / Math.PI, color);
            else circle(g, p[0], p[1], r, color);
        }

        // 垂れ: 輪の下のほうから下へ
        for (var k = 0; k < st.drips; k++) {
            var t = 0.1 + 0.8 * (k + range(0.2, 0.8)) / st.drips;
            var a = (startDeg - sweep * t) * Math.PI / 180;
            if (Math.sin(a) > -0.15) continue;   // 下半分だけ
            var top = pt(t, 0), len = range(14, 30) * (-Math.sin(a)), dw = Math.min(width(t) * 0.35, range(2.5, 5));
            if (top[1] - len < 6) len = top[1] - 6;
            var d = [], M = 16;
            for (var i = 0; i <= M; i++) { var u = i / M; d.push([top[0] + dw * (1 - 0.55 * u), top[1] - len * u]); }
            for (var i = M; i >= 0; i--) { var u = i / M; d.push([top[0] - dw * (1 - 0.55 * u), top[1] - len * u]); }
            poly(g, d, color);
            circle(g, top[0], top[1] - len, dw * 0.9, color);
        }

        // 内側のふちの暗いにじみ
        poly(g, band(0.02, 0.6, function () { return 2.2; }, -width(0.3) / 2 + 1.5, false), DARK_RED, 55);
    }

    function makeIcon(doc, kanji, iconSeed, styleKey) {
        seed = iconSeed;
        var st = styleOf(styleKey);
        var g = doc.groupItems.add();
        circle(g, C, C, st.discR, BG, 92);
        stroke(g, st, RED);
        if (st.second) {
            // 二度目: 少し外側を、ずらした所から速く払う
            var s2 = styleOf(styleKey);
            s2.ringR = st.ringR + 9; s2.startDeg = st.startDeg - 40; s2.sweep = 230;
            s2.wStart = 16; s2.wMid = 9; s2.gapNum = 6; s2.splatNum = 5;
            stroke(g, s2, RED, 85);
        }

        var tf = g.textFrames.add();
        tf.contents = kanji;
        tf.textRange.characterAttributes.textFont = font(FONT);
        tf.textRange.characterAttributes.size = 120;
        tf.textRange.characterAttributes.fillColor = rgb(WHITE);
        var o = tf.createOutline();
        var b = o.geometricBounds;
        var sc = KANJI_H / Math.max(b[1] - b[3], b[2] - b[0]) * 100;
        o.resize(sc, sc);
        b = o.geometricBounds;
        o.translate(C - (b[0] + b[2]) / 2, C - (b[1] + b[3]) / 2);
        return g;
    }

    function exportPng(doc, file) {
        var o = new ExportOptionsPNG24();
        o.transparency = true;
        o.artBoardClipping = true;
        o.antiAliasing = true;
        doc.exportFile(file, ExportType.PNG24, o);
    }
    function background(doc, w, h) {
        var bg = doc.pathItems.rectangle(h, 0, w, h);
        bg.stroked = false;
        bg.fillColor = rgb(BG);
        bg.zOrder(ZOrderMethod.SENDTOBACK);
    }

    // ゲーム用
    var doc = app.documents.add(DocumentColorSpace.RGB, SIZE, SIZE);
    doc.artboards[0].artboardRect = [0, SIZE, SIZE, 0];
    for (var i = 0; i < ICONS.length; i++) {
        var g = makeIcon(doc, ICONS[i][0], 101 + i * 7919, GAME_STYLE);
        exportPng(doc, new File(gameDir.fsName + "/icon_" + ICONS[i][1] + ".png"));
        g.translate(SIZE * i, 0);
    }
    doc.artboards[0].artboardRect = [0, SIZE, SIZE * ICONS.length, 0];
    background(doc, SIZE * ICONS.length, SIZE);
    exportPng(doc, new File(outDir.fsName + "/_preview.png"));
    doc.saveAs(new File(outDir.fsName + "/result_icon.ai"));
    doc.close(SaveOptions.DONOTSAVECHANGES);

    // 筆の型の比べ: 1行が1つの型
    var H = SIZE * STYLE_ORDER.length;
    var cmp = app.documents.add(DocumentColorSpace.RGB, SIZE * ICONS.length, H);
    cmp.artboards[0].artboardRect = [0, H, SIZE * ICONS.length, 0];
    for (var r = 0; r < STYLE_ORDER.length; r++) {
        for (var i = 0; i < ICONS.length; i++) {
            var g = makeIcon(cmp, ICONS[i][0], 101 + i * 7919, STYLE_ORDER[r]);
            g.translate(SIZE * i, H - SIZE * (r + 1));
        }
    }
    background(cmp, SIZE * ICONS.length, H);
    exportPng(cmp, new File(outDir.fsName + "/_brushes.png"));
    cmp.close(SaveOptions.DONOTSAVECHANGES);
    return "ok";
})();
