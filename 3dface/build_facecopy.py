"""
Low-poly + hair + animated version of a scanned head GLB (glTF 2.0).

Run inside Blender (5.x) — standalone script, also callable from the MCP server:

    exec(open(r"C:/Github/dev-dialogue/3dface/build_facecopy.py").read(),
         {"__name__": "__main__"})

Steps:
  1. import the scan
  2. bake the imported node transform, normalise scale (head = 0.23 m) and
     pivot (neck base)
  3. decimate to a low-poly budget (textures + UVs kept)
  4. build a low-poly hair cap from the scalp
  5. shrink oversized textures to 1k
  6. add head animation clips (Idle / Nod / Turn / Talk)
  7. export GLB + .blend and render previews
"""

import importlib.util
import math
import os
import sys

import bpy
import bmesh
from mathutils import Matrix, Vector

DIR = "C:/Github/dev-dialogue/3dface"
COLL_NAME = "FaceCopyLow"
TARGET_TRIS = 6000
HEAD_HEIGHT = 0.23          # metres, chin -> crown
HAIR_COLOR = (0.045, 0.030, 0.022)

_spec = importlib.util.spec_from_file_location("bf", os.path.join(DIR, "build_face.py"))
bf = importlib.util.module_from_spec(_spec)
sys.modules["bf"] = bf
_spec.loader.exec_module(bf)


# --------------------------------------------------------------------------
# helpers
# --------------------------------------------------------------------------

def get_collection():
    col = bpy.data.collections.get(COLL_NAME)
    if col is None:
        col = bpy.data.collections.new(COLL_NAME)
        bpy.context.scene.collection.children.link(col)
    return col


def clear_collection():
    col = bpy.data.collections.get(COLL_NAME)
    if not col:
        return
    for ob in list(col.objects):
        bpy.data.objects.remove(ob, do_unlink=True)


def head_metrics(me, up=2):
    """chin / crown / centre of the head in mesh local space"""
    verts = me.vertices
    ups = [v.co[up] for v in verts]
    umin, umax = min(ups), max(ups)
    bins = 80
    step = (umax - umin) / bins
    widths = []
    for i in range(bins):
        u0 = umin + i * step
        u1 = u0 + step
        w = 0.0
        for v in verts:
            if u0 <= v.co[up] < u1:
                w = max(w, abs(v.co.x))
        widths.append(((u0 + u1) * 0.5, w))
    max_w = max(w for _, w in widths)
    chin = umin
    for u, w in widths:                       # walk up until the head widens out
        if w > 0.55 * max_w:
            chin = u
            break
    head = [v.co for v in verts if v.co[up] >= chin]
    x_c = sum(v.x for v in head) / len(head)
    y_c = (min(v.y for v in head) + max(v.y for v in head)) * 0.5
    return chin, umax, x_c, y_c


def decimate(ob, target_tris):
    bf.activate(ob)
    bpy.ops.object.shade_smooth()
    tris = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    if tris > target_tris:
        mod = ob.modifiers.new("Decimate", "DECIMATE")
        mod.ratio = target_tris / tris
        mod.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return sum(len(p.vertices) - 2 for p in ob.data.polygons)


def build_hair(head, col, chin, crown, x_c, y_c):
    """hair cap: a copy of the scalp, cut at a hairline and given volume"""
    hair = head.copy()
    hair.data = head.data.copy()
    hair.name = "Hair"
    hair.data.name = "Hair"
    col.objects.link(hair)
    bf.activate(hair)

    h = crown - chin

    def hairline_u(co):
        """lowest up-coordinate that still counts as hair, per position"""
        t = 0.52
        if co.y < y_c - 0.14 * h:              # forehead (Blender front = -Y)
            t = 0.66
        elif co.y > y_c + 0.18 * h:            # nape -> lower
            t = 0.42
        if abs(co.x - x_c) > 0.62 * 0.10:      # sideburns dip in front of the ears
            t = max(t, 0.58)
        return chin + t * h

    me = hair.data
    bm = bmesh.new()
    bm.from_mesh(me)
    kill = []
    for f in bm.faces:
        c = f.calc_center_median()
        if c.z < hairline_u(c):
            kill.append(f)
    bmesh.ops.delete(bm, geom=kill, context="FACES")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    me.update()

    for v in me.vertices:                    # lift the cap off the scalp
        v.co += v.normal * 0.001
    solid = hair.modifiers.new("Solidify", "SOLIDIFY")
    solid.thickness = 0.004
    solid.offset = 1.0
    bpy.ops.object.modifier_apply(modifier=solid.name)
    decimate(hair, 2200)                     # keep the cap low-poly too

    mat = bf.material("HairDark", HAIR_COLOR, 0.45)
    me.materials.clear()
    me.materials.append(mat)
    bpy.ops.object.shade_smooth()
    return hair


def add_animations(mesh):
    """head motion clips on the head object (Z up: X = pitch, Z = yaw)"""
    actions = []
    scene = bpy.context.scene
    if mesh.animation_data:                 # drop tracks of earlier runs
        mesh.animation_data.nla_tracks.clear()
    for name, keys in (
        ("Idle", [(1, 0.0), (41, 1.0), (81, 0.0), (121, -1.0), (161, 0.0)]),
        ("Nod", [(1, 0.0), (14, 1.0), (27, -0.3), (40, 0.0)]),
        ("Turn", [(1, 0.0), (25, 1.0), (50, -1.0), (75, 0.0)]),
        ("Talk", [(1, 0.0), (9, 1.0), (17, -0.6), (25, 0.8), (33, 0.0)]),
    ):
        act = bf.fresh_action(name)
        ad = mesh.animation_data or mesh.animation_data_create()
        slot = act.slots.new(id_type="OBJECT", name="Object")
        ad.action = act
        ad.action_slot = slot
        mesh.rotation_mode = "XYZ"
        for f, w in keys:
            if name == "Nod":
                mesh.rotation_euler = (math.radians(9.0) * w, 0.0, 0.0)
            elif name == "Turn":
                mesh.rotation_euler = (0.0, 0.0, math.radians(16.0) * w)
            elif name == "Talk":
                mesh.rotation_euler = (math.radians(2.5) * w, math.radians(1.0) * w,
                                       math.radians(3.0) * w)
            else:
                mesh.rotation_euler = (math.radians(1.2) * w, math.radians(1.0) * w,
                                       math.radians(3.0) * w)
            mesh.keyframe_insert("rotation_euler", frame=f)
        track = ad.nla_tracks.new()
        strip = track.strips.new(act.name, 1, act)
        strip.action_slot = slot
        actions.append(act)
    ad.action = None
    scene.frame_start = 1
    scene.frame_end = 120
    return actions


# --------------------------------------------------------------------------

def main(src=None, out_glb=None, out_blend=None):
    src = src or os.path.join(DIR, "face copy.glb")
    out_glb = out_glb or os.path.join(DIR, "face copy lowpoly.glb")
    out_blend = out_blend or os.path.join(DIR, "face copy lowpoly.blend")
    if not os.path.exists(src):
        return {"skipped": "source not found: " + src}

    os.makedirs(DIR, exist_ok=True)
    col = get_collection()
    clear_collection()

    layer_col = bpy.context.view_layer.layer_collection.children.get(COLL_NAME)
    if layer_col:
        bpy.context.view_layer.active_layer_collection = layer_col
    before = set(bpy.data.objects.keys())
    bpy.ops.import_scene.gltf(filepath=src)
    imported = [o for o in bpy.data.objects if o.name not in before]
    for o in imported:
        for c in list(o.users_collection):
            c.objects.unlink(o)
        col.objects.link(o)

    meshes = [o for o in imported if o.type == "MESH"]
    mesh = max(meshes, key=lambda o: len(o.data.polygons))
    source_tris = sum(len(p.vertices) - 2 for o in meshes for p in o.data.polygons)

    # ---- bake the imported node transform (Z up, front = -Y) ----
    bpy.context.view_layer.update()
    mw = mesh.matrix_world.copy()
    for v in mesh.data.vertices:
        v.co = mw @ v.co
    mesh.data.update()
    mesh.location = (0, 0, 0)
    mesh.rotation_mode = "XYZ"
    mesh.rotation_euler = (0, 0, 0)
    mesh.scale = (1, 1, 1)
    for o in imported:
        if o.type == "EMPTY":
            o.location = (0, 0, 0)
            o.rotation_mode = "XYZ"
            o.rotation_euler = (0, 0, 0)
            o.scale = (1, 1, 1)
            o.matrix_parent_inverse = Matrix.Identity(4)
    bpy.context.view_layer.update()

    # ---- normalise scale + pivot (mesh local space, Z up) ----
    chin, crown, x_c, y_c = head_metrics(mesh.data, up=2)
    h = crown - chin
    s = HEAD_HEIGHT / h
    pivot = Vector((x_c, y_c, chin - 0.10 * h))
    for v in mesh.data.vertices:
        v.co = (v.co - pivot) * s          # pivot lands on the origin
    mesh.data.update()

    chin, crown, x_c, y_c = head_metrics(mesh.data, up=2)
    h = crown - chin

    # ---- low poly ----
    tris = decimate(mesh, TARGET_TRIS)

    # ---- hair ----
    hair = build_hair(mesh, col, chin, crown, x_c, y_c)
    hair.parent = mesh
    hair.matrix_parent_inverse = mesh.matrix_basis.inverted()

    # ---- texture budget ----
    resized = []
    for img in bpy.data.images:
        if img.size[0] > 1024 and img.name.startswith("Image"):
            img.scale(1024, 1024)
            resized.append(img.name)

    # ---- animation ----
    add_animations(mesh)

    # ---- export ----
    bpy.context.view_layer.update()
    bpy.ops.object.select_all(action="DESELECT")
    for o in imported:
        o.select_set(True)
    hair.select_set(True)
    bpy.context.view_layer.objects.active = mesh
    bpy.ops.export_scene.gltf(
        filepath=out_glb,
        export_format="GLB",
        use_selection=True,
        export_animations=True,
        export_animation_mode="ACTIONS",
        export_nla_strips=True,
        export_apply=True,
        export_yup=True,
        export_image_format="WEBP",
        export_image_quality=85,
        export_texcoords=True,
        export_normals=True,
        export_extras=True,
    )
    bf.purge_orphans()
    bpy.ops.wm.save_as_mainfile(filepath=out_blend, copy=True)

    previews = bf.render_previews([mesh, hair], names=("front", "3q", "side"),
                                  prefix="preview_copy")
    bf.focus_viewport([mesh, hair])

    return {
        "source": src,
        "source_tris": source_tris,
        "head_tris": tris,
        "hair_tris": sum(len(p.vertices) - 2 for p in hair.data.polygons),
        "head_height_m": round(h, 4),
        "glb": out_glb,
        "glb_size": os.path.getsize(out_glb),
        "resized_textures": resized,
        "previews": previews,
        "objects": [o.name for o in col.objects],
    }


if __name__ == "__main__":
    result = main()
