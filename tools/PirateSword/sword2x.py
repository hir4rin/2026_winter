# 海賊の剣のFBXを DxLib 用の .x に書き出す(ボスのmv1と同じ軸: x=x, y=Blenderのz, z=Blenderのy、×100でcm)
# 使い方: blender -b --python sword2x.py -- 入力.fbx 出力.x テクスチャ
import bpy, bmesh, sys
args = sys.argv[sys.argv.index('--') + 1:]
SRC, DST, TEX = args[0], args[1], args[2]
SCALE = 100.0
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SRC)
dg = bpy.context.evaluated_depsgraph_get()
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
def conv(p): return (p.x * SCALE, p.z * SCALE, p.y * SCALE)
def conv_n(n): return (n.x, n.z, n.y)
verts, norms, uvs, tris = [], [], [], []
for ob in meshes:
    me = ob.evaluated_get(dg).to_mesh()
    bm = bmesh.new(); bm.from_mesh(me); bmesh.ops.triangulate(bm, faces=bm.faces[:]); bm.to_mesh(me); bm.free()
    uvl = me.uv_layers.active.data if me.uv_layers.active else None
    mw = ob.matrix_world
    nm = mw.to_3x3().inverted().transposed()
    cn = me.corner_normals if hasattr(me, 'corner_normals') else None
    for poly in me.polygons:
        idx = []
        for li in poly.loop_indices:
            loop = me.loops[li]
            p = mw @ me.vertices[loop.vertex_index].co
            n = (nm @ (cn[li].vector if cn else loop.normal)).normalized()
            uv = uvl[li].uv if uvl else (0, 0)
            verts.append(conv(p)); norms.append(conv_n(n)); uvs.append((uv[0], 1.0 - uv[1]))
            idx.append(len(verts) - 1)
        tris.append(idx)
    ob.evaluated_get(dg).to_mesh_clear()
L = ["xof 0303txt 0032", "",
     "Material Mat0 {", " 1.000000;1.000000;1.000000;1.000000;;", " 10.000000;", " 0.000000;0.000000;0.000000;;",
     " 0.000000;0.000000;0.000000;;", ' TextureFilename { "%s"; }' % TEX, "}", "",
     "Frame Sword {", " FrameTransformMatrix { 1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0;; }", " Mesh {",
     "  %d;" % len(verts)]
L += ["  %f;%f;%f;%s" % (v[0], v[1], v[2], ";" if i == len(verts) - 1 else ",") for i, v in enumerate(verts)]
L.append("  %d;" % len(tris))
L += ["  3;%d,%d,%d;%s" % (t[0], t[1], t[2], ";" if i == len(tris) - 1 else ",") for i, t in enumerate(tris)]
L += ["  MeshNormals {", "   %d;" % len(norms)]
L += ["   %f;%f;%f;%s" % (n[0], n[1], n[2], ";" if i == len(norms) - 1 else ",") for i, n in enumerate(norms)]
L.append("   %d;" % len(tris))
L += ["   3;%d,%d,%d;%s" % (t[0], t[1], t[2], ";" if i == len(tris) - 1 else ",") for i, t in enumerate(tris)]
L += ["  }", "  MeshTextureCoords {", "   %d;" % len(uvs)]
L += ["   %f;%f;%s" % (u[0], u[1], ";" if i == len(uvs) - 1 else ",") for i, u in enumerate(uvs)]
L += ["  }", "  MeshMaterialList {", "   1;", "   %d;" % len(tris)]
L += ["   0%s" % (";;" if i == len(tris) - 1 else ",") for i in range(len(tris))]
L += ["   {Mat0}", "  }", " }", "}"]
open(DST, "w").write("\n".join(L) + "\n")
print("SWORD verts", len(verts), "tris", len(tris))
