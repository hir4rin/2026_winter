# Ch0 0-1 DDO列車のステージを作る
# 使い方: blender -b --python build_train_stage.py -- 素材フォルダ 出力フォルダ 通路.blend
#   通路.blend は prepare_corridor.py で作ったもの(焼き込み済みの宇宙船の通路)
# 間取り図(進捗/忍者外伝4_Ch0-1_ステージ設計図.xlsx「Ch0 間取り図」0-1)の流れを元に、
# はしごの代わりにスロープ車をはさんで、前から最後尾へ自然に進めるようにした
# 座標: 組み立ては列車の前(機関車)が+Y。出力の直前に180°回して、ゲームでは前が-Z・プレイヤーは+Zへ進む
#       デッキの上面がz=0、レール上面がz=-1.5。原点はスタート位置(1両目の前寄り)
import bpy, bmesh, math, sys, os
import numpy as np
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
ASSETS, OUT, CORRIDOR_BLEND = argv[0], argv[1], argv[2]
TEX_DIR = os.path.join(OUT, 'tex')
os.makedirs(TEX_DIR, exist_ok=True)

FLAT_LEN = 20.0       # 平台車・スロープ車の長さ
CAR_HALF_W = 8.0      # 平台車の幅の半分
GAP = 2.0             # 連結部のすき間
DECK_Z = 0.0
RAIL_Z = -1.5         # レール上面
TUNNEL_SCALE = 11.0   # トンネルの拡大率(build_all.sh と同じ値)
TUNNEL_X_SCALE = 2.0  # トンネルの横幅だけさらに広げる倍率(build_all.sh と同じ値)
WHEEL_X = 0.24 * TUNNEL_SCALE * TUNNEL_X_SCALE   # 車輪の位置(トンネルのレールに合わせる)
LOCO_SCALE = 0.55     # 車両の幅に合わせて大きめにする
CORR_SCALE = (3.0, 1.0, 1.9)   # 通路の車両の拡大率(横・長さ・高さ)
GRID_HALF_W = 6.8     # 平台車の真ん中の格子の床の幅の半分
RAMP_HALF_W = 4.0     # スロープの幅の半分
RAMP_LEN = 16.0       # スロープの斜めの部分の長さ

# 色の明るさ(テクスチャに掛ける色。convert.exe flat で 拡散×0.1・環境×0.9 になる)
TINT_SHELL = (0.5, 0.5, 0.53)
TINT_FLOOR = (0.7, 0.7, 0.7)
TINT_HULL = (0.6, 0.6, 0.62)
TINT_CONTAINER = (0.95, 0.95, 0.95)
TINT_ROOF = (1.0, 1.0, 1.0)
OUTER_WALL_H = 12.0   # 列車の外へ出られないようにする見えない壁の高さ(デッキから)

def Y(u):
    return 1.5 - u

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

# ---------------- マテリアル ----------------
MATS = {}
def make_mat(name, color=(0.8, 0.8, 0.8), img=None, emit=None, tint=None):
    """imgがあるときはcolorではなくtint(テクスチャに掛ける色)を使う"""
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    c = tint if img else color
    p.inputs['Base Color'].default_value = (*c, 1)
    m.diffuse_color = (*c, 1)
    if img:
        t = m.node_tree.nodes.new('ShaderNodeTexImage')
        t.image = img
        m.node_tree.links.new(t.outputs['Color'], p.inputs['Base Color'])
    if emit:
        p.inputs['Emission Color'].default_value = (*emit, 1)
        p.inputs['Emission Strength'].default_value = 2.0
    MATS[name] = m
    return m

def load_image(name):
    return bpy.data.images.load(os.path.join(TEX_DIR, name), check_existing=True)

make_mat('grid floor', img=load_image('grid_floor.png'), tint=TINT_FLOOR)
make_mat('hull', img=load_image('hull.jpg'), tint=TINT_HULL)
make_mat('chassis', (0.08, 0.08, 0.09))
make_mat('wheel', (0.22, 0.22, 0.24))
make_mat('lamp red', (0.9, 0.1, 0.05), emit=(1.0, 0.1, 0.05))

def tex_hazard():
    """黄色と黒の斜めじま(床の切れ目に貼るテープ)"""
    y, x = np.mgrid[0:64, 0:64]
    s = ((x + y) // 16) % 2 == 0
    a = np.zeros((64, 64, 4), np.float32)
    a[..., 3] = 1.0
    a[s, :3] = (0.95, 0.75, 0.05)
    a[~s, :3] = (0.05, 0.05, 0.05)
    img = bpy.data.images.new('hazard', 64, 64)
    img.pixels.foreach_set(a.ravel())
    img.filepath_raw = os.path.join(TEX_DIR, 'hazard.png')
    img.file_format = 'PNG'
    img.save()
    img.filepath = img.filepath_raw
    return img

make_mat('hazard', img=tex_hazard(), tint=(1.0, 1.0, 1.0))

# ---------------- 形を作る ----------------
VIS = []     # 見た目のオブジェクト
COL = []     # 当たり判定の形(8頂点)
GROUPS = []  # Unity用: (名前, VISの開始位置, COLの開始位置)。車両ごとに分けて書き出す
CONTAINERS = set()   # Unity用: 1つずつ動かせるように別のオブジェクトのままにする物

def begin_group(name):
    GROUPS.append((name, len(VIS), len(COL)))

def corners(mn, mx):
    return [Vector((mx[0] if i & 1 else mn[0], mx[1] if i & 2 else mn[1], mx[2] if i & 4 else mn[2])) for i in range(8)]

def prism(c, mat, tile=None, col=True, vis=True, name='part'):
    """8頂点の六面体(箱やスロープ)。cの並びはcorners()と同じ。tile: テクスチャ1枚の大きさ(m)"""
    if col:
        COL.append([v.copy() for v in c])
    if not vis:
        return None
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    vs = [bm.verts.new(p) for p in c]
    for q in ((0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)):
        bm.faces.new([vs[i] for i in q])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        a, b = {0: (1, 2), 1: (0, 2), 2: (1, 0)}[ax]
        for l in f.loops:
            p = l.vert.co
            l[uv].uv = (p[a] / tile, p[b] / tile) if tile else (p[a], p[b])
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    me.materials.append(MATS[mat])
    ob = bpy.data.objects.new(name, me)
    scene.collection.objects.link(ob)
    VIS.append(ob)
    return ob

def ubox(u0, u1, x0, x1, z0, z1, mat, tile=1.0, col=True, vis=True):
    """間取り図の向き(u=前からの距離)で箱を置く"""
    return prism(corners((x0, Y(u1), z0), (x1, Y(u0), z1)), mat, tile, col, vis)

def uslope(u0, u1, x0, x1, za, zb, t, mat, tile=1.0, col=True):
    """u0で上面の高さza、u1でzbになる厚さtの板"""
    c = corners((x0, Y(u1), 0), (x1, Y(u0), 0))
    for i, v in enumerate(c):
        top = zb if v.y == Y(u1) else za
        v.z = top if i & 4 else top - t
    return prism(c, mat, tile, col)

def cylinder(center, radius, depth, mat, verts=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=center, rotation=(0, math.pi / 2, 0))
    ob = bpy.context.object
    ob.data.materials.append(MATS[mat])
    VIS.append(ob)

# ---------------- 台車・床下 ----------------
def chassis(u0, length, half_w=CAR_HALF_W):
    u1 = u0 + length
    ubox(u0, u1, -half_w, half_w, -0.6, -0.35, 'hull', 2.0, col=False)               # 台枠
    ubox(u0, u1, -0.5, 0.5, -1.0, -0.6, 'chassis', 1.0, col=False)                  # 中梁
    for uc in (u0 + 3.5, u1 - 3.5):                                                  # 台車
        ubox(uc - 1.6, uc + 1.6, -WHEEL_X - 0.3, -WHEEL_X + 0.3, -1.25, -0.75, 'chassis', 1.0, col=False)
        ubox(uc - 1.6, uc + 1.6, WHEEL_X - 0.3, WHEEL_X + 0.3, -1.25, -0.75, 'chassis', 1.0, col=False)
        ubox(uc - 0.4, uc + 0.4, -WHEEL_X - 0.3, WHEEL_X + 0.3, -0.9, -0.6, 'chassis', 1.0, col=False)
        for ua in (uc - 1.0, uc + 1.0):
            for x in (-WHEEL_X, WHEEL_X):
                cylinder((x, Y(ua), RAIL_Z + 0.45), 0.45, 0.14, 'wheel')
            cylinder((0, Y(ua), RAIL_Z + 0.45), 0.08, WHEEL_X * 2, 'wheel', 8)
    for ue, d in ((u0, -1), (u1, 1)):                                                # 連結器(すき間の真ん中まで)
        ubox(min(ue, ue + d * GAP / 2), max(ue, ue + d * GAP / 2), -0.15, 0.15, -0.75, -0.45, 'chassis', 1.0, col=False)

TAPE_W = 0.35        # 床の切れ目に貼るテープの幅
TAPE_TILE = 0.6      # テープのしま模様1枚の大きさ(m)

def tape_across(ue, d, x0, x1, z):
    """uの位置ueの端に、横(x0〜x1)に貼るテープ。dは床の内側の向き(+1/-1)。zは床の上面"""
    ua, ub = sorted((ue, ue + d * TAPE_W))
    ubox(ua, ub, x0, x1, z - 0.01, z + 0.015, 'hazard', TAPE_TILE, col=False)

def tape_along(u0, u1, xe, d, z):
    """xの位置xeの縁に、長さ方向(u0〜u1)に貼るテープ。dは床の内側の向き(+1/-1)"""
    x0, x1 = sorted((xe, xe + d * TAPE_W))
    ubox(u0, u1, x0, x1, z - 0.01, z + 0.015, 'hazard', TAPE_TILE, col=False)

def flat_deck(u0, length=FLAT_LEN):
    """平台車の床: 真ん中が通路の床グリッド、両側が外板。四方の縁にテープ"""
    u1 = u0 + length
    ubox(u0, u1, -GRID_HALF_W, GRID_HALF_W, -0.35, DECK_Z, 'grid floor', 0.156 * 9)
    for s in (-1, 1):
        ubox(u0, u1, min(GRID_HALF_W * s, CAR_HALF_W * s), max(GRID_HALF_W * s, CAR_HALF_W * s), -0.35, DECK_Z + 0.05, 'hull', 2.0)
        tape_along(u0, u1, s * CAR_HALF_W, -s, DECK_Z + 0.05)
    tape_across(u0, 1, -CAR_HALF_W, CAR_HALF_W, DECK_Z + 0.05)
    tape_across(u1, -1, -CAR_HALF_W, CAR_HALF_W, DECK_Z + 0.05)

def flat_car(u0, length=FLAT_LEN):
    chassis(u0, length)
    flat_deck(u0, length)

# ---------------- 読み込み素材 ----------------
def import_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    return [o for o in bpy.data.objects if o not in before]

def import_obj(path):
    before = set(bpy.data.objects)
    bpy.ops.wm.obj_import(filepath=path)
    return [o for o in bpy.data.objects if o not in before]

def join(objs, name):
    bpy.ops.object.select_all(action='DESELECT')
    meshes = [o for o in objs if o.type == 'MESH']
    for o in meshes:   # 親(空のオブジェクト)を消す前に位置を残す
        mw = o.matrix_world.copy()
        o.parent = None
        o.matrix_world = mw
    for o in objs:
        if o.type != 'MESH':
            bpy.data.objects.remove(o)
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.convert(target='MESH')
    if len(meshes) > 1:
        bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return ob

def bounds(ob):
    vs = [ob.matrix_world @ v.co for v in ob.data.vertices]
    return Vector([min(v[i] for v in vs) for i in range(3)]), Vector([max(v[i] for v in vs) for i in range(3)])

def fit(ob, mn, mx):
    """obの外形をmn〜mxの箱にぴったり合わせる(縦横比は変わる)"""
    a, b = bounds(ob)
    sc = [(mx[i] - mn[i]) / (b[i] - a[i]) for i in range(3)]
    ob.data.transform(Matrix.Translation(-a))
    ob.data.transform(Matrix.Diagonal((*sc, 1)))
    ob.data.transform(Matrix.Translation(Vector(mn)))

def copy_obj(ob):
    c = ob.copy(); c.data = ob.data.copy()
    scene.collection.objects.link(c)
    c.hide_set(False)
    return c

def save_image(img, name, size=None, fmt='PNG'):
    if size and img.size[0] > size:
        img.scale(size, int(size * img.size[1] / img.size[0]))
    path = os.path.join(TEX_DIR, name + ('.png' if fmt == 'PNG' else '.jpg'))
    img.filepath_raw = path
    img.file_format = fmt
    img.save()
    img.filepath = path

def tex_material_from(mat, prefix, size, tint):
    """読み込んだマテリアルのBase Colorの画像だけを使う、DxLib向けの単純なマテリアルにする"""
    img = None
    p = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if mat.use_nodes else None
    if p and p.inputs['Base Color'].is_linked:
        n = p.inputs['Base Color'].links[0].from_node
        while n and n.type != 'TEX_IMAGE' and n.inputs:
            ins = [i for i in n.inputs if i.is_linked]
            n = ins[0].links[0].from_node if ins else None
        if n and n.type == 'TEX_IMAGE':
            img = n.image
    name = prefix + mat.name
    if img:
        img = img.copy()
        save_image(img, name.replace(' ', '_'), size)
        return make_mat(name, img=img, tint=tint)
    return make_mat(name, (0.3, 0.3, 0.3))

# コンテナ(オレンジ・さび)
def load_container(folder, prefix):
    objs = import_fbx(os.path.join(ASSETS, folder, 'source', 'shipping cointainer.fbx'))
    for o in objs:
        if o.type == 'MESH':
            for i, m in enumerate(o.data.materials):
                if m and not m.name.startswith(prefix):
                    o.data.materials[i] = MATS.get(prefix + m.name) or tex_material_from(m, prefix, 1024, TINT_CONTAINER)
    ob = join(objs, prefix + 'container')
    ob.data.transform(Matrix.Rotation(math.pi / 2, 4, 'Z'))   # 扉(元の+X側)を列車の前(+Y)へ
    ob.hide_set(True)
    return ob

def place_container(src, u0, u1, x0, x1, h):
    c = copy_obj(src)
    fit(c, (x0, Y(u1), DECK_Z), (x1, Y(u0), DECK_Z + h))
    VIS.append(c)
    CONTAINERS.add(c.name)
    COL.append(corners((x0, Y(u1), DECK_Z), (x1, Y(u0), DECK_Z + h)))

cont_orange = load_container('freight-shipping-container-orange', 'ContOrange_')
cont_rust = load_container('freight-shipping-container-rusted', 'ContRust_')

def make_roof_texture():
    """さびたコンテナの波板から文字のない所を切り出し、反転して並べて継ぎ目のない屋根のテクスチャにする"""
    src = bpy.data.images.load(os.path.join(TEX_DIR, 'ContRust_corrugated_sides.001.png'), check_existing=True)
    w, h = src.size
    px = np.array(src.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1]   # 画像の上をy=0にする
    k = w / 1024
    c = px[int(300 * k):int(570 * k), int(40 * k):int(270 * k)]
    row = np.concatenate([c, c[:, ::-1]], axis=1)
    tile = np.concatenate([row, row[::-1]], axis=0)[::-1]
    img = bpy.data.images.new('roof_rust', tile.shape[1], tile.shape[0])
    img.pixels.foreach_set(tile.ravel())
    save_image(img, 'roof_rust')
    return img

make_mat('roof rust', img=make_roof_texture(), tint=TINT_ROOF)

# 宇宙船の通路(prepare_corridor.pyで焼き込み済み)
with bpy.data.libraries.load(CORRIDOR_BLEND) as (src, dst):
    dst.objects = ['CorridorCar', 'CorrDoors']
for o in dst.objects:
    scene.collection.objects.link(o)
    o.hide_set(True)
corr_src, doors_src = dst.objects
for o in dst.objects:   # 焼いたテクスチャも暗めの色を掛ける
    for m in o.data.materials:
        p = m.node_tree.nodes.get('Principled BSDF')
        if p and p.inputs['Base Color'].is_linked:
            tint = TINT_SHELL if 'corridor' in m.name else TINT_FLOOR
            p.inputs['Base Color'].default_value = (*tint, 1)
corr_src.data.transform(Matrix.Diagonal((*CORR_SCALE, 1)))
doors_src.data.transform(Matrix.Diagonal((*CORR_SCALE, 1)))
# 床の格子は横に伸びないように、UVも横の拡大率だけ広げる
grid_ids = [i for i, m in enumerate(corr_src.data.materials) if m and 'grid' in m.name]
uvd = corr_src.data.uv_layers.active.data
for poly in corr_src.data.polygons:
    if poly.material_index in grid_ids:
        for li in poly.loop_indices:
            uvd[li].uv.x *= CORR_SCALE[0]
ca, cb = bounds(corr_src)
CORR_LEN = cb.y - ca.y
CORR_HALF_W = cb.x
CORR_TOP = cb.z                                  # 屋根の上面
CORR_INNER_X = 2.07 * CORR_SCALE[0]              # 車内の壁(当たり判定)
CORR_CEIL = 3.0 * CORR_SCALE[2]                  # 車内の天井(当たり判定)
CORR_ROOF_HALF_W = 1.82 * CORR_SCALE[0]          # 屋根の平らな部分の幅の半分(元の通路を測った値)
CORR_EAVE_X = 2.65 * CORR_SCALE[0]               # 屋根の斜めの部分の下端(八角形の角)
CORR_EAVE_Z = 2.21 * CORR_SCALE[2]               # その高さ
CORR_STEP_X = 1.47 * CORR_SCALE[0]               # 車内の床の端の段差(当たり判定)
CORR_FLOOR_Z = 0.01                              # 車内の床の上面(元の通路の床はほぼ0)

def corridor_car(u0, closed=False):
    """通路の車両。closed=Trueなら両端の扉を閉める(屋根の上を歩く車両)"""
    u1 = u0 + CORR_LEN
    chassis(u0, CORR_LEN, CORR_HALF_W)
    off = Matrix.Translation((0, Y(u0 + CORR_LEN / 2), DECK_Z))
    c = copy_obj(corr_src); c.data.transform(off); VIS.append(c)
    if closed:
        d = copy_obj(doors_src); d.data.transform(off); VIS.append(d)
        for ue in (u0, u1 - 0.3):
            ubox(ue, ue + 0.3, -CORR_HALF_W, CORR_HALF_W, DECK_Z, CORR_TOP, 'hull', vis=False)
    # 当たり判定: 床・壁・車内の段差・天井(=屋根)
    ubox(u0, u1, -CORR_HALF_W, CORR_HALF_W, -0.35, DECK_Z, 'hull', vis=False)
    for s in (-1, 1):
        ubox(u0, u1, min(s * CORR_INNER_X, s * CORR_HALF_W), max(s * CORR_INNER_X, s * CORR_HALF_W), DECK_Z, CORR_EAVE_Z, 'hull', vis=False)
        ubox(u0, u1, min(s * CORR_STEP_X, s * CORR_INNER_X), max(s * CORR_STEP_X, s * CORR_INNER_X), DECK_Z, DECK_Z + 0.35, 'hull', vis=False)
        # 屋根の斜めの部分(平らな部分の端から八角形の角まで)
        c = corners((min(s * CORR_ROOF_HALF_W, s * CORR_EAVE_X), Y(u1), 0), (max(s * CORR_ROOF_HALF_W, s * CORR_EAVE_X), Y(u0), 0))
        for i, v in enumerate(c):
            top = CORR_TOP if abs(v.x) < (CORR_ROOF_HALF_W + CORR_EAVE_X) / 2 else CORR_EAVE_Z
            v.z = top if i & 4 else top - 0.25
        prism(c, 'hull', vis=False)
    ubox(u0, u1, -CORR_ROOF_HALF_W, CORR_ROOF_HALF_W, CORR_CEIL, CORR_TOP, 'hull', vis=False)
    roof_skin(u0, u1)
    # 床の切れ目のテープ(車内の入口と、屋根の前後の端)
    for ue, d in ((u0, 1), (u1, -1)):
        tape_across(ue, d, -CORR_STEP_X, CORR_STEP_X, DECK_Z + CORR_FLOOR_Z)
        tape_across(ue, d, -CORR_ROOF_HALF_W, CORR_ROOF_HALF_W, CORR_TOP + 0.03)

def roof_skin(u0, u1):
    """通路の車両の屋根と横の壁に、赤いさびの波板を薄く貼る(見た目だけ)
    横の壁は窓をふさぐので、車内から外が見えなくなる(窓からは赤い壁が見える)"""
    t, lift = 0.05, 0.03
    for s in (-1, 1):
        ubox(u0, u1, min(s * CORR_HALF_W, s * (CORR_HALF_W + 0.06)), max(s * CORR_HALF_W, s * (CORR_HALF_W + 0.06)),
             DECK_Z, CORR_EAVE_Z + lift, 'roof rust', 3.0, col=False)
    ubox(u0, u1, -CORR_ROOF_HALF_W, CORR_ROOF_HALF_W, CORR_TOP - t + lift, CORR_TOP + lift, 'roof rust', 3.0, col=False)
    for s in (-1, 1):
        c = corners((min(s * CORR_ROOF_HALF_W, s * CORR_EAVE_X), Y(u1), 0), (max(s * CORR_ROOF_HALF_W, s * CORR_EAVE_X), Y(u0), 0))
        for i, v in enumerate(c):
            inner = abs(v.x) < (CORR_ROOF_HALF_W + CORR_EAVE_X) / 2
            top = (CORR_TOP if inner else CORR_EAVE_Z) + lift
            v.x += 0 if inner else s * lift   # 角は少し外へ出して元の屋根に埋もれないようにする
            v.z = top if i & 4 else top - t
        prism(c, 'roof rust', 3.0, col=False)

def ramp_car(u0, up=True):
    """スロープ車: 平台車の上に、隣の通路の車両の屋根まで上る(下りる)スロープを載せる"""
    flat_car(u0)
    u1 = u0 + FLAT_LEN
    h = CORR_TOP
    if up:
        ua, ub = u0 + 1.0, u0 + 1.0 + RAMP_LEN           # 斜め
        la, lb = ub, u1                                   # 上の踊り場
    else:
        la, lb = u0, u0 + FLAT_LEN - 1.0 - RAMP_LEN
        ua, ub = lb, u1 - 1.0
    za, zb = (DECK_Z, h) if up else (h, DECK_Z)
    uslope(ua, ub, -RAMP_HALF_W, RAMP_HALF_W, za, zb, 0.25, 'grid floor', 0.156 * 9)
    ubox(la, lb, -RAMP_HALF_W, RAMP_HALF_W, h - 0.25, h, 'grid floor', 0.156 * 9)
    # 手すりの壁(両側)
    for s in (-1, 1):
        x0, x1 = sorted((s * RAMP_HALF_W, s * (RAMP_HALF_W + 0.15)))
        uslope(ua, ub, x0, x1, za + 0.9, zb + 0.9, 0.9 + 0.25, 'hull', 2.0)
        ubox(la, lb, x0, x1, h - 0.25, h + 0.9, 'hull', 2.0)
    # 支柱
    for uu in [la + 0.5, lb - 0.5] + [ua + k * 3.0 for k in range(1, int(RAMP_LEN / 3.0) + 1)]:
        t = (uu - ua) / (ub - ua)
        top = h if la <= uu <= lb else za + (zb - za) * min(max(t, 0.0), 1.0)
        if top - 0.25 <= 0.3:
            continue
        for x0 in (-RAMP_HALF_W, RAMP_HALF_W - 0.4):
            ubox(uu - 0.2, uu + 0.2, x0, x0 + 0.4, DECK_Z, top - 0.25, 'chassis', 1.0, col=False)
    # 床の切れ目のテープ(スロープの上り口・下り口と、屋根につながる踊り場の端)
    tape_across(ua if up else ub, 1 if up else -1, -RAMP_HALF_W, RAMP_HALF_W, DECK_Z + 0.02)
    tape_across(lb if up else la, -1 if up else 1, -RAMP_HALF_W, RAMP_HALF_W, h)
    # 屋根の高さの赤い灯(上り口の目印)
    ubox(la + 0.3, la + 0.6, -RAMP_HALF_W - 0.15, -RAMP_HALF_W, h + 0.9, h + 1.1, 'lamp red', None, col=False)

begin_group("Locomotive")
# 機関車(1両目の前)
loco = join(import_obj(os.path.join(ASSETS, 'train', 'Train.obj')), 'locomotive')
for i, m in enumerate(loco.data.materials):
    loco.data.materials[i] = make_mat('loco ' + m.name, (0.10, 0.10, 0.11) if 'body' in m.name else (0.30, 0.07, 0.05))
loco.data.transform(Matrix.Rotation(math.pi, 4, 'Z'))   # 元のモデルは-Yが前
loco.data.transform(Matrix.Scale(LOCO_SCALE, 4))
a, b = bounds(loco)
loco.data.transform(Matrix.Translation((-(a.x + b.x) / 2, Y(-GAP) - a.y, RAIL_Z - a.z)))
VIS.append(loco)
la_, lb_ = bounds(loco)
COL.append(corners((la_.x + 0.5, la_.y, DECK_Z), (lb_.x - 0.5, lb_.y, lb_.z - 1.0)))

# ---------------- 編成 ----------------
u = 0.0
CARS = []
def next_car(length, name):
    global u
    u0 = u
    CARS.append((u0, length))
    begin_group(name)
    u += length + GAP
    return u0

# 1両目: 平台車(コンテナ) 間取り図 (16,2) など
u0 = next_car(FLAT_LEN, "Car1_Flat")
flat_car(u0)
place_container(cont_orange, u0 + 12, u0 + 20, -7.0, -2.5, 4.0)   # 左右に並べて真ん中を通路にする
place_container(cont_rust, u0 + 13, u0 + 20, 2.5, 7.0, 4.0)

# 2両目: 平台車
u0 = next_car(FLAT_LEN, "Car2_Flat")
flat_car(u0)
place_container(cont_rust, u0 + 12, u0 + 20, -7.0, -2.5, 4.0)
place_container(cont_orange, u0 + 11, u0 + 19, 2.5, 7.0, 4.0)

# 3両目: 通路の車両(中を通る)
U_CAR3 = next_car(CORR_LEN, "Car3_Corridor")
corridor_car(U_CAR3)

# 上りスロープ車 → 4両目の屋根へ
U_RAMP_UP = next_car(FLAT_LEN, "Ramp_Up")
ramp_car(U_RAMP_UP, up=True)

# 4両目: 通路の車両(扉を閉めて屋根の上を歩く)
U_CAR4 = next_car(CORR_LEN, "Car4_CorridorRoof")
corridor_car(U_CAR4, closed=True)

# 下りスロープ車
U_RAMP_DOWN = next_car(FLAT_LEN, "Ramp_Down")
ramp_car(U_RAMP_DOWN, up=False)

# 5両目: 通路の車両(中を通る)
U_CAR5 = next_car(CORR_LEN, "Car5_Corridor")
corridor_car(U_CAR5)

# 6両目: 平台車(最後尾)
U_CAR6 = next_car(FLAT_LEN, "Car6_Flat")
flat_car(U_CAR6)
TRAIN_END = u - GAP

begin_group("Outer")
# 線路の高さの床(すき間から落ちたとき用。見た目なし)
COL.append(corners((-10.0, Y(TRAIN_END + 2), RAIL_Z - 0.2), (10.0, lb_.y, RAIL_Z)))

# 列車の外へ出られないようにする見えない壁(左右の縁と前後の端)
wy0, wy1 = Y(TRAIN_END), lb_.y
for s in (-1, 1):
    x0, x1 = sorted((s * CAR_HALF_W, s * (CAR_HALF_W + 0.5)))
    COL.append(corners((x0, wy0, RAIL_Z), (x1, wy1, DECK_Z + OUTER_WALL_H)))
COL.append(corners((-CAR_HALF_W - 0.5, wy0 - 0.5, RAIL_Z), (CAR_HALF_W + 0.5, wy0, DECK_Z + OUTER_WALL_H)))
COL.append(corners((-CAR_HALF_W - 0.5, wy1, RAIL_Z), (CAR_HALF_W + 0.5, wy1 + 0.5, DECK_Z + OUTER_WALL_H)))

print("LAYOUT", [(round(a, 2), round(l, 2)) for a, l in CARS], "end", round(TRAIN_END, 2))
print("ZRANGE(ゲーム、回した後)", round(-lb_.y * 100), round(-Y(TRAIN_END) * 100))

# ---------------- まとめて出力 ----------------
for o in (cont_orange, cont_rust, corr_src, doors_src):
    bpy.data.objects.remove(o)


def centered(ob):
    """メッシュの中心(底面の中心)を原点にして、その位置をオブジェクトの位置にする(Unityで動かしやすくする)"""
    a, b = bounds(ob)
    c = Vector(((a.x + b.x) / 2, (a.y + b.y) / 2, a.z))
    ob.data.transform(Matrix.Translation(-c))
    ob.location = c
    return ob

def export_unity_fbx(path):
    """Unityで調整する用のFBX。見た目は TrainStage の下、当たり判定は TrainStage_Collision の下に箱1個ずつ。名前は「車両名__部品名」
    ゲーム用と違って180°回さない(Unityの左手系への変換で180°回るので、Unityの座標がそのままゲームの座標になる)
    コンテナは1個ずつ動かせるように別のオブジェクトにしている"""
    created = []
    def empty(name, parent=None):
        e = bpy.data.objects.new(name, None)
        scene.collection.objects.link(e)
        e.parent = parent
        created.append(e)
        return e
    col_mat = MATS.get('collision') or make_mat('collision', (0.2, 0.8, 0.3))
    vis_root, col_root = empty('TrainStage'), empty('TrainStage_Collision')
    bounds_list = GROUPS + [('_end', len(VIS), len(COL))]
    for (name, vs, cs), (_, ve, ce) in zip(GROUPS, bounds_list[1:]):
        vg, cg = vis_root, col_root   # 親は1段だけにする(2段にすると Apply Transform で子の位置の軸が変わらない)。車両のまとまりは Unity 側で名前から作る
        copies = []
        for k, o in enumerate(VIS[vs:ve]):
            c = copy_obj(o)
            c.data.transform(c.matrix_world)
            c.matrix_world = Matrix()
            if o.name in CONTAINERS:
                c.name = f"{name}__Container{k}"
                centered(c).parent = vg
                created.append(c)
            else:
                copies.append(c)
        if copies:
            m = join(copies, f"{name}__Mesh")
            centered(m).parent = vg
            created.append(m)
        for k, cc in enumerate(COL[cs:ce]):
            bm = bmesh.new()
            vv = [bm.verts.new(p) for p in cc]
            for q in ((0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)):
                bm.faces.new([vv[i] for i in q])
            bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
            me = bpy.data.meshes.new(f"{name}__Col{k}")
            bm.to_mesh(me); bm.free()
            me.materials.append(col_mat)
            ob = bpy.data.objects.new(f"{name}__Col{k}", me)
            scene.collection.objects.link(ob)
            centered(ob).parent = cg
            created.append(ob)
    bpy.ops.object.select_all(action='DESELECT')
    for o in created:
        o.select_set(True)
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'MESH', 'EMPTY'},
                             apply_scale_options='FBX_SCALE_ALL', axis_forward=UNITY_AXIS_FORWARD, axis_up='Y',
                             bake_space_transform=True, mesh_smooth_type='FACE', bake_anim=False, path_mode='STRIP')
    print("UNITY", path, "objects", len(created))
    for o in created:
        bpy.data.objects.remove(o)

UNITY_AXIS_FORWARD = '-Z'
export_unity_fbx(os.path.join(OUT, 'TrainStage_Unity.fbx'))

stage = join(VIS, 'TrainStage')
bm = bmesh.new()
for c in COL:
    vs = [bm.verts.new(p) for p in c]
    for q in ((0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)):
        bm.faces.new([vs[i] for i in q])
bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
cme = bpy.data.meshes.new('TrainStageCol')
bm.to_mesh(cme); bm.free()
cme.materials.append(make_mat('collision', (0.2, 0.8, 0.3)))
colob = bpy.data.objects.new('TrainStageCol', cme)
scene.collection.objects.link(colob)

# デモステージと同じくゲームの+Z(BlenderのY)へ進むように、最後に180°回す(機関車が-Z、最後尾が+Z)
# ゲームのライトは+Zへ向かって照らしているので、進む向きが逆だと敵やステージの照らされていない側ばかり見えてしまう
for me in (stage.data, cme):
    me.transform(Matrix.Rotation(math.pi, 4, "Z"))

fbx_kw = dict(use_selection=True, object_types={'MESH'}, apply_scale_options='FBX_SCALE_ALL',
              use_mesh_modifiers=True, mesh_smooth_type='FACE', bake_anim=False, path_mode='RELATIVE')
for ob in (stage, colob):
    bpy.ops.object.select_all(action='DESELECT')
    ob.select_set(True); bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, ob.name + '.fbx'), **fbx_kw)
    a, b = bounds(ob)
    print("OUT", ob.name, "tris", sum(len(p.vertices) - 2 for p in ob.data.polygons),
          "mats", len(ob.data.materials), "min", [round(v, 2) for v in a], "max", [round(v, 2) for v in b])
colob.hide_set(True)
colob.hide_render = True
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'TrainStage.blend'))
