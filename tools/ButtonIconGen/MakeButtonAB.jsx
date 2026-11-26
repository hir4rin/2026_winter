// X・Y と同じ形(黒い外輪・赤い円・内側の細い線・黒い文字)で A・B を作る
#target photoshop
app.preferences.rulerUnits = Units.PIXELS;
app.displayDialogs = DialogModes.NO;

var OUT_DIR = $.getenv("BTN_OUT") || "C:/temp";
var SIZE = 128;

function rgb(r, g, b) { var c = new SolidColor(); c.rgb.red = r; c.rgb.green = g; c.rgb.blue = b; return c; }

function fillCircle(doc, cx, cy, r, color) {
    doc.selection.select([[cx - r, cy - r], [cx + r, cy - r], [cx + r, cy + r], [cx - r, cy + r]], SelectionType.REPLACE, 0, true);
    // 四角を丸にする
    var d = new ActionDescriptor(); var ref = new ActionReference();
    ref.putProperty(charIDToTypeID("Chnl"), charIDToTypeID("fsel")); d.putReference(charIDToTypeID("null"), ref);
    var e = new ActionDescriptor();
    e.putUnitDouble(charIDToTypeID("Top "), charIDToTypeID("#Pxl"), cy - r);
    e.putUnitDouble(charIDToTypeID("Left"), charIDToTypeID("#Pxl"), cx - r);
    e.putUnitDouble(charIDToTypeID("Btom"), charIDToTypeID("#Pxl"), cy + r);
    e.putUnitDouble(charIDToTypeID("Rght"), charIDToTypeID("#Pxl"), cx + r);
    d.putObject(charIDToTypeID("T   "), charIDToTypeID("Elps"), e);
    d.putBoolean(charIDToTypeID("AntA"), true);
    executeAction(charIDToTypeID("setd"), d, DialogModes.NO);
    doc.selection.fill(color);
    doc.selection.deselect();
}

function newLayer(doc, name) { var l = doc.artLayers.add(); l.name = name; return l; }

// dx, dy: 押す方向(A=下、B=右)。その側だけ外の帯が赤く、反対側は黒が太い
function makeButton(letter, dx, dy) {
    var doc = app.documents.add(SIZE, SIZE, 72, "Button_" + letter, NewDocumentMode.RGB, DocumentFill.TRANSPARENT);
    var c = 64;

    newLayer(doc, "outer");    fillCircle(doc, c, c, 58, rgb(0, 0, 0));
    newLayer(doc, "band");     fillCircle(doc, c + dx * 9, c + dy * 9, 45, rgb(255, 75, 75));
    newLayer(doc, "line");     fillCircle(doc, c + dx * 6, c + dy * 6, 42, rgb(120, 22, 22));
    newLayer(doc, "inner");    fillCircle(doc, c + dx * 4, c + dy * 4, 40, rgb(255, 75, 75));

    var t = doc.artLayers.add(); t.kind = LayerKind.TEXT; t.name = "letter";
    t.textItem.contents = letter;
    t.textItem.font = "Arial-BoldMT";
    t.textItem.size = new UnitValue(52, "px");
    t.textItem.color = rgb(0, 0, 0);
    t.textItem.justification = Justification.CENTER;
    t.textItem.position = [c, c + 22];
    // 文字の見た目の中心を円の中心にそろえる
    var b = t.bounds;
    var w = b[2].as("px") - b[0].as("px"), h = b[3].as("px") - b[1].as("px");
    t.translate(c + dx * 4 - (b[0].as("px") + w / 2), c + dy * 4 - (b[1].as("px") + h / 2));

    var psd = new File(OUT_DIR + "/" + letter + ".psd");
    doc.saveAs(psd, new PhotoshopSaveOptions(), true);
    var png = new PNGSaveOptions(); png.compression = 9;
    doc.saveAs(new File(OUT_DIR + "/" + letter + ".png"), png, true);
    doc.close(SaveOptions.DONOTSAVECHANGES);
}

makeButton("A", 0, 1);
makeButton("B", 1, 0);
"ok";
