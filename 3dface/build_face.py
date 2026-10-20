"""
Low-poly male head ("face") generator for Dev Dialogue.

Run inside Blender (5.x) — standalone script, also callable from the Blender
MCP server:

    exec(open(r"C:/Github/dev-dialogue/3dface/build_face.py").read(),
         {"__name__": "__main__"})

Produces:
  * 3dface/face.glb   — glTF 2.0 binary, ready for three.js GLTFLoader
  * 3dface/face.blend — the editable source
  * 3dface/preview_face_*.png

Pipeline: metaball skull -> analytic feature sculpt -> mouth cavity cut ->
decimate (low poly, flat shaded) -> armature (head/jaw) -> animations ->
glTF export.

Model layout (Blender axes, front = -Y, up = +Z, origin = neck base).
"""

import math
import os

import bpy
import bmesh
from mathutils import Matrix, Quaternion, Vector

OUT_DIR = "C:/Github/dev-dialogue/3dface"
COLL_NAME = "3DFace"
TARGET_TRIS = 3600
METABALL_RES = 0.005

# A metaball BALL of influence radius R renders a surface of radius ~0.5734*R
SURF = 0.5734

EYE_X, EYE_Z = 0.030, 0.150
MOUTH_Z = 0.074


# --------------------------------------------------------------------------
# helpers
# --------------------------------------------------------------------------

def R(r):
    """desired surface radius -> metaball influence radius"""
    return r / SURF


def smoothstep(a, b, x):
    if a == b:
        return 0.0 if x < a else 1.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def band(x, lo, hi, soft=0.004):
    """1 inside [lo, hi], falling off over `soft` on both sides"""
    return smoothstep(lo - soft, lo, x) * smoothstep(hi + soft, hi, x)


def profile(x, points):
    """linear interpolation through (x, value) points, clamped at the ends"""
    if x <= points[0][0]:
        return points[0][1]
    for (x0, v0), (x1, v1) in zip(points, points[1:]):
        if x <= x1:
            t = (x - x0) / (x1 - x0) if x1 != x0 else 0.0
            return v0 + (v1 - v0) * t
    return points[-1][1]


def get_collection(name=COLL_NAME):
    col = bpy.data.collections.get(name)
    if col is None:
        col = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(col)
    return col


def clear_collection(name=COLL_NAME):
    col = bpy.data.collections.get(name)
    if not col:
        return
    for ob in list(col.objects):
        bpy.data.objects.remove(ob, do_unlink=True)
    for me in list(bpy.data.meshes):
        if me.users == 0:
            bpy.data.meshes.remove(me)


def link(ob, col):
    for c in list(ob.users_collection):
        c.objects.unlink(ob)
    col.objects.link(ob)


def activate(ob):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob


def material(name, color, roughness=0.7, metallic=0.0):
    mat = bpy.data.materials.get(name)
    if mat is None:
        mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    if "Specular IOR Level" in bsdf.inputs:
        bsdf.inputs["Specular IOR Level"].default_value = 0.2
    mat.diffuse_color = (*color, 1.0)
    return mat


def purge_orphans():
    """drop unused datablocks so the .blend stays small"""
    try:
        bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
    except Exception:
        for img in list(bpy.data.images):
            if img.users == 0:
                bpy.data.images.remove(img)


def fresh_action(name):
    """new action with a clean name (re-runs would otherwise get .002 suffixes)"""
    for act in list(bpy.data.actions):
        if act.name == name or act.name.startswith(name + "."):
            bpy.data.actions.remove(act)
    return bpy.data.actions.new(name)


def focus_viewport(objects=None, shading="MATERIAL"):
    """point every 3D viewport at the given objects (no operators, no local view)"""
    objs = objects or [o for o in bpy.context.scene.objects if o.type == "MESH"]
    pts = []
    for o in objs:
        pts += [o.matrix_world @ Vector(c) for c in o.bound_box]
    if not pts:
        return
    centre = Vector((sum(p.x for p in pts) / len(pts),
                     sum(p.y for p in pts) / len(pts),
                     sum(p.z for p in pts) / len(pts)))
    dist = max(max(p[i] for p in pts) - min(p[i] for p in pts) for i in range(3)) * 1.4
    front = Quaternion((0.7071068, 0.0, 0.0, 0.7071068))   # look along +Y
    for win in bpy.context.window_manager.windows:
        for a in win.screen.areas:
            if a.type != "VIEW_3D":
                continue
            space = a.spaces.active
            space.shading.type = shading
            r3d = space.region_3d
            r3d.view_perspective = "ORTHO"
            r3d.view_rotation = front
            r3d.view_location = centre
            r3d.view_distance = dist
            a.tag_redraw()


# --------------------------------------------------------------------------
# base mass
# --------------------------------------------------------------------------

# (center, desired surface radius) — skull/face mass only, features come later
ELEMENTS = [
    # cranium
    ((0.000, 0.010, 0.194), 0.074),
    ((0.000, 0.036, 0.170), 0.054),
    ((0.000, -0.030, 0.172), 0.050),
    ((0.000, 0.002, 0.218), 0.036),
    ((0.050, 0.008, 0.168), 0.030),
    ((-0.050, 0.008, 0.168), 0.030),
    # midface
    ((0.000, -0.012, 0.126), 0.054),
    ((0.042, -0.026, 0.138), 0.032),
    ((-0.042, -0.026, 0.138), 0.032),
    ((0.038, -0.026, 0.096), 0.030),
    ((-0.038, -0.026, 0.096), 0.030),
    # jaw
    ((0.044, 0.002, 0.068), 0.028),
    ((-0.044, 0.002, 0.068), 0.028),
    ((0.024, -0.014, 0.054), 0.026),
    ((-0.024, -0.014, 0.054), 0.026),
    ((0.000, -0.032, 0.040), 0.022),
    # neck
    ((0.000, 0.014, 0.014), 0.038),
    ((0.000, 0.016, -0.014), 0.038),
    # ears
    ((0.062, 0.006, 0.146), 0.011),
    ((0.062, 0.002, 0.126), 0.010),
    ((-0.062, 0.006, 0.146), 0.011),
    ((-0.062, 0.002, 0.126), 0.010),
]


def build_base_mesh(col):
    mb = bpy.data.metaballs.new("HeadMeta")
    mb.resolution = METABALL_RES
    mb.render_resolution = METABALL_RES
    mb.threshold = 0.6
    for co, r in ELEMENTS:
        e = mb.elements.new()
        e.co = co
        e.type = "BALL"
        e.radius = R(r)
        e.size_x = e.size_y = e.size_z = e.radius
    ob = bpy.data.objects.new("HeadMesh", mb)
    col.objects.link(ob)
    activate(ob)
    bpy.ops.object.convert(target="MESH")
    ob = bpy.context.active_object
    ob.name = "HeadMesh"
    ob.data.name = "HeadMesh"
    return ob


# --------------------------------------------------------------------------
# feature sculpt
# --------------------------------------------------------------------------

def sculpt(ob):
    """carve the face out of the metaball mass with analytic displacements"""
    me = ob.data
    for v in me.vertices:
        x, y, z = v.co
        if y > -0.012:          # only the front of the face is sculpted
            continue
        dy = 0.0

        # --- eye sockets: push the surface in, leaving a rim ---
        for sx in (1.0, -1.0):
            ex = x - sx * EYE_X
            r = math.hypot(ex, (z - EYE_Z) / 1.25)
            if r < 0.022:
                dy += 0.005 * smoothstep(0.022, 0.006, r)

        # --- brow ridge ---
        if abs(x) < 0.044:
            dy -= 0.005 * band(x, -0.044, 0.044, 0.014) * band(z, 0.156, 0.176, 0.010)

        # --- nose ---
        if abs(x) < 0.016:
            prof = profile(z, ((0.168, 0.0), (0.146, 0.008), (0.128, 0.016),
                               (0.114, 0.023), (0.102, 0.012), (0.094, 0.0)))
            dy -= prof * smoothstep(0.016, 0.009, abs(x))
        if 0.009 < abs(x) < 0.026:                 # nose wings
            dy -= 0.010 * band(x, 0.009, 0.026, 0.006) * band(z, 0.096, 0.113, 0.006)
        if abs(x) < 0.009:                        # philtrum
            dy += 0.004 * band(z, 0.088, 0.099, 0.005)

        # --- mouth ---
        if abs(x) < 0.020:
            dy += 0.006 * band(z, 0.0695, 0.0790, 0.0035)   # mouth line
            dy -= 0.0050 * band(z, 0.0795, 0.0900, 0.0040)  # upper lip
            dy -= 0.0055 * band(z, 0.0590, 0.0690, 0.0040)  # lower lip
        if 0.017 < abs(x) < 0.030:                # mouth corners
            dy += 0.005 * band(x, 0.017, 0.030, 0.006) * band(z, 0.066, 0.082, 0.005)
        if abs(x) < 0.024:                        # chin
            dy -= 0.005 * band(z, 0.036, 0.050, 0.005)
        if abs(x) < 0.030:                        # chin crease
            dy += 0.003 * band(z, 0.050, 0.060, 0.004)

        # --- eyelids ---
        for sx in (1.0, -1.0):
            ex = abs(x - sx * EYE_X)
            if ex < 0.020:
                dy -= 0.004 * band(ex, -0.020, 0.020, 0.005) * band(z, 0.1590, 0.1670, 0.0035)
                dy -= 0.003 * band(ex, -0.020, 0.020, 0.005) * band(z, 0.1335, 0.1410, 0.0035)

        v.co.y += dy
    me.update()


def smooth_mesh(ob, factor=0.4, iterations=1):
    me = ob.data
    for _ in range(iterations):
        bm = bmesh.new()
        bm.from_mesh(me)
        bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=factor, use_axis_x=True,
                              use_axis_y=True, use_axis_z=True)
        bm.to_mesh(me)
        bm.free()
    me.update()


def cut_mouth_cavity(ob):
    """open the mouth and extrude a cavity backwards"""
    me = ob.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()

    kill = []
    for f in bm.faces:
        c = f.calc_center_median()
        dx = c.x / 0.013
        dz = (c.z - MOUTH_Z) / 0.005
        if dx * dx + dz * dz < 1.0 and c.y < -0.030:
            kill.append(f)
    if not kill:
        bm.free()
        return 0
    bmesh.ops.delete(bm, geom=kill, context="FACES")

    edges = [e for e in bm.edges if e.is_boundary]
    if edges:
        ret = bmesh.ops.extrude_edge_only(bm, edges=edges)
        new_verts = [g for g in ret["geom"] if isinstance(g, bmesh.types.BMVert)]
        bmesh.ops.translate(bm, verts=new_verts, vec=(0.0, 0.030, 0.0))
        far_edges = [e for e in bm.edges if e.is_boundary]
        if far_edges:
            bmesh.ops.holes_fill(bm, edges=far_edges)
    bm.to_mesh(me)
    bm.free()
    me.update()

    for p in me.polygons:                      # tag the cavity with material 1
        c = p.center
        if c.y > -0.040 and abs(c.x) < 0.020 and MOUTH_Z - 0.012 < c.z < MOUTH_Z + 0.012:
            p.material_index = 1
    me.update()
    return len(kill)


def decimate(ob, target_tris):
    activate(ob)
    bpy.ops.object.shade_smooth()
    tris = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    if tris > target_tris:
        mod = ob.modifiers.new("Decimate", "DECIMATE")
        mod.ratio = target_tris / tris
        mod.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.ops.object.shade_flat()
    return sum(len(p.vertices) - 2 for p in ob.data.polygons)


# --------------------------------------------------------------------------
# parts
# --------------------------------------------------------------------------

def build_uv_sphere(name, loc, scale, col, mat, segs=(16, 10), rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segs[0], ring_count=segs[1],
                                         radius=1.0, location=loc)
    ob = bpy.context.active_object
    ob.name = name
    ob.data.name = name
    ob.scale = scale
    ob.rotation_euler = rot
    link(ob, col)
    ob.data.materials.append(mat)
    activate(ob)
    bpy.ops.object.shade_smooth()
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return ob


def build_box(name, loc, size, col, mat, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc)
    ob = bpy.context.active_object
    ob.name = name
    ob.data.name = name
    ob.scale = size
    ob.rotation_euler = rot
    link(ob, col)
    ob.data.materials.append(mat)
    activate(ob)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return ob


def build_empty(name, loc, col):
    ob = bpy.data.objects.new(name, None)
    ob.empty_display_type = "PLAIN_AXES"
    ob.empty_display_size = 0.03
    ob.location = loc
    col.objects.link(ob)
    return ob


def surface_y(head, x, z):
    """y of the head surface at (x, z), cast from inside the head towards -Y"""
    hit, loc, _, _ = head.ray_cast(Vector((x, 0.05, z)), Vector((0, -1, 0)))
    return loc.y if hit else -0.06


def head_metrics(me, up=2):
    """chin / crown / centre of the head in mesh local space"""
    verts = me.vertices
    ups = [v.co[up] for v in verts]
    umin, umax = min(ups), max(ups)
    bins = 60
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
    for u, w in widths:                    # walk up until the head widens out
        if w > 0.55 * max_w:
            chin = u
            break
    head = [v.co for v in verts if v.co[up] >= chin]
    x_c = sum(v.x for v in head) / len(head)
    y_c = (min(v.y for v in head) + max(v.y for v in head)) * 0.5
    return chin, umax, x_c, y_c


def build_hair(head, col, chin, crown, x_c, y_c):
    """hair cap: a copy of the scalp, cut at a hairline and given volume"""
    hair = head.copy()
    hair.data = head.data.copy()
    hair.name = "Hair"
    hair.data.name = "Hair"
    col.objects.link(hair)
    activate(hair)

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
    decimate(hair, 1500)                     # keep the cap low-poly too

    mat = material("HairDark", (0.045, 0.030, 0.022), 0.45)
    me.materials.clear()
    me.materials.append(mat)
    bpy.ops.object.shade_smooth()
    return hair


# --------------------------------------------------------------------------
# rig
# --------------------------------------------------------------------------

def build_armature(col):
    arm_data = bpy.data.armatures.new("Rig")
    arm = bpy.data.objects.new("Rig", arm_data)
    col.objects.link(arm)
    activate(arm)
    bpy.ops.object.mode_set(mode="EDIT")
    head = arm_data.edit_bones.new("head")
    head.head = (0.0, 0.0, 0.0)
    head.tail = (0.0, 0.0, 0.12)
    jaw = arm_data.edit_bones.new("jaw")
    jaw.head = (-0.055, 0.0, 0.082)
    jaw.tail = (0.055, 0.0, 0.082)
    jaw.parent = head
    jaw.use_connect = False
    bpy.ops.object.mode_set(mode="OBJECT")
    return arm


def skin_weights(head_mesh):
    """lower front of the face follows the jaw, the rest follows the head"""
    vg_head = head_mesh.vertex_groups.new(name="head")
    vg_jaw = head_mesh.vertex_groups.new(name="jaw")
    me = head_mesh.data
    cavity = set()
    for p in me.polygons:
        if p.material_index == 1:
            cavity.update(p.vertices)
    for v in me.vertices:
        if v.index in cavity:            # mouth cavity stays with the head
            continue
        w = smoothstep(0.082, 0.058, v.co.z) * smoothstep(-0.005, -0.030, v.co.y)
        if w > 0.001:
            vg_jaw.add([v.index], w, "REPLACE")
        if w < 0.999:
            vg_head.add([v.index], 1.0 - w, "REPLACE")


def parent_to_bone(child, arm, bone_name):
    """bone-parent without operators (keeps the child's world transform)"""
    bpy.context.view_layer.update()
    world = child.matrix_world.copy()
    basis = child.matrix_basis.copy()
    child.parent = arm
    child.parent_type = "BONE"
    child.parent_bone = bone_name
    child.matrix_parent_inverse = Matrix.Identity(4)
    bpy.context.view_layer.update()
    # blender's internal parent matrix P is unknown -> derive it from the result
    p = child.matrix_world @ basis.inverted()
    child.matrix_parent_inverse = p.inverted() @ world @ basis.inverted()
    bpy.context.view_layer.update()


# --------------------------------------------------------------------------
# animation
# --------------------------------------------------------------------------

def local_euler(bone, axis, angle):
    """bone-local euler for a rotation of `angle` about a global axis"""
    q = Quaternion(axis, angle)
    rq = bone.matrix_local.to_quaternion()
    return (rq.conjugated() @ q @ rq).to_euler("XYZ")


def build_animations(arm, eyes_empty, brows_empty):
    scene = bpy.context.scene
    actions = []
    for id_data in (arm, eyes_empty, brows_empty):     # drop tracks of earlier runs
        ad = id_data.animation_data
        if ad:
            ad.nla_tracks.clear()

    # ---- Idle: slow breathing sway (head bone) ----
    act = fresh_action("Idle")
    ad = arm.animation_data_create()
    slot = act.slots.new(id_type="OBJECT", name="Object")
    ad.action = act
    ad.action_slot = slot
    pb = arm.pose.bones["head"]
    pb.rotation_mode = "XYZ"
    for f, w in ((1, 0.0), (31, 1.0), (61, 0.0), (91, -1.0), (121, 0.0)):
        pb.rotation_euler = local_euler(arm.data.bones["head"], (0, 1, 0),
                                        math.radians(1.4) * w)
        pb.keyframe_insert("rotation_euler", frame=f)
        pb.rotation_euler = local_euler(arm.data.bones["head"], (1, 0, 0),
                                        math.radians(0.9) * w)
        pb.keyframe_insert("rotation_euler", frame=f)
    track = ad.nla_tracks.new()
    strip = track.strips.new(act.name, 1, act)
    strip.action_slot = slot
    actions.append(act)

    # ---- Nod: head pitch ----
    act = fresh_action("Nod")
    slot = act.slots.new(id_type="OBJECT", name="Object")
    ad.action = act
    ad.action_slot = slot
    for f, w in ((1, 0.0), (13, 1.0), (25, -0.35), (37, 0.0)):
        pb.rotation_euler = local_euler(arm.data.bones["head"], (1, 0, 0),
                                        math.radians(17.0) * w)
        pb.keyframe_insert("rotation_euler", frame=f)
    track = ad.nla_tracks.new()
    strip = track.strips.new(act.name, 1, act)
    strip.action_slot = slot
    actions.append(act)

    # ---- Turn: head yaw ----
    act = fresh_action("Turn")
    slot = act.slots.new(id_type="OBJECT", name="Object")
    ad.action = act
    ad.action_slot = slot
    for f, w in ((1, 0.0), (21, 1.0), (41, -1.0), (61, 0.0)):
        pb.rotation_euler = local_euler(arm.data.bones["head"], (0, 1, 0),
                                        math.radians(24.0) * w)
        pb.keyframe_insert("rotation_euler", frame=f)
    track = ad.nla_tracks.new()
    strip = track.strips.new(act.name, 1, act)
    strip.action_slot = slot
    actions.append(act)

    # ---- Talk: jaw opens around the ear axis ----
    act = fresh_action("Talk")
    slot = act.slots.new(id_type="OBJECT", name="Object")
    ad.action = act
    ad.action_slot = slot
    jb = arm.pose.bones["jaw"]
    jb.rotation_mode = "XYZ"
    for f, w in ((1, 0.0), (7, 0.35), (13, 1.0), (19, 0.4), (25, 0.8), (31, 0.0)):
        jb.rotation_euler = (0.0, math.radians(13.0) * w, 0.0)
        jb.keyframe_insert("rotation_euler", frame=f)
    track = ad.nla_tracks.new()
    strip = track.strips.new(act.name, 1, act)
    strip.action_slot = slot
    actions.append(act)

    ad.action = None

    # ---- Blink: squash the eye group ----
    act = fresh_action("Blink")
    ead = eyes_empty.animation_data_create()
    slot = act.slots.new(id_type="OBJECT", name="Object")
    ead.action = act
    ead.action_slot = slot
    for f, s in ((1, 1.0), (3, 1.0), (5, 0.06), (7, 1.0), (39, 1.0), (41, 0.06), (43, 1.0)):
        eyes_empty.scale = (1.0, s, 1.0)
        eyes_empty.keyframe_insert("scale", frame=f)
    track = ead.nla_tracks.new()
    strip = track.strips.new(act.name, 1, act)
    strip.action_slot = slot
    ead.action = None
    actions.append(act)

    # ---- BrowRaise ----
    act = fresh_action("BrowRaise")
    bad = brows_empty.animation_data_create()
    slot = act.slots.new(id_type="OBJECT", name="Object")
    bad.action = act
    bad.action_slot = slot
    base = brows_empty.location.copy()
    for f, d in ((1, 0.0), (15, 0.005), (30, 0.0)):
        brows_empty.location = base + Vector((0.0, 0.0, d))
        brows_empty.keyframe_insert("location", frame=f)
    track = bad.nla_tracks.new()
    strip = track.strips.new(act.name, 1, act)
    strip.action_slot = slot
    bad.action = None
    actions.append(act)

    scene.frame_start = 1
    scene.frame_end = 120
    return actions


# --------------------------------------------------------------------------
# preview
# --------------------------------------------------------------------------

def render_previews(targets, names=("front", "3q", "side"), res=640, prefix="preview"):
    """front / 3-quarter / side renders for quick visual review"""
    scene = bpy.context.scene
    keep = get_collection("3DFace_Preview")
    pts = []
    for o in targets:
        pts += [o.matrix_world @ Vector(c) for c in o.bound_box]
    if not pts:
        return []
    cx = sum(p.x for p in pts) / len(pts)
    cy = sum(p.y for p in pts) / len(pts)
    cz = sum(p.z for p in pts) / len(pts)
    span = max(max(p[i] for p in pts) - min(p[i] for p in pts) for i in range(3))

    def ensure(name, data):
        ob = bpy.data.objects.get(name)
        if ob is None:
            ob = bpy.data.objects.new(name, data)
            keep.objects.link(ob)
        return ob

    cam = ensure("PreviewCam", bpy.data.cameras.get("PreviewCam") or bpy.data.cameras.new("PreviewCam"))
    cam.data.lens = 85
    cam.data.clip_start = 0.005
    old_cam, old_engine = scene.camera, scene.render.engine
    old_res = (scene.render.resolution_x, scene.render.resolution_y)
    old_world = scene.world
    scene.camera = cam
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.film_transparent = False

    world = bpy.data.worlds.get("PreviewWorld") or bpy.data.worlds.new("PreviewWorld")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.04, 0.045, 0.055, 1.0)
    scene.world = world

    d = max(span, 0.25) * 1.6
    k = d / 0.55
    centre = Vector((cx, cy, cz))
    for name, energy, size, loc in (
        ("PreviewKey", 9.0, 0.4, (-0.82, -1.0, 0.82)),
        ("PreviewFill", 3.5, 0.7, (1.1, -0.64, 0.28)),
        ("PreviewRim", 6.0, 0.3, (0.28, 1.0, 0.72)),
    ):
        light = ensure(name, bpy.data.lights.get(name) or bpy.data.lights.new(name, "AREA"))
        light.data.energy = energy * k * k
        light.data.size = size * k
        light.location = centre + Vector(loc) * k
        light.rotation_euler = (centre - light.location).to_track_quat("-Z", "Y").to_euler()

    hidden = []
    keep_cols = {c for o in targets for c in o.users_collection}
    for ob in scene.objects:
        if ob in targets or ob.name in keep.objects:
            continue
        if keep_cols & set(ob.users_collection):     # siblings of the model
            continue
        if not ob.hide_render:
            ob.hide_render = True
            hidden.append(ob)

    def shoot(loc, target, path):
        cam.location = loc
        cam.rotation_euler = (Vector(target) - cam.location).to_track_quat("-Z", "Y").to_euler()
        scene.render.resolution_x = scene.render.resolution_y = res
        scene.render.filepath = path
        bpy.ops.render.render(write_still=True)

    out = []
    r = d * 1.15
    shots = [((cx, cy - r, cz), (cx, cy, cz), "front"),
             ((cx + r * 0.65, cy - r * 0.7, cz + r * 0.12), (cx, cy, cz), "3q"),
             ((cx + r, cy, cz), (cx, cy, cz), "side")]
    for (loc, target, name) in shots[:len(names)]:
        path = os.path.join(OUT_DIR, "%s_%s.png" % (prefix, name))
        shoot(loc, target, path)
        out.append(path)

    for ob in hidden:
        ob.hide_render = False
    scene.camera, scene.render.engine = old_cam, old_engine
    scene.render.resolution_x, scene.render.resolution_y = old_res
    scene.world = old_world
    return out


# --------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    col = get_collection()
    clear_collection()

    skin = material("FaceSkin", (0.45, 0.28, 0.20), 0.72)
    mouth_mat = material("MouthDark", (0.09, 0.03, 0.03), 0.9)
    sclera = material("EyeWhite", (0.88, 0.88, 0.87), 0.3)
    iris = material("EyeDark", (0.04, 0.04, 0.05), 0.2)
    brow = material("BrowDark", (0.06, 0.04, 0.03), 0.85)

    head = build_base_mesh(col)
    sculpt(head)
    smooth_mesh(head, factor=0.5, iterations=1)
    head.data.materials.append(skin)
    head.data.materials.append(mouth_mat)
    faces_cut = cut_mouth_cavity(head)
    tris = decimate(head, TARGET_TRIS)

    # eyes — seated in the sculpted sockets (front of the eyeball ~flush with the rim)
    eye_y = surface_y(head, EYE_X, EYE_Z) + 0.008
    eyes = build_empty("Eyes", (0.0, eye_y, EYE_Z), col)
    for side, sx in (("L", 1.0), ("R", -1.0)):
        sc = build_uv_sphere("Eye_" + side, (sx * EYE_X, 0.0, 0.0),
                             (0.0138, 0.0138, 0.0138), col, sclera, segs=(14, 8))
        ir = build_uv_sphere("Iris_" + side, (sx * EYE_X, -0.0110, 0.0),
                             (0.0065, 0.0045, 0.0065), col, iris, segs=(12, 8))
        sc.parent = eyes
        ir.parent = eyes

    # brows — sitting on the brow ridge
    brow_y = surface_y(head, 0.030, 0.164) + 0.004
    brows = build_empty("Brows", (0.0, brow_y, 0.164), col)
    for side, sx, ry in (("L", 1.0, -0.20), ("R", -1.0, 0.20)):
        br = build_box("Brow_" + side, (sx * 0.030, 0.0, 0.0),
                       (0.026, 0.011, 0.008), col, brow, rot=(0, ry, 0))
        br.parent = brows

    # rig
    arm = build_armature(col)
    skin_weights(head)
    mod = head.modifiers.new("Armature", "ARMATURE")
    mod.object = arm
    mod.use_vertex_groups = True
    mod.use_bone_envelopes = False
    head.parent = arm

    # hair cap cut from the scalp
    chin, crown, x_c, y_c = head_metrics(head.data, up=2)
    hair = build_hair(head, col, chin, crown, x_c, y_c)

    for child in (eyes, brows, hair):
        parent_to_bone(child, arm, "head")

    build_animations(arm, eyes, brows)

    # save + export
    bpy.context.view_layer.update()
    glb = os.path.join(OUT_DIR, "face.glb")
    bpy.ops.object.select_all(action="DESELECT")
    for ob in col.objects:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = head
    bpy.ops.export_scene.gltf(
        filepath=glb,
        export_format="GLB",
        use_selection=True,
        export_animations=True,
        export_animation_mode="ACTIONS",
        export_nla_strips=True,
        export_apply=True,
        export_yup=True,
        export_morph=True,
        export_skins=True,
        export_def_bones=False,
        export_extras=True,
    )
    purge_orphans()
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT_DIR, "face.blend"), copy=True)

    previews = render_previews([head, eyes, brows, hair], names=("front", "3q", "side"),
                               prefix="preview_face")
    focus_viewport([head, hair])

    return {
        "tris": tris,
        "hair_tris": sum(len(p.vertices) - 2 for p in hair.data.polygons),
        "verts": len(head.data.vertices),
        "mouth_faces_cut": faces_cut,
        "glb": glb,
        "glb_size": os.path.getsize(glb),
        "previews": previews,
        "objects": [o.name for o in col.objects],
    }


if __name__ == "__main__":
    result = main()
