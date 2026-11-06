"""
Headless Blender 5.2 generator for Phase 1 Wild Omission assets.
Follows the flat-shaded untextured low-poly style (<= 1.5k tris per prop, 8-sided silhouettes, Palette.json colors).
Generates:
  - SM_SulfurOreNode.blend / .fbx
  - SM_HQMOreNode.blend / .fbx
  - SM_WoodenSpear.blend / .fbx
  - SM_Crossbow.blend / .fbx
  - SM_HandmadeRevolver.blend / .fbx
  - SM_BoneArrow.blend / .fbx
"""
import json
import os
import random
import sys
import bpy
import bmesh

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.dirname(__file__))
import export_fbx


def hex_to_rgba(hex_str: str):
    h = hex_str.lstrip("#")
    return tuple(int(h[i:i+2], 16) / 255.0 for i in (0, 2, 4)) + (1.0,)


def make_mat(name: str, hex_color: str, metallic: float = 0.0, roughness: float = 0.85):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = hex_to_rgba(hex_color)
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
    return mat


def save_and_export(asset_name: str):
    blend_path = os.path.join(REPO_ROOT, "Art", "Blender", f"{asset_name}.blend")
    fbx_path = os.path.join(REPO_ROOT, "Art", "Export", f"{asset_name}.fbx")
    os.makedirs(os.path.dirname(fbx_path), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    export_fbx.export_clean_fbx(fbx_path)


def generate_ore_node(asset_name: str, stone_hex: str, vein_hex: str, metallic: float, seed: int):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_stone = make_mat("M_Stone_Base", stone_hex, 0.0, 0.9)
    mat_vein = make_mat("M_Ore_Vein", vein_hex, metallic, 0.5)

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=72.0, location=(0.0, 0.0, 42.0))
    obj = bpy.context.active_object
    obj.name = asset_name

    random.seed(seed)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    for v in bm.verts:
        if v.co.z < -24.0:
            v.co.z = -42.0
        else:
            s = random.uniform(0.84, 1.18)
            v.co.x *= s
            v.co.y *= s
            v.co.z *= random.uniform(0.88, 1.15)

    obj.data.materials.append(mat_stone)
    obj.data.materials.append(mat_vein)
    for f in bm.faces:
        f.smooth = False
        f.material_index = 1 if (f.calc_center_median().z > -5.0 and random.random() < 0.30) else 0

    bm.to_mesh(obj.data)
    bm.free()
    save_and_export(asset_name)


def generate_wooden_spear(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Wood_Medium", palette["Wood_Medium"])
    mat_tip = make_mat("M_Wood_Light", palette["Wood_Light"])

    # 8-sided shaft along Y axis (210 cm total length)
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=1.8, depth=185.0, location=(0.0, 0.0, 0.0), rotation=(1.5708, 0.0, 0.0))
    shaft = bpy.context.active_object
    shaft.name = "SM_WoodenSpear"
    shaft.data.materials.append(mat_wood)

    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=2.1, radius2=0.1, depth=28.0, location=(0.0, -106.0, 0.0), rotation=(-1.5708, 0.0, 0.0))
    tip = bpy.context.active_object
    tip.data.materials.append(mat_tip)

    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = shaft
    bpy.ops.object.join()
    save_and_export("SM_WoodenSpear")


def generate_bone_arrow(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Wood_Light", palette["Wood_Light"])
    mat_bone = make_mat("M_Bone_OffWhite", palette["Bone_OffWhite"])
    mat_cloth = make_mat("M_Cloth_Burlap", palette["Cloth_Burlap"])

    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=0.6, depth=65.0, location=(0.0, 0.0, 0.0), rotation=(1.5708, 0.0, 0.0))
    shaft = bpy.context.active_object
    shaft.name = "SM_BoneArrow"
    shaft.data.materials.append(mat_wood)

    bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=1.6, radius2=0.1, depth=12.0, location=(0.0, -38.0, 0.0), rotation=(-1.5708, 0.0, 0.0))
    head = bpy.context.active_object
    head.data.materials.append(mat_bone)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 28.0, 0.0), scale=(0.2, 6.0, 2.2))
    fletch = bpy.context.active_object
    fletch.data.materials.append(mat_cloth)

    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = shaft
    bpy.ops.object.join()
    save_and_export("SM_BoneArrow")


def generate_crossbow(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Wood_Dark", palette["Wood_Dark"])
    mat_metal = make_mat("M_Metal_Sheet", palette["Metal_Sheet"], 0.5, 0.5)
    mat_string = make_mat("M_Cloth_Burlap", palette["Cloth_Burlap"])

    # Stock
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.0), scale=(4.0, 62.0, 5.5))
    stock = bpy.context.active_object
    stock.name = "SM_Crossbow"
    stock.data.materials.append(mat_wood)

    # Prod (bow limb)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -28.0, 1.5), scale=(48.0, 3.0, 2.2))
    prod = bpy.context.active_object
    prod.data.materials.append(mat_metal)

    # Bowstring
    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=0.35, depth=46.0, location=(0.0, -10.0, 1.8), rotation=(0.0, 1.5708, 0.0))
    string_obj = bpy.context.active_object
    string_obj.data.materials.append(mat_string)

    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = stock
    bpy.ops.object.join()
    save_and_export("SM_Crossbow")


def generate_handmade_revolver(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Wood_Medium", palette["Wood_Medium"])
    mat_metal = make_mat("M_Metal_Rusted", palette["Metal_Rusted"], 0.45, 0.55)
    mat_barrel = make_mat("M_Metal_Sheet", palette["Metal_Sheet"], 0.6, 0.45)

    # Frame
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 2.0), scale=(3.2, 16.0, 4.2))
    frame = bpy.context.active_object
    frame.name = "SM_HandmadeRevolver"
    frame.data.materials.append(mat_metal)

    # 6-chamber revolving cylinder (8-sided low-poly silhouette)
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=2.6, depth=6.2, location=(0.0, -1.0, 3.0), rotation=(1.5708, 0.0, 0.0))
    cyl = bpy.context.active_object
    cyl.data.materials.append(mat_barrel)

    # Barrel
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=1.1, depth=15.0, location=(0.0, -11.5, 4.0), rotation=(1.5708, 0.0, 0.0))
    barrel = bpy.context.active_object
    barrel.data.materials.append(mat_barrel)

    # Wooden grip
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 6.5, -3.5), rotation=(-0.32, 0.0, 0.0), scale=(2.8, 4.2, 9.5))
    grip = bpy.context.active_object
    grip.data.materials.append(mat_wood)

    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = frame
    bpy.ops.object.join()
    save_and_export("SM_HandmadeRevolver")


def main():
    with open(os.path.join(REPO_ROOT, "Art", "Palette.json"), "r", encoding="utf-8-sig") as f:
        palette = json.load(f)["colors"]

    generate_ore_node("SM_SulfurOreNode", palette["Stone_Medium"], palette["Sulfur_Yellow"], 0.1, 101)
    generate_ore_node("SM_HQMOreNode", palette["Stone_Dark"], palette["Metal_HQM"], 0.75, 202)
    generate_wooden_spear(palette)
    generate_bone_arrow(palette)
    generate_crossbow(palette)
    generate_handmade_revolver(palette)


if __name__ == "__main__":
    main()
