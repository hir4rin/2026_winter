// タイトルロゴの案を作る(Illustrator の ExtendScript)
// 筆の線を1画ずつ組む文字(GLYPHS)と、フォントを輪郭にして加工する文字を組み合わせる
// MakeTitleLogo.ps1 から実行する。出力: このフォルダの out/<名前>.png(透過) / <名前>_preview.png(暗い背景) / <名前>.ai
#target illustrator

(function () {
    var here = File($.fileName).parent;
    var outDir = new Folder(here.fsName + "/out");
    if (!outDir.exists) outDir.create();
    var gameDir = new Folder(here.parent.parent.fsName + "/2026_winter/data/UI/Title");
    var GAME_MARGIN = 40;            // ゲーム用は光・押し出しが切れないよう少し余白を残す
    var ONLY = ["Shinobu_vertical"];   // 空にすると全部の案を作る

    var SHINOBU = "忍", MONO = "者";   // 忍 / 者

    // 筆の字形: 高さ100、y は下向き。線 = [点の列, 太さ, 入り, 抜き]
    // 入り: "b" 筆を置いた角 / "s" 細く入る。抜き: "s" 鋭く抜く / "f" かすれて抜く / "b" 止め
    var GLYPHS = {
        N: { adv: 66, s: [
            [[[11, -2], [10, 40], [8, 100], [6, 114]], 15, "b", "s"],
            [[[11, 0], [30, 46], [55, 100]], 17, "b", "f"],
            [[[55, 104], [57, 50], [61, -16]], 13, "b", "s"]] },
        I: { adv: 24, s: [
            [[[13, -6], [11, 50], [9, 110]], 16, "b", "f"]] },
        J: { adv: 52, s: [
            [[[12, 2], [34, -1], [60, -5]], 9, "b", "s"],
            [[[42, -3], [42, 50], [38, 84], [24, 100], [8, 97], [-4, 84]], 15, "b", "s"]] },
        A: { adv: 66, s: [
            [[[34, -10], [18, 48], [0, 108]], 14, "s", "s"],
            [[[33, -8], [48, 46], [64, 102]], 17, "b", "f"],
            [[[-16, 74], [30, 63], [88, 50]], 8, "s", "s"]] },
        S: { adv: 58, s: [
            [[[58, 8], [38, -2], [14, 4], [8, 22], [22, 40], [44, 54], [53, 74], [42, 94], [18, 101], [-6, 90]], 15, "b", "s"]] },
        H: { adv: 66, s: [
            [[[11, -4], [10, 50], [8, 108]], 15, "b", "f"],
            [[[56, -12], [55, 50], [53, 106]], 15, "b", "s"],
            [[[-8, 57], [30, 51], [78, 42]], 10, "s", "s"]] },
        O: { adv: 70, s: [
            [[[48, 2], [20, 6], [4, 42], [12, 86], [38, 101], [62, 86], [67, 40], [52, 8], [28, -3]], 15, "b", "s"]] },
        B: { adv: 60, s: [
            [[[11, -2], [10, 50], [8, 104]], 15, "b", "s"],
            [[[8, 1], [36, -1], [50, 15], [41, 39], [16, 48]], 12, "b", "s"],
            [[[15, 46], [45, 52], [57, 72], [45, 94], [17, 101], [2, 99]], 13, "b", "s"]] }
    };
    GLYPHS[SHINOBU] = { adv: 100, s: [
        [[[12, 13], [40, 9], [70, 5]], 10, "b", "b"],
        [[[67, 3], [72, 12], [71, 40], [66, 54], [56, 59], [47, 51]], 13, "b", "s"],
        [[[46, 8], [44, 28], [32, 48], [-8, 74]], 15, "b", "f"],
        [[[17, 25], [24, 32], [31, 40]], 13, "b", "s"],
        [[[5, 68], [10, 77], [15, 88]], 13, "b", "s"],
        [[[29, 57], [27, 80], [34, 95], [56, 99], [80, 93], [98, 66]], 15, "b", "s"],
        [[[44, 54], [50, 63], [56, 74]], 13, "b", "s"],
        [[[70, 54], [80, 64], [91, 79]], 13, "b", "s"]] };

    var RED = [196, 18, 32], DEEP_RED = [120, 6, 14], WHITE = [238, 236, 232], BG = [18, 20, 26];
    var STEEL = [[0, [52, 58, 66]], [47, [206, 212, 220]], [53, [78, 86, 96]], [100, [246, 248, 250]]];
    var GOLD = [[0, [96, 62, 14]], [46, [244, 210, 120]], [54, [132, 88, 22]], [100, [255, 242, 196]]];
    var BLOOD = [[0, [96, 4, 10]], [100, [222, 26, 38]]];
    var W = 1600, H = 800, FIT_W = 1360, FIT_H = 560;

    var seed = 11;
    function rand() {
        seed = (seed * 1103515245 + 12345) % 2147483648;
        return seed / 2147483648;
    }
    function rgb(a) {
        var c = new RGBColor();
        c.red = a[0]; c.green = a[1]; c.blue = a[2];
        return c;
    }
    var doc;
    function gradient(stops) {
        var g = doc.gradients.add();
        g.type = GradientType.LINEAR;
        while (g.gradientStops.length < stops.length) g.gradientStops.add();
        for (var i = 0; i < stops.length; i++) {
            g.gradientStops[i].rampPoint = stops[i][0];
            g.gradientStops[i].midPoint = 50;
            g.gradientStops[i].color = rgb(stops[i][1]);
            if (stops[i].length > 2) g.gradientStops[i].opacity = stops[i][2];
        }
        var gc = new GradientColor();
        gc.gradient = g;
        return gc;
    }

    // ---- 筆の線 ----
    function smooth(pts, n) {
        var out = [];
        var seg = pts.length - 1;
        for (var i = 0; i <= n; i++) {
            var f = i / n * seg;
            var k = Math.min(seg - 1, Math.floor(f));
            var t = f - k;
            var p0 = pts[Math.max(0, k - 1)], p1 = pts[k], p2 = pts[k + 1], p3 = pts[Math.min(seg, k + 2)];
            var t2 = t * t, t3 = t2 * t;
            out.push([
                0.5 * (2 * p1[0] + (-p0[0] + p2[0]) * t + (2 * p0[0] - 5 * p1[0] + 4 * p2[0] - p3[0]) * t2 + (-p0[0] + 3 * p1[0] - 3 * p2[0] + p3[0]) * t3),
                0.5 * (2 * p1[1] + (-p0[1] + p2[1]) * t + (2 * p0[1] - 5 * p1[1] + 4 * p2[1] - p3[1]) * t2 + (-p0[1] + 3 * p1[1] - 3 * p2[1] + p3[1]) * t3)
            ]);
        }
        return out;
    }
    function noise(n, amp) {
        var v = [], s = 0;
        for (var i = 0; i <= n; i++) { s = s * 0.45 + (rand() - 0.5); v.push(s * amp); }
        return v;
    }
    function width(t, head, tail) {
        var w = 1;
        if (head == "s") w *= Math.pow(Math.min(1, t / 0.22), 0.7);
        else w *= 0.85 + 0.3 * Math.exp(-Math.pow((t - 0.06) / 0.06, 2));
        var e = tail == "b" ? 0.12 : 0.4;
        if (t > 1 - e) {
            var u = (t - (1 - e)) / e;
            w *= tail == "b" ? Math.sqrt(Math.max(0, 1 - u * u)) * 0.6 + 0.4 * (1 - u) : Math.pow(1 - u, 0.9);
        }
        return w;
    }
    // 1画の輪郭と、かすれの穴(点の列の配列。最初が外側)
    function strokeShapes(def, st) {
        var head = def[2], tail = st.sharp ? "s" : def[3];
        if (st.sharp) head = "s";
        var W0 = def[1] * st.weight;
        var n = 60;
        var c = smooth(def[0], n);
        var nl = noise(n, st.rough * W0 * 0.025), nr = noise(n, st.rough * W0 * 0.06);
        var left = [], right = [], ws = [], nx = [], ny = [];
        for (var i = 0; i <= n; i++) {
            var a = c[Math.max(0, i - 1)], b = c[Math.min(n, i + 1)];
            var dx = b[0] - a[0], dy = b[1] - a[1], l = Math.sqrt(dx * dx + dy * dy) || 1;
            nx.push(-dy / l); ny.push(dx / l);
            var w = W0 * width(i / n, head, tail) * 0.5;
            ws.push(w);
            left.push([c[i][0] + nx[i] * (w + nl[i] * Math.min(1, w)), c[i][1] + ny[i] * (w + nl[i] * Math.min(1, w))]);
            right.push([c[i][0] - nx[i] * (w + nr[i] * Math.min(1, w)), c[i][1] - ny[i] * (w + nr[i] * Math.min(1, w))]);
        }
        var outline = left.slice(0);
        for (var j = n; j >= 0; j--) outline.push(right[j]);
        if (head == "b") {
            var d0x = c[1][0] - c[0][0], d0y = c[1][1] - c[0][1], l0 = Math.sqrt(d0x * d0x + d0y * d0y) || 1;
            outline.push([c[0][0] - d0x / l0 * ws[0] * 0.5 + nx[0] * ws[0] * 0.5, c[0][1] - d0y / l0 * ws[0] * 0.5 + ny[0] * ws[0] * 0.5]);
        }
        var shapes = [outline];
        if (tail == "f" && st.rough > 0) {
            for (var h = 0; h < 3; h++) {
                var off = (rand() - 0.5) * 0.9, t0 = 0.45 + rand() * 0.25, t1 = 0.93 + rand() * 0.06, th = 0.08 + rand() * 0.1;
                var hl = [], hr = [];
                for (var k = Math.ceil(t0 * n); k <= Math.floor(t1 * n); k++) {
                    var u = Math.max(0, Math.min(1, (k / n - t0) / (t1 - t0)));
                    var hw = ws[k] * th * Math.sin(Math.PI * Math.pow(u, 0.6));
                    var o = ws[k] * off;
                    hl.push([c[k][0] + nx[k] * (o + hw), c[k][1] + ny[k] * (o + hw)]);
                    hr.unshift([c[k][0] + nx[k] * (o - hw), c[k][1] + ny[k] * (o - hw)]);
                }
                shapes.push(hl.concat(hr));
            }
        }
        return shapes;
    }
    // 筆の文字列。size は高さ(pt)
    function brush(parent, text, size, st) {
        var g = parent.groupItems.add();
        var scale = size / 100, x = 0;
        for (var i = 0; i < text.length; i++) {
            var gl = GLYPHS[text.charAt(i)];
            for (var s = 0; s < gl.s.length; s++) {
                var shapes = strokeShapes(gl.s[s], st);
                var cp = g.compoundPathItems.add();
                for (var k = 0; k < shapes.length; k++) {
                    var a = [];
                    for (var m = 0; m < shapes[k].length; m++) {
                        var px = shapes[k][m][0] + x + (100 - shapes[k][m][1]) * st.slant;
                        a.push([px * scale, -shapes[k][m][1] * scale]);
                    }
                    var p = cp.pathItems.add();
                    p.setEntirePath(a);
                    p.closed = true;
                    p.stroked = false;
                    p.filled = true;
                    p.fillColor = rgb(st.color);
                    p.polarity = k == 0 ? PolarityValues.POSITIVE : PolarityValues.NEGATIVE;
                }
                if (st.spray && gl.s[s][3] != "b" && rand() < st.spray) {
                    var pts = gl.s[s][0], e = pts[pts.length - 1], f = pts[pts.length - 2];
                    var drops = spray(e, f, gl.s[s][1] * st.weight);
                    for (var d = 0; d < drops.length; d++) {
                        var dp = [];
                        for (var q = 0; q < drops[d].length; q++)
                            dp.push([(drops[d][q][0] + x + (100 - drops[d][q][1]) * st.slant) * scale, -drops[d][q][1] * scale]);
                        var dpi = g.pathItems.add();
                        dpi.setEntirePath(dp);
                        dpi.closed = true;
                        dpi.stroked = false;
                        dpi.fillColor = rgb(st.color);
                    }
                }
            }
            x += gl.adv + st.track;
        }
        return g;
    }
    // 払いの先に飛ぶ墨(線の向きに細長いしずく)
    function spray(e, f, w) {
        var dx = e[0] - f[0], dy = e[1] - f[1], l = Math.sqrt(dx * dx + dy * dy) || 1;
        dx /= l; dy /= l;
        var nx = -dy, ny = dx, out = [], dist = 4 + rand() * 4;
        var n = 1 + Math.floor(rand() * 3);
        for (var i = 0; i < n; i++) {
            var len = w * (0.25 + rand() * 0.45), wid = w * (0.05 + rand() * 0.08);
            var side = (rand() - 0.5) * w * 0.5;
            var cx = e[0] + dx * (dist + len / 2) + nx * side, cy = e[1] + dy * (dist + len / 2) + ny * side;
            var pts = [];
            for (var k = 0; k < 12; k++) {
                var a = k / 12 * Math.PI * 2, ca = Math.cos(a), sa = Math.sin(a);
                var r = wid * (ca > 0 ? 1 - ca * 0.85 : 1);   // 進む側が細い
                pts.push([cx + dx * ca * len / 2 + nx * sa * r, cy + dy * ca * len / 2 + ny * sa * r]);
            }
            out.push(pts);
            dist += len + w * (0.2 + rand() * 0.5);
        }
        return out;
    }
    // 好きな形の1画(点は文書の座標、y は上が大きい)
    function strokeAt(parent, pts, w, head, tail, color, rough) {
        var shapes = strokeShapes([pts, w, head, tail], { weight: 1, rough: rough || 0, slant: 0 });
        var cp = parent.compoundPathItems.add();
        for (var k = 0; k < shapes.length; k++) {
            var p = cp.pathItems.add();
            p.setEntirePath(shapes[k]);
            p.closed = true;
            p.stroked = false;
            p.fillColor = rgb(color);
            p.polarity = k == 0 ? PolarityValues.POSITIVE : PolarityValues.NEGATIVE;
        }
        return cp;
    }

    // ---- フォントの文字 ----
    function font(name, fallback) {
        try { return app.textFonts.getByName(name); } catch (e) { return app.textFonts.getByName(fallback); }
    }
    function type(parent, text, fontName, size, tracking, fallback) {
        var tf = parent.textFrames.add();
        tf.contents = text;
        var a = tf.textRange.characterAttributes;
        a.textFont = font(fontName, fallback || "YuGothic-Bold");
        a.size = size;
        a.tracking = tracking || 0;
        return tf.createOutline();
    }
    // 縦書き(1文字ずつ下へ)
    function typeV(parent, text, fontName, size, gap) {
        var g = parent.groupItems.add(), prev = null;
        for (var i = 0; i < text.length; i++) {
            var t = type(g, text.charAt(i), fontName, size);
            if (prev) {
                var pb = prev.geometricBounds, tb = t.geometricBounds;
                t.translate((pb[0] + pb[2]) / 2 - (tb[0] + tb[2]) / 2, pb[3] - gap - tb[1]);
            }
            prev = t;
        }
        return g;
    }
    // 塗り: fill は [r,g,b] か グラデーションの段(下→上)、null で塗りなし
    function paint(item, fill, stroke, sw) {
        if (item.typename == "GroupItem") {
            for (var i = 0; i < item.pageItems.length; i++) paint(item.pageItems[i], fill, stroke, sw);
        } else if (item.typename == "CompoundPathItem") {
            for (var j = 0; j < item.pathItems.length; j++) paint(item.pathItems[j], fill, stroke, sw);
        } else if (item.typename == "PathItem" && !item.clipping) {
            item.filled = fill != null;
            if (fill != null) {
                if (fill[0].length) {
                    item.fillColor = gradient(fill);
                    item.rotate(90, false, false, true, false, Transformation.CENTER);
                } else item.fillColor = rgb(fill);
            }
            item.stroked = stroke != null;
            if (stroke != null) {
                item.strokeColor = rgb(stroke);
                item.strokeWidth = sw;
                item.strokeJoin = StrokeJoin.MITERENDJOIN;
            }
        }
    }
    // 子を左から並べる(1文字ずつ色を変えるため)
    function lettersByX(g) {
        var a = [];
        for (var i = 0; i < g.pageItems.length; i++) a.push(g.pageItems[i]);
        a.sort(function (p, q) { return p.geometricBounds[0] - q.geometricBounds[0]; });
        return a;
    }

    // ---- 配置 ----
    function bounds(it) { return it.geometricBounds; }   // [左, 上, 右, 下]、y は上が大きい
    function centerOf(it) { var b = bounds(it); return [(b[0] + b[2]) / 2, (b[1] + b[3]) / 2]; }
    function moveTo(it, x, y) { var c = centerOf(it); it.translate(x - c[0], y - c[1]); }
    function below(it, ref, gap) {
        var rb = bounds(ref), b = bounds(it);
        it.translate((rb[0] + rb[2]) / 2 - (b[0] + b[2]) / 2, rb[3] - gap - b[1]);
    }
    function rightOf(it, ref, gap, alignTop) {
        var rb = bounds(ref), b = bounds(it);
        var dy = alignTop ? rb[1] - b[1] : (rb[1] + rb[3]) / 2 - (b[1] + b[3]) / 2;
        it.translate(rb[2] + gap - b[0], dy);
    }

    // ---- 切る ----
    // 中心 c を通る角度 deg の線に沿った帯(線からの距離 a〜b)
    function strip(c, deg, a, b) { return { c: c, deg: deg, a: a, b: b }; }
    // 帯の形。マスクの大きさがグループの大きさになるので、文字より少し大きいだけにする
    function stripPoints(s, ib) {
        var r = s.deg * Math.PI / 180, dx = Math.cos(r), dy = Math.sin(r), nx = -dy, ny = dx, c = s.c, m = 4;
        var d0 = 1e9, d1 = -1e9, n0 = 1e9, n1 = -1e9;
        var corners = [[ib[0], ib[1]], [ib[2], ib[1]], [ib[2], ib[3]], [ib[0], ib[3]]];
        for (var i = 0; i < 4; i++) {
            var px = corners[i][0] - c[0], py = corners[i][1] - c[1];
            var pd = px * dx + py * dy, pn = px * nx + py * ny;
            d0 = Math.min(d0, pd); d1 = Math.max(d1, pd); n0 = Math.min(n0, pn); n1 = Math.max(n1, pn);
        }
        d0 -= m; d1 += m;
        var a = Math.max(n0 - m, Math.min(n1 + m, s.a)), b = Math.max(n0 - m, Math.min(n1 + m, s.b));
        var poly = [
            [c[0] + dx * d0 + nx * a, c[1] + dy * d0 + ny * a], [c[0] + dx * d1 + nx * a, c[1] + dy * d1 + ny * a],
            [c[0] + dx * d1 + nx * b, c[1] + dy * d1 + ny * b], [c[0] + dx * d0 + nx * b, c[1] + dy * d0 + ny * b]];
        // 文字の外枠の四角で切る(Sutherland-Hodgman)
        var edges = [[0, ib[0] - m, 1], [0, ib[2] + m, -1], [1, ib[3] - m, 1], [1, ib[1] + m, -1]];
        for (var e = 0; e < 4; e++) {
            var ax = edges[e][0], lim = edges[e][1], sg = edges[e][2], out = [];
            for (var j = 0; j < poly.length; j++) {
                var p = poly[j], q = poly[(j + 1) % poly.length];
                var pin = (p[ax] - lim) * sg >= 0, qin = (q[ax] - lim) * sg >= 0;
                if (pin) out.push(p);
                if (pin != qin) {
                    var t = (lim - p[ax]) / (q[ax] - p[ax]);
                    out.push([p[0] + (q[0] - p[0]) * t, p[1] + (q[1] - p[1]) * t]);
                }
            }
            poly = out;
        }
        return poly;
    }
    // 帯の部分だけ残す(マスクは1本のパスしか使えないので、帯ごとに複製して切り抜く)
    function clipTo(item, polys) {
        var ib = bounds(item);
        var all = item.parent.groupItems.add();
        for (var i = 0; i < polys.length; i++) {
            var h = all.groupItems.add();
            item.duplicate(h, ElementPlacement.PLACEATEND);
            var p = h.pathItems.add();
            p.setEntirePath(stripPoints(polys[i], ib));
            p.closed = true;
            p.move(h, ElementPlacement.PLACEATBEGINNING);
            h.clipped = true;
        }
        item.remove();
        return all;
    }
    // 斜めに斬って、上側を線に沿ってずらす。上下で色を変えられる
    function slash(item, deg, gap, shift, upperFill, lowerFill, at) {
        var c = at || centerOf(item);
        var lo = item.duplicate();
        if (upperFill) paint(item, upperFill, null, 0);
        if (lowerFill) paint(lo, lowerFill, null, 0);
        var up = clipTo(item, [strip(c, deg, gap / 2, 3000)]);
        var dn = clipTo(lo, [strip(c, deg, -3000, -gap / 2)]);
        var r = deg * Math.PI / 180;
        up.translate(-Math.cos(r) * shift, -Math.sin(r) * shift);
        var g = up.parent.groupItems.add();
        dn.move(g, ElementPlacement.PLACEATEND);
        up.move(g, ElementPlacement.PLACEATBEGINNING);
        return g;
    }

    // ---- 重ねる効果 ----
    function blur(item, r) {
        item.applyEffect('<LiveEffect name="Adobe PSL Gaussian Blur"><Dict data="R blur ' + r + ' "/></LiveEffect>');
    }
    // 本体の後ろに重ねる。奥から 光 → 押し出し → ふち → 面取り(明・暗) → 本体。まとめたグループを返す
    function fx(item, o) {
        var g = item.parent.groupItems.add();
        item.move(g, ElementPlacement.PLACEATEND);
        function behind(fill, stroke, sw, dx, dy) {
            var d = item.duplicate();
            paint(d, fill, stroke, sw);
            d.translate(dx || 0, dy || 0);
            d.move(item, ElementPlacement.PLACEAFTER);
            return d;
        }
        if (o.glow) {
            var gl = behind(o.glow[0], o.glow[0], o.glow[1]);
            blur(gl, o.glow[2]);
            gl.opacity = o.glow[3];
        }
        if (o.shadow) {
            var sh = behind([0, 0, 0], [0, 0, 0], o.shadow[2], o.shadow[0], o.shadow[1]);
            blur(sh, o.shadow[3]);
            sh.opacity = o.shadow[4];
        }
        if (o.extrude) {
            for (var i = o.extrude[2]; i >= 1; i--)
                behind(o.extrude[3], o.extrude[3], o.outline ? o.outline[1] : 0.01, o.extrude[0] * i, o.extrude[1] * i);
        }
        if (o.rim) behind(o.rim[0], o.rim[0], o.rim[1]);
        if (o.outline) behind(o.outline[0], o.outline[0], o.outline[1]);
        if (o.emboss) {
            behind(o.emboss[1], null, 0, o.emboss[2], -o.emboss[2]);
            behind(o.emboss[0], null, 0, -o.emboss[2], o.emboss[2]);
        }
        return g;
    }
    function group(parent, items) {
        var g = parent.groupItems.add();
        for (var i = 0; i < items.length; i++) items[i].move(g, ElementPlacement.PLACEATEND);
        return g;
    }
    function polyAt(parent, pts, color) {
        var p = parent.pathItems.add();
        p.setEntirePath(pts);
        p.closed = true;
        p.stroked = false;
        p.fillColor = rgb(color);
        return p;
    }
    function sizeOf(it) { var b = bounds(it); return [b[2] - b[0], b[1] - b[3]]; }
    // 斬った線の光(slash と同じ中心・角度で)
    function cutLine(parent, c, deg, half, w) {
        var r = deg * Math.PI / 180, dx = Math.cos(r), dy = Math.sin(r);
        var line = strokeAt(parent, [[c[0] - dx * half, c[1] - dy * half], [c[0], c[1]], [c[0] + dx * half, c[1] + dy * half]], w, "s", "s", [255, 236, 224]);
        return fx(line, { glow: [[255, 30, 40], w * 1.5, 16, 90] });
    }

    var BLOOD_V = [[0, [70, 0, 6]], [55, [176, 10, 24]], [100, [236, 40, 48]]];
    var GOLD_RICH = [[0, [84, 50, 10]], [30, [196, 142, 48]], [46, [255, 228, 150]], [52, [150, 98, 26]], [70, [226, 178, 86]], [100, [255, 246, 210]]];
    var STEEL_RICH = [[0, [40, 44, 52]], [28, [150, 158, 170]], [47, [236, 240, 246]], [52, [70, 76, 88]], [74, [176, 184, 196]], [100, [255, 255, 255]]];
    var WHITE_V = [[0, [170, 168, 166]], [100, [255, 255, 255]]];
    var BLACK = [8, 8, 10];

    // 斬った文字: 上が白・下が赤、押し出しと斬り口の光
    function slicedTitle(L, word, deg) {
        var t = type(L, word, "AcuminConcept-WideBlackItalic", 240, -10);
        var c = centerOf(t);
        var s = slash(t, deg, 14, 30, WHITE_V, BLOOD_V);
        fx(s, { extrude: [1.2, -1.6, 8, [26, 28, 34]], outline: [BLACK, 4] });
        var sb = bounds(s);
        cutLine(L, c, deg, (sb[2] - sb[0]) / 2 + 40, 4);
    }

    // ---- 案 ----
    var VARIANTS = [
        { name: "R3_02_NINJA_brush", build: function (L) {
            var n = brush(L, "NINJA", 300, { color: RED, weight: 1.1, rough: 1, slant: 0.22, track: 4, spray: 0.8 });
            paint(n, BLOOD_V, null, 0);
            var whole = fx(n, { glow: [[150, 0, 10], 8, 40, 45], extrude: [1.4, -1.4, 7, [52, 0, 4]], outline: [BLACK, 5] });
            var cut = slash(whole, 7, 14, 40, null, null);
            var sb = bounds(cut);
            var k = type(L, SHINOBU, "KsoKokuryuSou", 170, 0);
            paint(k, WHITE, null, 0);
            k.rotate(-8);
            var kb = bounds(k);
            k.translate(sb[0] + 10 - kb[2], sb[1] + 30 - kb[1]);
            fx(k, { glow: [[220, 20, 30], 6, 26, 85], outline: [BLACK, 6] });
        } },
        { name: "R3_03_Ninja_gold", build: function (L) {
            var k = type(L, SHINOBU + MONO, "KsoKokuryuSou", 320, -60);
            paint(k, GOLD_RICH, null, 0);
            var c = centerOf(k), s = sizeOf(k), r = s[1] * 0.62, pts = [];
            k = fx(k, { shadow: [6, -10, 4, 14, 70], outline: [[22, 12, 4], 8], emboss: [[255, 248, 220], [70, 40, 8], 2] });
            for (var i = 0; i <= 9; i++) {
                var a = (200 - i * 36) * Math.PI / 180;
                pts.push([c[0] + Math.cos(a) * r * 1.25, c[1] + Math.sin(a) * r]);
            }
            var ring = strokeAt(L, pts, s[1] * 0.16, "b", "f", [110, 8, 16], 1.2);
            paint(ring, BLOOD_V, null, 0);
            ring.opacity = 85;
            ring.zOrder(ZOrderMethod.SENDTOBACK);
            var t = type(L, "NINJA", "PerpetuaTitlingMT-Bold", 54, 1400);
            paint(t, GOLD_RICH, null, 0);
            below(t, k, 34);
            fx(t, { outline: [[22, 12, 4], 3] });
            var tb = bounds(t), ty = (tb[1] + tb[3]) / 2;
            strokeAt(L, [[tb[0] - 260, ty], [tb[0] - 120, ty + 1], [tb[0] - 30, ty]], 5, "s", "b", [214, 170, 80]);
            strokeAt(L, [[tb[2] + 30, ty], [tb[2] + 120, ty + 1], [tb[2] + 260, ty]], 5, "b", "s", [214, 170, 80]);
        } },
        { name: "R3_04_SHINOBI_serif", build: function (L) {
            var k = type(L, SHINOBU, "NotoSerifJP-Heavy", 760, 0);
            paint(k, [[0, [255, 255, 255], 0], [100, [255, 255, 255], 16]], [255, 255, 255], 1.2);
            k.opacity = 70;
            var t = type(L, "SHINOBI", "BodoniMT", 200, 240);
            paint(t, WHITE_V, null, 0);
            var letters = lettersByX(t);
            paint(letters[4], [[0, [120, 4, 14]], [100, [230, 30, 40]]], null, 0);
            moveTo(t, centerOf(k)[0], centerOf(k)[1]);
            fx(t, { shadow: [0, -6, 2, 16, 80] });
            var tb = bounds(t), w = tb[2] - tb[0], h = tb[1] - tb[3];
            strokeAt(L, [[tb[0] - w * 0.12, tb[1] + h * 0.22], [tb[0] + w * 0.5, tb[1] + h * 0.24], [tb[2] + w * 0.02, tb[1] + h * 0.22]], 2.2, "s", "s", WHITE);
            strokeAt(L, [[tb[0] - w * 0.02, tb[3] - h * 0.24], [tb[0] + w * 0.5, tb[3] - h * 0.26], [tb[2] + w * 0.12, tb[3] - h * 0.24]], 2.2, "s", "s", WHITE);
            var ob = bounds(letters[4]);
            var sl = strokeAt(L, [[ob[0] - 50, ob[3] - 40], [(ob[0] + ob[2]) / 2, (ob[1] + ob[3]) / 2], [ob[2] + 50, ob[1] + 40]], 3, "s", "s", [255, 60, 60]);
            fx(sl, { glow: [[255, 30, 30], 4, 10, 80] });
            var kata = typeV(L, "シノビ", "NotoSerifJP-SemiBold", 64, 10);
            paint(kata, RED, null, 0);
            var kb = bounds(kata);
            kata.translate(tb[2] + w * 0.05 - kb[0], tb[1] + h * 0.15 - kb[1]);
        } },
        // 本命(05 の斬った文字)。game を書くと data/UI/Title/ に余白を詰めて書き出す
        { name: "Title_NINJA", game: "logo_NINJA.png", build: function (L) { slicedTitle(L, "NINJA", 10); } },
        { name: "Title_SHINOBI", game: "logo_SHINOBI.png", build: function (L) { slicedTitle(L, "SHINOBI", 6); } },
        { name: "R3_09_NINJA_steel", build: function (L) {
            var t = type(L, "NINJA", "TrajanColor-Concept", 240, 60, "PerpetuaTitlingMT-Bold");
            paint(t, STEEL_RICH, null, 0);
            fx(t, { shadow: [4, -10, 6, 12, 80], rim: [[170, 12, 22], 22], outline: [BLACK, 12], emboss: [[255, 255, 255], [18, 20, 26], 2] });
            var k = type(L, SHINOBU, "KsoKokuryuSou", 620, 0);
            paint(k, [[0, [60, 0, 6]], [100, [150, 8, 20]]], null, 0);
            moveTo(k, centerOf(t)[0], centerOf(t)[1] + 10);
            k.zOrder(ZOrderMethod.SENDTOBACK);
            k.opacity = 75;
            var g = k.duplicate();
            paint(g, [200, 10, 24], [200, 10, 24], 10);
            blur(g, 40);
            g.opacity = 35;
            g.zOrder(ZOrderMethod.SENDTOBACK);
        } },
        { name: "R3_10_Ninja_vertical", build: function (L) {
            var k = typeV(L, SHINOBU + MONO, "KsoKokuryuSou", 300, -30);
            paint(k, WHITE_V, null, 0);
            var kb = bounds(k), kw = kb[2] - kb[0], kh = kb[1] - kb[3];
            k = fx(k, { shadow: [6, -8, 4, 12, 70], outline: [BLACK, 7], emboss: [[255, 255, 255], [90, 88, 86], 1.5] });
            var cx = kb[2] + kw * 0.16;
            var band = strokeAt(L, [[cx + 10, kb[1] + kh * 0.08], [cx - 6, kb[1] - kh * 0.5], [cx + 4, kb[3] - kh * 0.1]], kw * 0.55, "b", "f", [180, 12, 26], 1.4);
            paint(band, [[0, [90, 0, 8]], [60, [186, 14, 28]], [100, [220, 30, 40]]], null, 0);
            band.zOrder(ZOrderMethod.SENDTOBACK);
            var t = type(L, "NINJA", "CopperplateGothic-Bold", 54, 900);
            paint(t, [24, 4, 6], null, 0);
            t.rotate(-90);
            moveTo(t, cx + kw * 0.02, (kb[1] + kb[3]) / 2);
            var hs = kw * 0.28;
            var seal = L.groupItems.add();
            var sq = seal.pathItems.roundedRectangle(kb[3] + hs * 0.2, kb[0] - hs * 0.55, hs, hs, hs * 0.12, hs * 0.12);
            sq.stroked = false;
            sq.fillColor = rgb([200, 20, 30]);
            var sk = type(seal, SHINOBU, "KsoKokuryuSou", hs * 0.95, 0);
            paint(sk, WHITE, null, 0);
            moveTo(sk, centerOf(sq)[0], centerOf(sq)[1]);
            seal.rotate(-6);
        } },
        // 決定版。10 の作り込み(忍 = 刃＋心)。上の刃は鋼、下の心は血の赤で、境目を斬る
        // layers: ゲームの演出用に 後ろ / 斬る前の鋼 / 斬る前の赤 / 斬り口の光 を同じ大きさで別々に書き出す
        { name: "Shinobu_vertical", game: "logo.png", layers: true, build: function (L) {
            var k = type(L, SHINOBU, "KsoKokuryuSou", 520, 0);
            var kb = bounds(k), w = kb[2] - kb[0], h = kb[1] - kb[3];
            // 斬り線 = 後ろの斜めの筆(左上→右下のまっすぐな線)。忍の真ん中の x で筆の上の点を取る
            var crossFrom = [kb[0] - w * 0.3, kb[1] - h * 0.05], crossTo = [kb[2] + w * 0.45, kb[3] - h * 0.02];
            var cdx = crossTo[0] - crossFrom[0], cdy = crossTo[1] - crossFrom[1];
            var cutDeg = Math.atan2(cdy, cdx) * 180 / Math.PI;
            var ct = ((kb[0] + kb[2]) / 2 - crossFrom[0]) / cdx;
            var at = [crossFrom[0] + cdx * ct, crossFrom[1] + cdy * ct];
            var look = { glow: [[170, 0, 12], 10, 40, 55], shadow: [8, -12, 6, 16, 75], outline: [BLACK, 9], emboss: [[255, 255, 255], [20, 18, 22], 2] };
            var ks = k.duplicate(), kr = k.duplicate();
            paint(ks, STEEL_RICH, null, 0);
            paint(kr, BLOOD_V, null, 0);
            var steel = fx(ks, look), red = fx(kr, look);
            steel.name = "steel"; red.name = "red";
            steel.hidden = true; red.hidden = true;
            var s = slash(k, cutDeg, 12, 22, STEEL_RICH, BLOOD_V, at);
            fx(s, look);
            var r = cutDeg * Math.PI / 180;
            var cut = cutLine(L, at, cutDeg, Math.max(w, h) * 0.5 / Math.max(Math.abs(Math.cos(r)), Math.abs(Math.sin(r))) + 20, 4);
            cut.name = "cut";
            // 斬り線の目印(縮小・移動の後の位置を読む)
            var mk = L.pathItems.add();
            mk.setEntirePath([at, [at[0] + Math.cos(r) * 100, at[1] + Math.sin(r) * 100]]);
            mk.filled = false; mk.stroked = false; mk.name = "cutMarker";

            var bx = kb[2] + w * 0.06;
            var bandPts = [[bx + 14, kb[1] + h * 0.14], [bx - 8, kb[1] - h * 0.45], [bx + 6, kb[3] - h * 0.18]];
            var band = strokeAt(L, bandPts, w * 0.4, "b", "f", [180, 12, 26], 1.4);
            paint(band, [[0, [80, 0, 8]], [55, [186, 14, 28]], [100, [226, 32, 42]]], null, 0);
            var drops = spray(bandPts[2], bandPts[1], w * 0.4);
            for (var i = 0; i < drops.length; i++) polyAt(L, drops[i], [150, 8, 20]);
            var crossPts = [crossFrom, at, crossTo];
            var cross = strokeAt(L, crossPts, h * 0.13, "b", "s", [86, 0, 10], 1.2);
            cross.opacity = 85;
            var cd = spray(crossPts[2], crossPts[1], h * 0.13);
            for (var j = 0; j < cd.length; j++) polyAt(L, cd[j], [86, 0, 10]).opacity = 85;
            band.zOrder(ZOrderMethod.SENDTOBACK);
            cross.zOrder(ZOrderMethod.SENDTOBACK);

            var t = type(L, "SHINOBI", "CopperplateGothic-Bold", 44, 700);
            paint(t, [26, 4, 6], null, 0);
            t.rotate(-90);
            moveTo(t, bx + w * 0.035, kb[1] - h * 0.4);

            // 忍より後に足した物(帯・交差の筆・しずく・文字)を「後ろ」にまとめる
            var back = L.groupItems.add();
            back.name = "back";
            var keep = ["steel", "red", "cut", "cutMarker", "back"], others = [];
            for (var q = 0; q < L.pageItems.length; q++) {
                var nm = L.pageItems[q].name, isKeep = false;
                for (var z = 0; z < keep.length; z++) if (nm == keep[z]) isKeep = true;
                if (!isKeep) others.push(L.pageItems[q]);
            }
            // 忍の完成形(斬った物)は others の中で一番手前にある。帯などは後ろ
            var finalKanji = s.parent;
            for (var o = 0; o < others.length; o++) {
                if (others[o] == finalKanji) { finalKanji.name = "final"; continue; }
                others[o].move(back, ElementPlacement.PLACEATEND);
            }
            back.move(L, ElementPlacement.PLACEATEND);
        } }
    ];

    function exportPng(file) {
        var o = new ExportOptionsPNG24();
        o.transparency = true;
        o.artBoardClipping = true;
        o.antiAliasing = true;
        doc.exportFile(file, ExportType.PNG24, o);
    }

    // 層ごとに logo_<名前>.png、斬り線を logo_cut.csv(画像のピクセル座標、y は下向き)に書く
    function exportLayers(logo, rect) {
        var names = ["back", "steel", "red", "cut"];
        for (var n = 0; n < names.length; n++) {
            for (var i = 0; i < logo.pageItems.length; i++) logo.pageItems[i].hidden = logo.pageItems[i].name != names[n];
            exportPng(new File(gameDir.fsName + "/logo_" + names[n] + ".png"));
        }
        var pts = logo.pageItems.getByName("cutMarker").pathPoints;
        var a = pts[0].anchor, b = pts[1].anchor;
        var x = a[0] - rect[0], y = rect[1] - a[1];
        var dx = b[0] - a[0], dy = -(b[1] - a[1]), l = Math.sqrt(dx * dx + dy * dy);
        var f = new File(gameDir.fsName + "/logo_cut.csv");
        f.encoding = "UTF-8";
        f.lineFeed = "Windows";
        f.open("w");
        f.writeln("x,y,dirX,dirY");
        f.writeln(x.toFixed(2) + "," + y.toFixed(2) + "," + (dx / l).toFixed(5) + "," + (dy / l).toFixed(5));
        f.close();
    }

    app.userInteractionLevel = UserInteractionLevel.DONTDISPLAYALERTS;
    var done = [];
    for (var v = 0; v < VARIANTS.length; v++) {
        var V = VARIANTS[v];
        if (ONLY.length && ("," + ONLY.join(",") + ",").indexOf("," + V.name + ",") < 0) continue;
        seed = 11 + v * 101;
        doc = app.documents.add(DocumentColorSpace.RGB, W, H);
        try {
            var ab = doc.artboards[0].artboardRect;
            var logo = doc.layers[0].groupItems.add();
            logo.name = V.name;
            V.build(logo);
            var b = logo.geometricBounds;
            var k = Math.min(FIT_W / (b[2] - b[0]), FIT_H / (b[1] - b[3])) * 100;
            logo.resize(k, k, true, true, true, true, k, Transformation.CENTER);
            moveTo(logo, (ab[0] + ab[2]) / 2, (ab[1] + ab[3]) / 2);

            exportPng(new File(outDir.fsName + "/" + V.name + ".png"));
            var bg = doc.layers[0].pathItems.rectangle(ab[1], ab[0], W, H);
            bg.stroked = false;
            bg.fillColor = rgb(BG);
            bg.zOrder(ZOrderMethod.SENDTOBACK);
            exportPng(new File(outDir.fsName + "/" + V.name + "_preview.png"));
            bg.remove();
            if (V.game) {
                var gb = logo.visibleBounds, m = GAME_MARGIN;
                doc.artboards[0].artboardRect = [gb[0] - m, gb[1] + m, gb[2] + m, gb[3] - m];
                if (!gameDir.exists) gameDir.create();
                exportPng(new File(gameDir.fsName + "/" + V.game));
                if (V.layers) exportLayers(logo, doc.artboards[0].artboardRect);
                doc.artboards[0].artboardRect = ab;
            }
            doc.saveAs(new File(outDir.fsName + "/" + V.name + ".ai"), new IllustratorSaveOptions());
            done.push(V.name);
        } catch (e) {
            done.push(V.name + " ERROR " + e.message + " line " + e.line);
        }
        doc.close(SaveOptions.DONOTSAVECHANGES);
    }
    return done.join("\n");
})();
