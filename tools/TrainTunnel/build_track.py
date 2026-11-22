# トンネルの外で使う「線路だけ」のモデルを作る(トンネルの出口より先に、トンネルと同じ位置・間隔で並べる)
# 使い方: blender -b TrainTunnel.blend --python build_track.py -- 出力フォルダ SubwaySection.fbx
#   TrainTunnel.blend / SubwaySection.fbx は build_tunnel.py が作ったもの(data/Stage/TrainTunnel)
#   出力: 出力フォルダ/TrainTrack.blend(オブジェクト TrainTrack)。export_x.py → convert.exe flat で mv1 にする
# 並べ方・大きさは build_tunnel.py と同じ(build_all.sh の引数 11 4 2 = SCALE COUNT X_SCALE)
import bpy, bmesh, os, sys
from mathutils import Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
OUT = argv[0]
SECTION_FBX = argv[1]
SCALE = 11.0
COUNT = 4
X_SCALE = 2.0
SECTION_LEN = 3.0
RAIL_CENTER_X = 0.065
FLOOR = ['Rails', 'wires2', 'wires3', 'bolts', 'underground pipes']   # build_tunnel.py と同じ
DECK_MARGIN = 0.22      # 線路の外側に出す高架の床の幅(元モデルの単位)
DECK_THICK = 0.16       # 高架の床の厚さ
DECK_BELOW = 0.02       # レールの下の端から床の上面まで

# TrainTunnel.blend の材質(sRGBに直してある)を名前で使う
base_mats = {m.name: m for m in bpy.data.materials}
def base_mat(m):
    if m is None:
        return None
    return base_mats.get(m.name.split('.')[0], m)

before = set(bpy.data.objects)
bpy.ops.import_scene.fbx(filepath=SECTION_FBX)
imported = [o for o in bpy.data.objects if o not in before]
dg = bpy.context.evaluated_depsgraph_get()

mat_list = []
def mat_index(m):
    m = base_mat(m)
    if m not in mat_list:
        mat_list.append(m)
    return mat_list.index(m)

section = bmesh.new()
rail_top = -1e9
rail_bottom = 1e9
xmin, xmax = 1e9, -1e9
for o in imported:
    if o.type != 'MESH' or o.name.split('.')[0] not in FLOOR:
        continue
    me = bpy.data.meshes.new_from_object(o.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
    me.transform(o.matrix_world)
    me.transform(Matrix.Translation((-RAIL_CENTER_X, 0, 0)))
    remap = [mat_index(m) for m in me.materials] or [0]
    if o.name.split('.')[0] == 'Rails':
        rail_top = max(v.co.z for v in me.vertices)
        rail_bottom = min(v.co.z for v in me.vertices)
        xmin = min(v.co.x for v in me.vertices)
        xmax = max(v.co.x for v in me.vertices)
    tmp = bmesh.new(); tmp.from_mesh(me)
    vmap = {v: section.verts.new(v.co) for v in tmp.verts}
    for f in tmp.faces:
        try:
            nf = section.faces.new([vmap[v] for v in f.verts])
            nf.material_index = remap[min(f.material_index, len(remap) - 1)]
        except ValueError:
            pass
    tmp.free(); bpy.data.meshes.remove(me)

# 高架の床(レールの下のコンクリートの箱)
deck_mat = mat_index(base_mats.get('concrete'))
x0, x1 = xmin - DECK_MARGIN, xmax + DECK_MARGIN
z1 = rail_bottom - DECK_BELOW
z0 = z1 - DECK_THICK
y0, y1 = -SECTION_LEN / 2, SECTION_LEN / 2
c = [section.verts.new(p) for p in [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
                                     (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]]
for q in ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)):
    f = section.faces.new([c[i] for i in q])
    f.material_index = deck_mat
bmesh.ops.recalc_face_normals(section, faces=section.faces[:])
print("TRACK rail_top", rail_top, "rail_bottom", rail_bottom, "x", xmin, xmax, "faces", len(section.faces))

me = bpy.data.meshes.new('TrainTrack')
full = bmesh.new()
for k in range(COUNT):
    off = (k - (COUNT - 1) / 2) * SECTION_LEN
    tmp = bpy.data.meshes.new('tmp'); section.to_mesh(tmp)
    tmp.transform(Matrix.Translation((0, off, -rail_top)))
    b = bmesh.new(); b.from_mesh(tmp)
    vmap = {v: full.verts.new(v.co) for v in b.verts}
    for f in b.faces:
        try:
            nf = full.faces.new([vmap[v] for v in f.verts])
            nf.material_index = f.material_index
        except ValueError:
            pass
    b.free(); bpy.data.meshes.remove(tmp)
full.to_mesh(me); full.free(); section.free()
me.transform(Matrix.Diagonal((SCALE * X_SCALE, SCALE, SCALE, 1)))
for m in mat_list:
    me.materials.append(m)

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
ob = bpy.data.objects.new('TrainTrack', me)
bpy.context.scene.collection.objects.link(ob)
xs = [v.co for v in me.vertices]
print("OUT TrainTrack tris", sum(len(p.vertices) - 2 for p in me.polygons),
      "min", [round(min(v[i] for v in xs), 2) for i in range(3)], "max", [round(max(v[i] for v in xs), 2) for i in range(3)])
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'TrainTrack.blend'))
