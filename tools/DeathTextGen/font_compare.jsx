// 「死」の字の書体の比べ(Illustrator の ExtendScript)。MakeFontCompare.ps1 から実行する。
// 出力: out/_fonts.png(同じ赤・にじみで並べる。垂れ・飛沫はなし)
#target illustrator

(function () {
    var here = File($.fileName).parent;
    var outDir = new Folder(here.fsName + "/out");
    if (!outDir.exists) outDir.create();

    var FONTS = [
        ["KsoKokuryuSou", "KSO黒龍爽(今)"],
        ["SicYubi-HyojunGakushu", "Sic優美標準岳州"],
        ["TAwKazuhatai-Regular", "TAw-和羽体"],
        ["HGGyoshotai", "HG行書体"],
        ["HGSeikaishotaiPRO", "HG正楷書体-PRO"],
        ["HGKyokashotai", "HG教科書体"],
        ["UDDigiKyokashoN-Bold", "UDデジタル教科書体 Bold"],
        ["MatisseProN-M", "FOT-マティス ProN M"],
        ["HGMinchoE", "HG明朝E"],
        ["YuMincho-Demibold", "游明朝 Demibold"],
        ["HGSoeiPresenceEB", "HG創英プレゼンスEB"],
        ["HGSoeiKakugothicUB", "HG創英角ゴシックUB"]
    ];
    var COLS = 4, CW = 300, CH = 330, KANJI_H = 230;
    var ROWS = Math.ceil(FONTS.length / COLS), W = CW * COLS, H = CH * ROWS;
    var RED = [200, 24, 40], DARK_RED = [110, 8, 20], BG = [14, 8, 16], LABEL = [200, 194, 186];

    function rgb(c) { var o = new RGBColor(); o.red = c[0]; o.green = c[1]; o.blue = c[2]; return o; }
    function eachPath(item, fn) {
        var i;
        if (item.typename == "PathItem") fn(item);
        else if (item.typename == "CompoundPathItem") for (i = 0; i < item.pathItems.length; i++) fn(item.pathItems[i]);
        else if (item.typename == "GroupItem") for (i = 0; i < item.pageItems.length; i++) eachPath(item.pageItems[i], fn);
    }

    var doc = app.documents.add(DocumentColorSpace.RGB, W, H);
    doc.artboards[0].artboardRect = [0, H, W, 0];
    var bg = doc.pathItems.rectangle(H, 0, W, H);
    bg.stroked = false; bg.fillColor = rgb(BG);

    var missing = [];
    for (var n = 0; n < FONTS.length; n++) {
        var col = n % COLS, row = Math.floor(n / COLS);
        var cx = CW * col + CW / 2, cy = H - CH * row - 140;
        var f;
        try { f = app.textFonts.getByName(FONTS[n][0]); } catch (e) { missing.push(FONTS[n][1]); continue; }

        var tf = doc.textFrames.add();
        tf.contents = "死";
        tf.textRange.characterAttributes.textFont = f;
        tf.textRange.characterAttributes.size = 200;
        tf.textRange.characterAttributes.fillColor = rgb(RED);
        var glyph = tf.createOutline();
        var b = glyph.geometricBounds;
        var sc = KANJI_H / Math.max(b[1] - b[3], (b[2] - b[0]) * 0.9) * 100;
        glyph.resize(sc, sc);
        b = glyph.geometricBounds;
        glyph.translate(cx - (b[0] + b[2]) / 2, cy - (b[1] + b[3]) / 2);

        var halo = glyph.duplicate(glyph, ElementPlacement.PLACEAFTER);
        eachPath(halo, function (p) {
            p.filled = true; p.fillColor = rgb(DARK_RED);
            p.stroked = true; p.strokeColor = rgb(DARK_RED); p.strokeWidth = 5; p.strokeJoin = StrokeJoin.ROUNDENDJOIN;
        });
        halo.opacity = 32;

        var lb = doc.textFrames.add();
        lb.contents = (n + 1) + ". " + FONTS[n][1];
        lb.textRange.characterAttributes.textFont = app.textFonts.getByName("YuGothic-Medium");
        lb.textRange.characterAttributes.size = 16;
        lb.textRange.characterAttributes.fillColor = rgb(LABEL);
        lb.paragraphs[0].paragraphAttributes.justification = Justification.CENTER;
        lb.position = [cx - lb.width / 2, H - CH * row - CH + 36];
    }

    var o = new ExportOptionsPNG24();
    o.transparency = false; o.artBoardClipping = true; o.antiAliasing = true;
    doc.exportFile(new File(outDir.fsName + "/_fonts.png"), ExportType.PNG24, o);
    doc.close(SaveOptions.DONOTSAVECHANGES);
    return "ok missing=" + missing.join(",");
})();
