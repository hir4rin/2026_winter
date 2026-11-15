# 列車の通路車両の入口をふさぐ扉: 赤さびの波板の仕切り + 貨車の扉(WagonDoor、列車のアセット)を左右に切った引き戸
# 使い方: blender -b --python build_train_bulkhead.py -- 出力フォルダ(data/Stage/Gates/Train) WagonDoor.fbx
#   出力: 出力フォルダ/TrainBulkhead.blend(Bulkhead / WagonDoor_L / WagonDoor_R)と parts.txt(板の位置)
#   テクスチャは 出力フォルダ/tex の roof_rust.png / hazard.png / wagon_door.jpg を使う(先に写しておく)
# 座標: Blender(x, y, z)[m] → ゲーム(x, z, y)[cm]。原点 = 入口の真ん中・床・仕切りの表の面。-Y がゲームの -Z(列車の前、プレイヤーが来る側)
import bpy, bmesh, os, sys
from mathutils import Matrix, Vector

argv = sys.argv[sys.argv.index('--') + 1:]
OUT, FBX = argv[0], argv[1]
TEX = os.path.join(OUT, 'tex')
OPEN_HALF = 6.21      # 車両の入口の幅の半分(車内の壁 CORR_INNER_X)
OPEN_H = 5.70         # 入口の高さ(車内の天井)
PANEL_T = 0.05        # 仕切りの厚さ
TAPE_W = 0.12         # 黄黒のテープの幅

bpy.ops.wm.read_factory_settings(use_empty=True)

def mat(name, tex, color=(1, 1, 1)):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    p = m.node_tree.nodes['Principled BSDF']
    p.inputs['Base Color'].default_value = (*color, 1)
    n = m.node_tree.nodes.new('ShaderNodeTexImage')
    n.image = bpy.data.images.load(os.path.join(TEX, tex))
    m.node_tree.links.new(n.outputs['Color'], p.inputs['Base Color'])
    return m

RUST = mat('roof rust', 'roof_rust.png')
HAZARD = mat('hazard', 'hazard.png')

# ---- 貨車の扉 ----
bpy.ops.import_scene.fbx(filepath=FBX)
door = [o for o in bpy.context.scene.objects if o.type == 'MESH'][0]
door.data.transform(door.matrix_world)
door.matrix_world = Matrix.Identity(4)
me = door.data
# 幅が X になるように回す(一番長い横の向きを X に)
xs = [v.co.x for v in me.vertices]; ys = [v.co.y for v in me.vertices]
if max(ys) - min(ys) > max(xs) - min(xs):
    me.transform(Matrix.Rotation(1.5707963, 4, 'Z'))
# 表(面積で重みを付けた法線の向き)を -Y(プレイヤーが来る側)へ
n = Vector((0, 0, 0))
for p in me.polygons:
    n += p.normal * p.area
if n.y > 0:
    me.transform(Matrix.Rotation(3.14159265, 4, 'Z'))
bb = [v.co for v in me.vertices]
lo = Vector((min(c.x for c in bb), min(c.y for c in bb), min(c.z for c in bb)))
hi = Vector((max(c.x for c in bb), max(c.y for c in bb), max(c.z for c in bb)))
print('WAGON size', hi - lo, 'normal', n)
DOOR_HALF = (hi.x - lo.x) / 2
DOOR_H = hi.z - lo.z
# 真ん中・床・仕切りの裏(+Y)へ寄せる
me.transform(Matrix.Translation((-(lo.x + hi.x) / 2, PANEL_T + 0.03 - lo.y, -lo.z)))
for m in me.materials:
    for nd in m.node_tree.nodes:
        if nd.type == 'TEX_IMAGE' and nd.image:
            nd.image.filepath = os.path.join(TEX, 'wagon_door.jpg')
        if nd.type == 'BSDF_PRINCIPLED':
            nd.inputs['Base Color'].default_value = (0.8, 0.8, 0.8, 1)  # export_props.py の TINT['door']

lines = []
def half(side, name):
    b = bmesh.new()
    b.from_mesh(me)
    geom = b.verts[:] + b.edges[:] + b.faces[:]
    bmesh.ops.bisect_plane(b, geom=geom, plane_co=(0, 0, 0), plane_no=(side, 0, 0), clear_inner=True)
    bmesh.ops.delete(b, geom=[v for v in b.verts if not v.link_faces], context='VERTS')
    cx = sum(v.co.x for v in b.verts) and (min(v.co.x for v in b.verts) + max(v.co.x for v in b.verts)) / 2
    bmesh.ops.translate(b, verts=b.verts, vec=(-cx, 0, 0))
    m2 = bpy.data.meshes.new(name)
    b.to_mesh(m2)
    for m in me.materials:
        m2.materials.append(m)
    b.free()
    ob = bpy.data.objects.new(name, m2)
    bpy.context.scene.collection.objects.link(ob)
    lines.append('%s %.1f' % (name, cx * 100))
half(-1, 'WagonDoor_L')
half(1, 'WagonDoor_R')
bpy.data.objects.remove(door)

# ---- 仕切り(赤さびの波板)と黄黒のテープ ----
verts, faces, uvs, fmat = [], [], [], []
def box(mn, mx, mi, uvscale=(1.0, 1.0)):
    x, y, z = (mn[0], mx[0]), (mn[1], mx[1]), (mn[2], mx[2])
    c = lambda i, j, k: (x[i], y[j], z[k])
    quads = [((c(1,0,0), c(1,1,0), c(1,1,1), c(1,0,1)), (1, 2)), ((c(0,0,0), c(0,0,1), c(0,1,1), c(0,1,0)), (1, 2)),
             ((c(0,1,0), c(0,1,1), c(1,1,1), c(1,1,0)), (0, 2)), ((c(0,0,0), c(1,0,0), c(1,0,1), c(0,0,1)), (0, 2)),
             ((c(0,0,1), c(1,0,1), c(1,1,1), c(0,1,1)), (0, 1)), ((c(0,0,0), c(0,1,0), c(1,1,0), c(1,0,0)), (0, 1))]
    for vs, (au, av) in quads:
        base = len(verts)
        verts.extend(vs)
        faces.append((base, base + 1, base + 2, base + 3))
        uvs.append([(v[au] * uvscale[0], v[av] * uvscale[1]) for v in vs])
        fmat.append(mi)
H = DOOR_H + 0.05
box((-OPEN_HALF, 0, 0), (-DOOR_HALF, PANEL_T, OPEN_H), 0)
box((DOOR_HALF, 0, 0), (OPEN_HALF, PANEL_T, OPEN_H), 0)
box((-DOOR_HALF, 0, H), (DOOR_HALF, PANEL_T, OPEN_H), 0)
T = 1 / 0.35  # hazard.png は幅0.35mで1回
box((-DOOR_HALF - TAPE_W, -0.01, 0), (-DOOR_HALF, 0, H + TAPE_W), 1, (T, T))
box((DOOR_HALF, -0.01, 0), (DOOR_HALF + TAPE_W, 0, H + TAPE_W), 1, (T, T))
box((-DOOR_HALF - TAPE_W, -0.01, H), (DOOR_HALF + TAPE_W, 0, H + TAPE_W), 1, (T, T))
bm_me = bpy.data.meshes.new('Bulkhead')
bm_me.from_pydata(verts, [], faces)
bm_me.materials.append(RUST)
bm_me.materials.append(HAZARD)
uvl = bm_me.uv_layers.new()
for poly, uv, mi in zip(bm_me.polygons, uvs, fmat):
    poly.material_index = mi
    for li, u in zip(poly.loop_indices, uv):
        uvl.data[li].uv = u
bpy.context.scene.collection.objects.link(bpy.data.objects.new('Bulkhead', bm_me))
lines.append('Bulkhead 0 doorHalf %.1f doorH %.1f' % (DOOR_HALF * 100, H * 100))

with open(os.path.join(OUT, 'parts.txt'), 'w') as f:
    f.write('# name centerX(cm)\n' + '\n'.join(lines) + '\n')
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'TrainBulkhead.blend'))
print('BULKHEAD_DONE', lines)
