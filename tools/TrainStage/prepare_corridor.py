# 宇宙船の通路(spaceship_corridor_blend)を列車の車両に使える形にする
# ・ノードで作られた材質を画像に焼き込む(DxLibはノードを読めないため)
# ・消火器・斧・天井のケーブルなどの重い小物を外し、ポリゴンを減らす
# ・窓のガラスを外す(車内からトンネルが見える)
# 使い方: blender -b "Spaceship corridor.blend" --python prepare_corridor.py -- テクスチャの出力フォルダ 出力.blend
import bpy, bmesh, os, sys, math
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
TEX_DIR, OUT_BLEND = argv[0], argv[1]
os.makedirs(TEX_DIR, exist_ok=True)
scene = bpy.context.scene

KEEP = ['Corridor', 'Floor_Wires', 'Roof_Pipes', 'Roof_Vent', 'Flickering_emmision',
        'Screen1', 'Screen1.001', 'Screen1.003', 'Screen_Damaged',
        'Screen_video', 'Screen_video.001', 'Screen_video.002', 'Screen_video.003',
        'FrontDoor_1.001', 'FrontDoor_2.001', 'BackDoor_1', 'BackDoor_2', 'Cube', 'Cube.002']
DECIMATE = {'Cube': 0.12, 'Cube.002': 0.12}   # 細かい格子・木箱
DISSOLVE = ['Corridor', 'Roof_Vent']                    # 平らな面をまとめる
FLOOR_Z = 0.1433                                         # 通路の床の高さ(これを0にする)

def lin2srgb(c):
    return 12.92 * c if c <= 0.0031308 else 1.055 * (c ** (1 / 2.4)) - 0.055

def select_only(obs, active=None):
    bpy.ops.object.select_all(action='DESELECT')
    for o in obs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = active or obs[0]

# ---- 床グリッドのテクスチャ(平台車・スロープ用)を真上から撮る ----
def render_floor_texture():
    for o in bpy.data.objects:
        if o.type == 'LIGHT' or o.name in ('Wires_Roof', 'Roof_Pipes', 'Roof_Vent', 'Flickering_emmision', 'Cabels'):
            o.hide_render = True
    w = bpy.data.worlds.new('flat')
    w.use_nodes = True
    w.node_tree.nodes['Background'].inputs[0].default_value = (1, 1, 1, 1)
    w.node_tree.nodes['Background'].inputs[1].default_value = 1.0
    scene.world = w
    scene.render.engine = 'BLENDER_EEVEE'
    scene.render.resolution_x = scene.render.resolution_y = 512
    scene.render.film_transparent = False
    scene.view_settings.view_transform = 'Standard'
    cd = bpy.data.cameras.new('topcam')
    cd.type = 'ORTHO'
    cd.ortho_scale = 0.156 * 9                    # 格子9マス分(継ぎ目なく並ぶように)
    cam = bpy.data.objects.new('topcam', cd)
    scene.collection.objects.link(cam)
    cam.location = (0.0, -7.25, 2.0)
    scene.camera = cam
    scene.render.filepath = os.path.join(TEX_DIR, 'grid_floor.png')
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)

render_floor_texture()

# ---- いらない物を消す ----
for o in list(bpy.data.objects):
    if o.name not in KEEP:
        bpy.data.objects.remove(o)
for o in bpy.data.objects:
    # ビューレイヤーから外れたコレクションに入っている物があるので、シーン直下に移す(選択・適用できるように)
    for c in list(o.users_collection):
        c.objects.unlink(o)
    scene.collection.objects.link(o)
    o.hide_render = False
    o.hide_set(False)
    o.hide_viewport = False
    o.hide_select = False

# 親を外して変形を確定(モディファイアも適用)
for o in list(bpy.data.objects):
    mw = o.matrix_world.copy()
    o.parent = None
    o.matrix_world = mw
    if o.data.users > 1:   # 共有メッシュは単独にしないと適用できない
        o.data = o.data.copy()
select_only(list(bpy.data.objects))
bpy.ops.object.convert(target='MESH')
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# 窓のガラス・床のケーブル・天井の格子を外す(ケーブルと格子はポリゴンが多すぎる)
def delete_faces(ob, mat_names):
    bm = bmesh.new(); bm.from_mesh(ob.data)
    ids = [i for i, m in enumerate(ob.data.materials) if m and m.name in mat_names]
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index in ids], context='FACES')
    bm.to_mesh(ob.data); bm.free()

delete_faces(bpy.data.objects['Corridor'], ('Glass', 'Rubber'))
delete_faces(bpy.data.objects['Roof_Vent'], ('Roof_Grid_Metal',))

# ポリゴンを減らす
for name, ratio in DECIMATE.items():
    o = bpy.data.objects[name]
    o.modifiers.new('dec', 'DECIMATE').ratio = ratio
for name in DISSOLVE:
    md = bpy.data.objects[name].modifiers.new('dis', 'DECIMATE')
    md.decimate_type = 'DISSOLVE'
    md.angle_limit = math.radians(2)
select_only(list(bpy.data.objects))
bpy.ops.object.convert(target='MESH')

# 動画の画面は静止画にする
screen_mat = bpy.data.materials['Screen']
for o in bpy.data.objects:
    for i, m in enumerate(o.data.materials):
        if m and m.name == 'Screen_video':
            o.data.materials[i] = screen_mat

# ---- 焼き込み ----
def albedo_socket(mat):
    """マテリアルの色(金属部分も含む)を出すソケットを作って返す"""
    nt = mat.node_tree
    out = next(n for n in nt.nodes if n.type == 'OUTPUT_MATERIAL' and n.is_active_output)
    src = out.inputs['Surface'].links[0].from_node if out.inputs['Surface'].is_linked else None

    def color_of(n):
        if n is None:
            return None, (0.5, 0.5, 0.5)
        if n.type in ('BSDF_PRINCIPLED', 'BSDF_METALLIC', 'BSDF_DIFFUSE'):
            s = n.inputs['Base Color'] if 'Base Color' in n.inputs else n.inputs['Color']
            return (s.links[0].from_socket if s.is_linked else None), tuple(s.default_value[:3])
        if n.type == 'EMISSION':
            s = n.inputs['Color']
            return (s.links[0].from_socket if s.is_linked else None), tuple(s.default_value[:3])
        if n.type == 'MIX_SHADER':
            fac = n.inputs[0]
            a = color_of(n.inputs[1].links[0].from_node if n.inputs[1].is_linked else None)
            b = color_of(n.inputs[2].links[0].from_node if n.inputs[2].is_linked else None)
            mix = nt.nodes.new('ShaderNodeMix'); mix.data_type = 'RGBA'
            if fac.is_linked:
                nt.links.new(fac.links[0].from_socket, mix.inputs['Factor'])
            else:
                mix.inputs['Factor'].default_value = fac.default_value
            for (sock, col), inp in ((a, mix.inputs[6]), (b, mix.inputs[7])):
                if sock:
                    nt.links.new(sock, inp)
                else:
                    inp.default_value = (*col, 1)
            return mix.outputs[2], None
        return None, (0.5, 0.5, 0.5)
    return color_of(src), out

def bake_objects(obs, img_name, size, fmt='JPEG'):
    """obsの全マテリアルの色を1枚の画像に焼き込み、その画像だけのマテリアルに差し替える"""
    select_only(obs)
    if len(obs) > 1:
        bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.data.uv_layers.new(name='bake')
    ob.data.uv_layers.active = ob.data.uv_layers['bake']
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.002)
    bpy.ops.object.mode_set(mode='OBJECT')
    img = bpy.data.images.new(img_name, size, size, alpha=False)
    for m in {m for m in ob.data.materials if m}:
        (sock, col), out = albedo_socket(m)
        nt = m.node_tree
        em = nt.nodes.new('ShaderNodeEmission')
        if sock:
            nt.links.new(sock, em.inputs['Color'])
        else:
            em.inputs['Color'].default_value = (*col, 1)
        nt.links.new(em.outputs[0], out.inputs['Surface'])
        t = nt.nodes.new('ShaderNodeTexImage'); t.image = img
        nt.nodes.active = t
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 8
    scene.cycles.device = 'CPU'
    bpy.ops.object.bake(type='EMIT', margin=4, use_clear=True)
    path = os.path.join(TEX_DIR, img_name + ('.jpg' if fmt == 'JPEG' else '.png'))
    img.filepath_raw = path; img.file_format = fmt
    img.save()
    img.filepath = path
    # 焼いた画像だけのマテリアルにする
    mat = bpy.data.materials.new(img_name)
    mat.use_nodes = True
    p = mat.node_tree.nodes['Principled BSDF']
    t = mat.node_tree.nodes.new('ShaderNodeTexImage'); t.image = img
    mat.node_tree.links.new(t.outputs['Color'], p.inputs['Base Color'])
    ob.data.materials.clear()
    ob.data.materials.append(mat)
    for uv in [u for u in ob.data.uv_layers if u.name != 'bake']:
        ob.data.uv_layers.remove(uv)
    ob.data.uv_layers['bake'].name = 'UVMap'   # 他の部品と結合したときに同じUVとして扱われるように
    return ob

def split_by_material(ob, names):
    """namesのマテリアルの面を別オブジェクトに分ける"""
    select_only([ob])
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='DESELECT')
    for i, m in enumerate(ob.data.materials):
        if m and m.name in names:
            ob.active_material_index = i
            bpy.ops.object.material_slot_select()
    bpy.ops.mesh.separate(type='SELECTED')
    bpy.ops.object.mode_set(mode='OBJECT')
    return [o for o in bpy.context.selected_objects if o != ob][0]

PROC = ('Procedural Imperfections01.001', 'Procedural Imperfections01.003')
shell_a = split_by_material(bpy.data.objects['Corridor'], PROC)
shell_b = split_by_material(bpy.data.objects['Roof_Vent'], PROC)
shell = bake_objects([shell_a, shell_b], 'corridor_shell', 4096)
shell.name = 'CorrShell'
doors = bake_objects([bpy.data.objects[n] for n in ('FrontDoor_1.001', 'FrontDoor_2.001', 'BackDoor_1', 'BackDoor_2')], 'corridor_door', 1024)
doors.name = 'CorrDoors'
crate_a = bake_objects([bpy.data.objects['Cube']], 'scifi_crate_a', 1024)
crate_a.name = 'SciCrateA'
crate_b = bake_objects([bpy.data.objects['Cube.002']], 'scifi_crate_b', 1024)
crate_b.name = 'SciCrateB'

# 外板のテクスチャ(スロープや台枠に貼る)を板に焼く
bpy.ops.mesh.primitive_plane_add(size=2.0)
plane = bpy.context.object
plane.data.materials.append(bpy.data.materials['Procedural Imperfections01.001'].copy())
hull = bake_objects([plane], 'hull', 1024)
bpy.data.objects.remove(hull)

# 残りは単色のマテリアルにする(DxLib向け)
EMIT = {'Emmision': (1.0, 0.95, 0.85), 'Flickering_emmision': (1.0, 0.95, 0.85)}
for m in list(bpy.data.materials):
    if m.users == 0 or m.name in ('corridor_shell', 'corridor_door', 'scifi_crate_a', 'scifi_crate_b', 'hull') or not m.use_nodes:
        continue
    if m.name in ('Screen', 'Screen_damged'):
        img = next(n.image for n in m.node_tree.nodes if n.type == 'TEX_IMAGE')
        img.unpack(method='USE_ORIGINAL') if img.packed_file else None
        path = os.path.join(TEX_DIR, 'screen.jpg')
        img.filepath_raw = path; img.file_format = 'JPEG'; img.save(); img.filepath = path
        new = bpy.data.materials.new('screen')
        new.use_nodes = True
        t = new.node_tree.nodes.new('ShaderNodeTexImage'); t.image = img
        new.node_tree.links.new(t.outputs['Color'], new.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
        new.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value = 0.6
        new.node_tree.nodes['Principled BSDF'].inputs['Emission Color'].default_value = (0.6, 0.7, 1.0, 1)
    else:
        (sock, col), out = albedo_socket(m)
        col = col or (0.3, 0.3, 0.3)
        new = bpy.data.materials.new('corr ' + m.name)
        new.use_nodes = True
        p = new.node_tree.nodes['Principled BSDF']
        c = tuple(lin2srgb(v) for v in col)
        p.inputs['Base Color'].default_value = (*c, 1)
        if m.name in EMIT:
            p.inputs['Base Color'].default_value = (*EMIT[m.name], 1)
            p.inputs['Emission Color'].default_value = (*EMIT[m.name], 1)
            p.inputs['Emission Strength'].default_value = 2.0
    for o in bpy.data.objects:
        for i, mm in enumerate(o.data.materials):
            if mm == m:
                o.data.materials[i] = new

# 床の格子は重いので、撮った格子のテクスチャを貼った板にする
gimg = bpy.data.images.load(os.path.join(TEX_DIR, 'grid_floor.png'))
gmat = bpy.data.materials.new('grid_floor')
gmat.use_nodes = True
gt = gmat.node_tree.nodes.new('ShaderNodeTexImage'); gt.image = gimg
gmat.node_tree.links.new(gt.outputs['Color'], gmat.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
ys = [(shell.matrix_world @ v.co).y for v in shell.data.vertices]
y0, y1, gx, tile = min(ys) + 0.3, max(ys) - 0.3, 1.42, 0.156 * 9
fbm = bmesh.new()
uvl = fbm.loops.layers.uv.new('UVMap')
f = fbm.faces.new([fbm.verts.new(p) for p in ((-gx, y0, 0.13), (gx, y0, 0.13), (gx, y1, 0.13), (-gx, y1, 0.13))])
for l in f.loops:
    l[uvl].uv = (l.vert.co.x / tile, l.vert.co.y / tile)
fme = bpy.data.meshes.new('FloorPlate'); fbm.to_mesh(fme); fbm.free()
fme.materials.append(gmat)
plate = bpy.data.objects.new('FloorPlate', fme)
scene.collection.objects.link(plate)

# 本体(扉以外)を1つにまとめ、床をz=0・長さ方向の中心をy=0にする
for o in list(bpy.data.objects):
    if len(o.data.polygons) == 0:
        bpy.data.objects.remove(o)
body = [o for o in bpy.data.objects if o.name not in ('CorrDoors', 'SciCrateA', 'SciCrateB')]
select_only(body, shell)
bpy.ops.object.join()
car = bpy.context.view_layer.objects.active
car.name = 'CorridorCar'
ys = [v.co.y for v in car.data.vertices]
cy = (min(ys) + max(ys)) / 2
for o in (car, doors):
    o.data.transform(Matrix.Translation((0, -cy, -FLOOR_Z)))
for o in (crate_a, crate_b):
    vs = [v.co for v in o.data.vertices]
    c = Vector(((min(v.x for v in vs) + max(v.x for v in vs)) / 2, (min(v.y for v in vs) + max(v.y for v in vs)) / 2, min(v.z for v in vs)))
    o.data.transform(Matrix.Translation(-c))
for o in bpy.data.objects:
    print("CORR", o.name, "tris", sum(len(p.vertices) - 2 for p in o.data.polygons), "mats", [m.name for m in o.data.materials],
          "y", round(min(v.co.y for v in o.data.vertices), 2), round(max(v.co.y for v in o.data.vertices), 2))
for img in list(bpy.data.images):   # 元の空の画像(exrなど)を外して軽くする
    if img.packed_file and not img.filepath.startswith(TEX_DIR):
        bpy.data.images.remove(img)
bpy.data.orphans_purge(do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=OUT_BLEND)
