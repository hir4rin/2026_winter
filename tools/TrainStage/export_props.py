# 列車ステージで使った素材を、Unityでそのまま置ける部品(FBX)にする
# 使い方: blender -b --python export_props.py -- 素材フォルダ 列車ステージのフォルダ(data/Stage/TrainStage) 出力フォルダ(Unity の Assets/TrainStage/Props)
# ・大きさはステージで使っている大きさ(1m=1)。原点は底面の中心
# ・テクスチャは 出力フォルダ/Textures にコピーする(Unity はFBXの隣の Textures からテクスチャを探す)
# ・軸の設定は TrainStage_Unity.fbx と同じ(Unity の +Z が列車の後ろ=プレイヤーが進む向き)
import bpy, bmesh, math, sys, os, shutil
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
ASSETS, STAGE_DIR, OUT = argv[0], argv[1], argv[2]
TEX_OUT = os.path.join(OUT, 'Textures')
os.makedirs(TEX_OUT, exist_ok=True)
STAGE_TEX = os.path.join(STAGE_DIR, 'tex')

# build_train_stage.py と同じ大きさ・色
CORR_SCALE = (3.0, 1.0, 1.9)
LOCO_SCALE = 0.55
CRATE_SCALE = 1.8
TINT = {'container': 0.95, 'shell': (0.5, 0.5, 0.53), 'floor': 0.7, 'crate': 0.65, 'door': 0.8}


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def tex_mat(name, img_path, tint=1.0, color=None):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    t = tint if isinstance(tint, tuple) else (tint, tint, tint)
    if img_path:
        img = bpy.data.images.load(img_path, check_existing=True)
        n = m.node_tree.nodes.new('ShaderNodeTexImage'); n.image = img
        m.node_tree.links.new(n.outputs['Color'], p.inputs['Base Color'])
        p.inputs['Base Color'].default_value = (*t, 1)
    else:
        p.inputs['Base Color'].default_value = (*(color or t), 1)
    return m

def save_tex(src, name, size=None):
    """画像を出力の Textures に保存する(大きすぎるものは縮める)"""
    dst = os.path.join(TEX_OUT, name)
    if size:
        img = bpy.data.images.load(src)
        if img.size[0] > size:
            img.scale(size, int(size * img.size[1] / img.size[0]))
        img.filepath_raw = dst
        img.file_format = 'JPEG' if name.lower().endswith('.jpg') else 'PNG'
        img.save()
    else:
        shutil.copyfile(src, dst)
    return dst

def import_any(path):
    before = set(bpy.data.objects)
    if path.lower().endswith('.fbx'):
        bpy.ops.import_scene.fbx(filepath=path)
    else:
        bpy.ops.wm.obj_import(filepath=path)
    return [o for o in bpy.data.objects if o not in before]

def join_all(name):
    objs = [o for o in bpy.data.objects if o.type == 'MESH']
    for o in objs:
        mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw
    for o in [o for o in bpy.data.objects if o.type != 'MESH']:
        bpy.data.objects.remove(o)
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
        if o.data.users > 1:
            o.data = o.data.copy()
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.convert(target='MESH')
    if len(objs) > 1:
        bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    return ob

def bounds(ob):
    vs = [v.co for v in ob.data.vertices]
    return Vector([min(v[i] for v in vs) for i in range(3)]), Vector([max(v[i] for v in vs) for i in range(3)])

def fit(ob, size):
    """外形を size(x, y, z) の箱に合わせて、底面の中心を原点にする"""
    a, b = bounds(ob)
    ob.data.transform(Matrix.Translation(-(a + b) / 2))
    ob.data.transform(Matrix.Diagonal((*[size[i] / (b[i] - a[i]) for i in range(3)], 1)))
    a, b = bounds(ob)
    ob.data.transform(Matrix.Translation((0, 0, -a.z)))

def to_origin(ob, matrix=None):
    """(拡大などをしてから)底面の中心を原点にする"""
    if matrix is not None:
        ob.data.transform(matrix)
    a, b = bounds(ob)
    ob.data.transform(Matrix.Translation((-(a.x + b.x) / 2, -(a.y + b.y) / 2, -a.z)))

def decimate(ob, ratio):
    md = ob.modifiers.new('dec', 'DECIMATE'); md.ratio = ratio
    bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True); bpy.context.view_layer.objects.active = ob
    bpy.ops.object.modifier_apply(modifier=md.name)

def export(ob, name):
    ob.name = name
    bpy.ops.object.select_all(action='DESELECT'); ob.select_set(True); bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + '.fbx'), use_selection=True, object_types={'MESH'},
                             apply_scale_options='FBX_SCALE_ALL', axis_forward='-Z', axis_up='Y', bake_space_transform=True,
                             mesh_smooth_type='FACE', bake_anim=False, path_mode='STRIP')
    a, b = bounds(ob)
    print("PROP", name, "tris", sum(len(p.vertices) - 2 for p in ob.data.polygons), "size", [round(b[i] - a[i], 2) for i in range(3)])

# ---- コンテナ(オレンジ・さび)。ステージと同じ 幅4.5 × 長さ8 × 高さ4、扉が -Y(Unityでは +Z) ----
for key, folder, prefix, sub in (('Orange', 'freight-shipping-container-orange', 'ContOrange_', 'Orange'),
                                 ('Rust', 'freight-shipping-container-rusted', 'ContRust_', 'Rusted')):
    reset()
    import_any(os.path.join(ASSETS, folder, 'source', 'shipping cointainer.fbx'))
    ob = join_all('c')
    tex_dir = os.path.join(ASSETS, folder, 'source', sub)
    files = {f.lower(): f for f in os.listdir(tex_dir)}
    for i, m in enumerate(ob.data.materials):
        part = m.name.split('.')[0]
        cand = [f for f in files if part.replace(' ', '_') in f.replace(' ', '_') and 'basecolor' in f]
        path = None
        if cand:
            path = save_tex(os.path.join(tex_dir, files[cand[0]]), f'{prefix}{part.replace(" ", "_")}.png', 1024)
        ob.data.materials[i] = tex_mat(prefix + part, path, TINT['container'])
    ob.data.transform(Matrix.Rotation(math.pi / 2, 4, 'Z'))
    fit(ob, (4.5, 8.0, 4.0))
    export(ob, f'Container_{key}')

# ---- 機関車 ----
reset()
import_any(os.path.join(ASSETS, 'train', 'Train.obj'))
ob = join_all('loco')
for i, m in enumerate(ob.data.materials):
    ob.data.materials[i] = tex_mat('loco ' + m.name, None, color=(0.10, 0.10, 0.11) if 'body' in m.name else (0.30, 0.07, 0.05))
to_origin(ob, Matrix.Scale(LOCO_SCALE, 4))
export(ob, 'Locomotive')

# ---- 貨車の扉(スキャン) 高さ3.8m ----
reset()
import_any(os.path.join(ASSETS, 'rusty-train-wagon-door', 'source', 'vagon1.fbx'))
ob = join_all('wagon')
decimate(ob, 0.2)
ob.data.materials.clear()
ob.data.materials.append(tex_mat('wagon door', save_tex(os.path.join(ASSETS, 'rusty-train-wagon-door', 'textures', '29k_u1_v1_diffuse-sharpen.jpeg'), 'wagon_door.jpg', 2048), TINT['door']))
a, b = bounds(ob)
to_origin(ob, Matrix.Scale(3.8 / (b.z - a.z), 4))
export(ob, 'WagonDoor')

# ---- ドア(スキャン) 高さ2.6m ----
reset()
import_any(os.path.join(ASSETS, 'scan-door-04-freepolyorg', 'source', 'Door 04_OK_Scans.obj'))
ob = join_all('door')
decimate(ob, 0.1)
ob.data.materials.clear()
ob.data.materials.append(tex_mat('door04', save_tex(os.path.join(ASSETS, 'scan-door-04-freepolyorg', 'textures', 'Door 04_Diffuse.jpeg'), 'door04.jpg', 2048), TINT['door']))
a, b = bounds(ob)
to_origin(ob, Matrix.Scale(2.6 / (b.z - a.z), 4))
export(ob, 'ScanDoor')

# ---- 宇宙船の通路の車両・扉・SF木箱(prepare_corridor.py で焼き込み済み) ----
def from_corridor(obj_name, out_name, matrix, tint_of):
    reset()
    with bpy.data.libraries.load(os.path.join(STAGE_DIR, 'corridor_car.blend')) as (src, dst):
        dst.objects = [obj_name]
    ob = dst.objects[0]
    bpy.context.scene.collection.objects.link(ob)
    ob.data = ob.data.copy()
    for m in ob.data.materials:
        p = m.node_tree.nodes.get('Principled BSDF')
        img = next((n.image for n in m.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image), None)
        if img:
            save_tex(bpy.path.abspath(img.filepath), os.path.basename(img.filepath))
            t = tint_of(m.name)
            p.inputs['Base Color'].default_value = (*(t if isinstance(t, tuple) else (t, t, t)), 1)
    to_origin(ob, matrix)
    export(ob, out_name)

shell_tint = lambda n: TINT['shell'] if 'corridor' in n else TINT['floor'] if 'grid' in n else TINT['crate']
from_corridor('CorridorCar', 'CorridorCar', Matrix.Diagonal((*CORR_SCALE, 1)), shell_tint)
from_corridor('CorrDoors', 'CorridorDoors', Matrix.Diagonal((*CORR_SCALE, 1)), shell_tint)
from_corridor('SciCrateA', 'SciCrateA', Matrix.Scale(CRATE_SCALE, 4), shell_tint)
from_corridor('SciCrateB', 'SciCrateB', Matrix.Scale(CRATE_SCALE, 4), shell_tint)

# ---- 黄黒のテープ(1m × 0.35m、厚さ2.5cm) と 赤いさびの波板(1m角の板) ----
def tile_plate(name, mat_name, tex_name, size, tile, tint=1.0):
    reset()
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    bmesh.ops.create_cube(bm, size=1.0)
    bm.transform(Matrix.Diagonal((*size, 1)))
    for f in bm.faces:
        ax = max(range(3), key=lambda i: abs(f.normal[i]))
        a, b = {0: (1, 2), 1: (0, 2), 2: (1, 0)}[ax]
        for l in f.loops:
            l[uv].uv = (l.vert.co[a] / tile, l.vert.co[b] / tile)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    me.materials.append(tex_mat(mat_name, save_tex(os.path.join(STAGE_TEX, tex_name), tex_name), tint))
    ob = bpy.data.objects.new(name, me); bpy.context.scene.collection.objects.link(ob)
    to_origin(ob)
    export(ob, name)

tile_plate('HazardTape', 'hazard', 'hazard.png', (0.35, 1.0, 0.025), 0.6)
tile_plate('RustPanel', 'roof rust', 'roof_rust.png', (1.0, 1.0, 0.05), 3.0)

# ---- 地下鉄のトンネル1区間(ステージと同じ大きさ・色) ----
reset()
with bpy.data.libraries.load(os.path.join(os.path.dirname(STAGE_DIR), 'TrainTunnel', 'TrainTunnel.blend')) as (src, dst):
    dst.objects = ['TrainTunnelSection']
ob = dst.objects[0]
bpy.context.scene.collection.objects.link(ob)
ob.data = ob.data.copy()
export(ob, 'TunnelSection')
