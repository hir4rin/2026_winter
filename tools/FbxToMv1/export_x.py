# Blenderのメッシュを DxLib で読める .x(テキスト)に書き出す。convert.exe で mv1 にする前段
# 使い方: blender -b 入力.blend --python export_x.py -- 出力フォルダ オブジェクト名 [オブジェクト名 ...] [dif=倍率]
# テクスチャ付きのマテリアルは Base Color の値(テクスチャに掛ける色)を拡散色にする
# 座標: Blender(x, y, z)[m] → .x(x, z, y)[cm]。BlenderのYがゲームのZ、BlenderのZがゲームのY
import bpy, os, sys

argv = sys.argv[sys.argv.index('--') + 1:]
OUT = argv[0]
SCALE = 100.0
DIF_SCALE = float(next((a[4:] for a in argv if a.startswith("dif=")), 1.0))   # 拡散色に掛ける倍率(dif=0.6 のように指定)
NAMES = [a for a in argv[1:] if not a.startswith("dif=")]

def mat_info(m, out_dir):
    dif, emi, tex = (0.8, 0.8, 0.8), (0.0, 0.0, 0.0), None
    if m is None:
        return tuple(c * DIF_SCALE for c in dif), emi, tex
    dif = tuple(m.diffuse_color[:3])
    if m.use_nodes:
        p = next((n for n in m.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        if p:
            bc = p.inputs['Base Color']
            dif = tuple(bc.default_value[:3])
            if bc.is_linked and bc.links[0].from_node.type == 'TEX_IMAGE' and bc.links[0].from_node.image:
                img = bc.links[0].from_node.image
                tex = os.path.relpath(bpy.path.abspath(img.filepath), out_dir).replace('\\', '/')
            st = p.inputs['Emission Strength'].default_value
            if st > 0:
                emi = tuple(min(1.0, c * st * 0.5) for c in p.inputs['Emission Color'].default_value[:3])
    return tuple(c * DIF_SCALE for c in dif), emi, tex

def export(ob, path):
    out_dir = os.path.dirname(path)
    dg = bpy.context.evaluated_depsgraph_get()
    me = ob.evaluated_get(dg).to_mesh()
    me.transform(ob.matrix_world)
    me.calc_loop_triangles()
    uv = me.uv_layers.active.data if me.uv_layers.active else None
    normals = me.corner_normals
    verts, index, faces, fmat = [], {}, [], []
    for t in me.loop_triangles:
        ids = []
        for li in t.loops:
            co = me.vertices[me.loops[li].vertex_index].co
            n = normals[li].vector
            u = uv[li].uv if uv else (0.0, 0.0)
            key = (round(co.x, 4), round(co.y, 4), round(co.z, 4), round(n.x, 3), round(n.y, 3), round(n.z, 3), round(u[0], 4), round(u[1], 4))
            if key not in index:
                index[key] = len(verts)
                verts.append((co.x * SCALE, co.z * SCALE, co.y * SCALE, n.x, n.z, n.y, u[0], 1.0 - u[1]))
            ids.append(index[key])
        faces.append(ids[::-1])   # 回り順を逆にする(そのままだと当たり判定の面の法線が裏向きになる)
        fmat.append(t.material_index)
    mats = [mat_info(m, out_dir) for m in me.materials] or [mat_info(None, out_dir)]
    L = ["xof 0303txt 0032\n"]
    for i, (dif, emi, tex) in enumerate(mats):
        L.append("Material M%d {\n %.4f;%.4f;%.4f;1.0;;\n 10.0;\n 0.0;0.0;0.0;;\n %.4f;%.4f;%.4f;;\n" % (i, *dif, *emi))
        if tex:
            L.append(' TextureFilename { "%s"; }\n' % tex)
        L.append("}\n")
    L.append("Frame %s {\n FrameTransformMatrix { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1;; }\n Mesh {\n  %d;\n" % (ob.name.replace(' ', '_'), len(verts)))
    L.append(",\n".join("  %.3f;%.3f;%.3f;" % v[:3] for v in verts) + ";\n")
    L.append("  %d;\n" % len(faces))
    L.append(",\n".join("  3;%d,%d,%d;" % tuple(f) for f in faces) + ";\n")
    L.append("  MeshNormals {\n   %d;\n" % len(verts))
    L.append(",\n".join("   %.4f;%.4f;%.4f;" % v[3:6] for v in verts) + ";\n")
    L.append("   %d;\n" % len(faces))
    L.append(",\n".join("   3;%d,%d,%d;" % tuple(f) for f in faces) + ";\n  }\n")
    L.append("  MeshTextureCoords {\n   %d;\n" % len(verts))
    L.append(",\n".join("   %.5f;%.5f;" % v[6:8] for v in verts) + ";\n  }\n")
    L.append("  MeshMaterialList {\n   %d;\n   %d;\n" % (len(mats), len(faces)))
    L.append(",\n".join("   %d" % min(m, len(mats) - 1) for m in fmat) + ";\n")
    L.append("".join("   {M%d}\n" % i for i in range(len(mats))) + "  }\n }\n}\n")
    with open(path, 'w', encoding='utf-8') as f:
        f.write("".join(L))
    ob.evaluated_get(dg).to_mesh_clear()
    print("XOUT", path, "verts", len(verts), "tris", len(faces), "mats", len(mats))

for name in NAMES:
    export(bpy.data.objects[name], os.path.join(OUT, name + '.x'))
