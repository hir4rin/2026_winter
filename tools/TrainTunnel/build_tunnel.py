import bpy, bmesh, math, sys, os
from mathutils import Matrix, Vector

argv = sys.argv[sys.argv.index('--') + 1:]
OUT = argv[0]
SCALE = float(argv[1]) if len(argv) > 1 else 6.0   # 元モデル(直径約2m)の拡大率
COUNT = int(argv[2]) if len(argv) > 2 else 4      # ループモデルに並べる区間数
X_SCALE = float(argv[3]) if len(argv) > 3 else 1.0    # 横幅だけさらに広げる倍率
SECTION_LEN = 3.0                                  # 元モデル1区間の長さ(m)
os.makedirs(OUT, exist_ok=True)

WALL = ['Tunnel']
FLOOR = ['Rails', 'wires2', 'wires3', 'bolts', 'underground pipes']
RAIL_CENTER_X = 0.065   # 線路中心のずれ
DECIMATE = {'lamp net': 0.2, 'lamp net2': 0.2, 'wires': 0.3, 'wire3': 0.3, 'wires2': 0.3, 'wires3': 0.3,
            'lamp wires': 0.3, 'lamp wires2': 0.3}   # 重い小物のポリゴンを減らす
RING_RADIUS = 0.885     # リング照明の半径(元モデル単位)

for name, ratio in DECIMATE.items():
    o = bpy.data.objects.get(name)
    if o:
        o.modifiers.new('dec', 'DECIMATE').ratio = ratio

# ---- マテリアル: DxLib向けにsRGBへ変換して明るさを合わせる ----
def lin2srgb(c):
    return 12.92 * c if c <= 0.0031308 else 1.055 * (c ** (1 / 2.4)) - 0.055

for m in bpy.data.materials:
    if not m.use_nodes:
        continue
    p = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    if p is None:
        continue
    bc = p.inputs['Base Color']
    for l in list(bc.links):
        m.node_tree.links.remove(l)
    if m.name == 'wet':
        col = (0.10, 0.09, 0.07)
    else:
        col = tuple(lin2srgb(v) for v in bc.default_value[:3])
    bc.default_value = (*col, 1.0)
    m.diffuse_color = (*col, 1.0)
    rl = p.inputs['Roughness']
    for l in list(rl.links):
        m.node_tree.links.remove(l)

# ---- 1) 元の1区間をそのままFBX化 ----
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
bpy.ops.object.select_all(action='DESELECT')
for o in meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
fbx_kw = dict(use_selection=True, object_types={'MESH'}, apply_scale_options='FBX_SCALE_ALL',
              use_mesh_modifiers=True, mesh_smooth_type='FACE', bake_anim=False, path_mode='STRIP')
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, 'SubwaySection.fbx'), **fbx_kw)

# ---- 2) ループ用トンネルを組み立て ----
dg = bpy.context.evaluated_depsgraph_get()
mat_list = []
def mat_index(mat):
    if mat not in mat_list:
        mat_list.append(mat)
    return mat_list.index(mat)

def to_bm(o):
    me = bpy.data.meshes.new_from_object(o.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
    me.transform(o.matrix_world)
    bm = bmesh.new(); bm.from_mesh(me)
    remap = [mat_index(m) for m in me.materials] or [0]
    for f in bm.faces:
        f.material_index = remap[min(f.material_index, len(remap) - 1)]
    bpy.data.meshes.remove(me)
    return bm

def angle_normal(deg):
    a = math.radians(deg)
    return Vector((-math.sin(a), 0, math.cos(a)))   # XZ面で角度が増える側を向く

def cut(bm, no, keep_positive):
    geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-5, plane_co=(0, 0, 0), plane_no=no,
                           clear_inner=keep_positive, clear_outer=not keep_positive)

def merge_into(dst, src, matrix=None, flip=False):
    tmp = bpy.data.meshes.new('tmp'); src.to_mesh(tmp)
    if matrix is not None:
        tmp.transform(matrix)
    if flip:
        tmp.flip_normals()
    dst_tmp = bmesh.new(); dst_tmp.from_mesh(tmp)
    vmap = {}
    for v in dst_tmp.verts:
        vmap[v] = dst.verts.new(v.co)
    dst.verts.index_update()
    for f in dst_tmp.faces:
        try:
            nf = dst.faces.new([vmap[v] for v in f.verts])
            nf.material_index = f.material_index
            nf.smooth = f.smooth
        except ValueError:
            pass
    dst_tmp.free(); bpy.data.meshes.remove(tmp)

MIRROR = Matrix.Scale(-1, 4, (1, 0, 0))
section = bmesh.new()

# 壁: 左半分(角度116°以上)を残し、左右反転 + 天井をくさびで埋める
wall_src = to_bm(bpy.data.objects['Tunnel'])
cut(wall_src, Vector((1, 0, 0)), keep_positive=False)        # x<0
wedge = wall_src.copy()
cut(wall_src, angle_normal(116), keep_positive=True)          # 116°〜270°
cut(wedge, angle_normal(120), keep_positive=True)
cut(wedge, angle_normal(172), keep_positive=False)            # 120°〜172°
merge_into(section, wall_src)
merge_into(section, wall_src, MIRROR, flip=True)
merge_into(section, wedge, Matrix.Rotation(math.radians(56), 4, 'Y'))   # → 64°〜116°
wall_src.free(); wedge.free()

rail_top = -1e9
for o in meshes:
    if o.name in WALL:
        continue
    bm = to_bm(o)
    if o.name in FLOOR:
        if o.name == 'Rails':
            rail_top = max(v.co.z for v in bm.verts)
        merge_into(section, bm, Matrix.Translation((-RAIL_CENTER_X, 0, 0)))
    else:
        merge_into(section, bm)
        merge_into(section, bm, MIRROR, flip=True)
    bm.free()

# リング照明(床にかからない範囲の円弧)
ring_mat = bpy.data.materials.new('ring light')
ring_mat.diffuse_color = (1.0, 0.85, 0.55, 1.0)
ring_mat.use_nodes = True
rp = ring_mat.node_tree.nodes['Principled BSDF']
rp.inputs['Base Color'].default_value = (1.0, 0.85, 0.55, 1.0)
rp.inputs['Emission Color'].default_value = (1.0, 0.85, 0.55, 1.0)
rp.inputs['Emission Strength'].default_value = 3.0
ring_idx = mat_index(ring_mat)
ring = bmesh.new()
steps = 48
a0, a1 = math.radians(-35), math.radians(215)
for i in range(steps + 1):
    a = a0 + (a1 - a0) * i / steps
    c, s = math.cos(a), math.sin(a)
    for r in (RING_RADIUS, RING_RADIUS - 0.012):
        for y in (-0.03, 0.03):
            ring.verts.new((c * r, y, s * r))
ring.verts.ensure_lookup_table()
for i in range(steps):
    b = i * 4
    for q in ((0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)):   # 外・内・手前・奥
        f = ring.faces.new([ring.verts[b + k] for k in q])
        f.material_index = ring_idx
bmesh.ops.recalc_face_normals(ring, faces=ring.faces[:])
merge_into(section, ring)
ring.free()

ys = [v.co.y for v in section.verts]
print("SECTION y range", min(ys), max(ys), "rail_top", rail_top, "faces", len(section.faces))

def make_object(name, bm_src, count):
    me = bpy.data.meshes.new(name)
    full = bmesh.new()
    for k in range(count):
        off = (k - (count - 1) / 2) * SECTION_LEN
        merge_into(full, bm_src, Matrix.Translation((0, off, -rail_top)))
    full.to_mesh(me); full.free()
    me.transform(Matrix.Diagonal((SCALE * X_SCALE, SCALE, SCALE, 1)))
    for m in mat_list:
        me.materials.append(m)
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob

# 元オブジェクトは片付ける
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)

tile = make_object('TrainTunnelSection', section, 1)
loop = make_object('TrainTunnel', section, COUNT)
section.free()

for ob, fname in ((tile, 'TrainTunnelSection.fbx'), (loop, 'TrainTunnel.fbx')):
    bpy.ops.object.select_all(action='DESELECT')
    ob.select_set(True); bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, fname), **fbx_kw)
    xs = [v.co for v in ob.data.vertices]
    print("OUT", fname, "tris", sum(len(p.vertices) - 2 for p in ob.data.polygons),
          "min", [round(min(c[i] for c in xs), 2) for i in range(3)],
          "max", [round(max(c[i] for c in xs), 2) for i in range(3)])

# Unityで列車ステージを調整するときの目安用(軸の設定は TrainStage_Unity.fbx と同じ)
bpy.ops.object.select_all(action='DESELECT')
loop.select_set(True); bpy.context.view_layer.objects.active = loop
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, 'TrainTunnel_Unity.fbx'), use_selection=True, object_types={'MESH'},
                         apply_scale_options='FBX_SCALE_ALL', axis_forward='-Z', axis_up='Y', bake_space_transform=True,
                         mesh_smooth_type='FACE', bake_anim=False, path_mode='STRIP')

tile.hide_set(True); tile.hide_render = True
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'TrainTunnel.blend'))
