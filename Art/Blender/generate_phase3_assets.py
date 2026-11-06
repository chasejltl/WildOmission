"""
Headless Blender 5.2 generator for Phase 3 Wild Omission assets:
  - SM_BoltActionRifle, SM_CustomSMG, SM_TimedExplosiveCharge, SM_MedicalSyringe
  - SM_HazmatHelm, SM_HempPlant, SM_PumpkinPlant, SM_LockedCrate, SM_MonumentRadSign
"""
import json
import os
import sys
import bpy

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


def join_and_export(asset_name: str, root_obj):
    bpy.ops.object.select_all(action='SELECT')
    bpy.context.view_layer.objects.active = root_obj
    bpy.ops.object.join()
    root_obj.name = asset_name
    for poly in root_obj.data.polygons:
        poly.use_smooth = False
    blend_path = os.path.join(REPO_ROOT, "Art", "Blender", f"{asset_name}.blend")
    fbx_path = os.path.join(REPO_ROOT, "Art", "Export", f"{asset_name}.fbx")
    os.makedirs(os.path.dirname(fbx_path), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    export_fbx.export_clean_fbx(fbx_path)


def generate_bolt_action_rifle(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Bolt_Wood", palette["Wood_Dark"])
    mat_steel = make_mat("M_Bolt_Steel", palette["Metal_HQM"], 0.7, 0.35)

    # Stock & body
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 12.0, 0.0), scale=(3.6, 58.0, 6.2))
    stock = bpy.context.active_object
    stock.data.materials.append(mat_wood)

    # Long sniper barrel
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=1.15, depth=78.0, location=(0.0, -32.0, 2.5), rotation=(1.5708, 0.0, 0.0))
    barrel = bpy.context.active_object
    barrel.data.materials.append(mat_steel)

    # Scope tube
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=1.8, depth=24.0, location=(0.0, -4.0, 6.2), rotation=(1.5708, 0.0, 0.0))
    scope = bpy.context.active_object
    scope.data.materials.append(mat_steel)

    join_and_export("SM_BoltActionRifle", stock)


def generate_custom_smg(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_body = make_mat("M_SMG_Body", palette["Metal_Armored"], 0.65, 0.4)
    mat_mag = make_mat("M_SMG_Mag", palette["Metal_Sheet"], 0.5, 0.5)
    mat_grip = make_mat("M_SMG_Grip", palette["Wood_Medium"])

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 2.0), scale=(3.8, 34.0, 5.5))
    receiver = bpy.context.active_object
    receiver.data.materials.append(mat_body)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -8.0, -7.5), scale=(2.4, 3.8, 15.0))
    mag = bpy.context.active_object
    mag.data.materials.append(mat_mag)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 8.0, -4.5), rotation=(-0.25, 0.0, 0.0), scale=(2.8, 4.0, 9.5))
    grip = bpy.context.active_object
    grip.data.materials.append(mat_grip)

    join_and_export("SM_CustomSMG", receiver)


def generate_c4_charge(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_pack = make_mat("M_C4_Pack", palette["Cloth_Burlap"])
    mat_tape = make_mat("M_C4_Tape", palette["Charcoal_Black"])
    mat_led = make_mat("M_C4_LED", palette["Hazard_Red"])

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 4.0), scale=(18.0, 26.0, 8.0))
    pack = bpy.context.active_object
    pack.data.materials.append(mat_pack)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 4.2), scale=(19.0, 6.0, 8.5))
    tape = bpy.context.active_object
    tape.data.materials.append(mat_tape)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -4.0, 8.8), scale=(6.0, 8.0, 2.5))
    timer = bpy.context.active_object
    timer.data.materials.append(mat_led)

    join_and_export("SM_TimedExplosiveCharge", pack)


def generate_medical_syringe(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_body = make_mat("M_Syringe_White", palette["Medical_White"])
    mat_cap = make_mat("M_Syringe_Red", palette["Hazard_Red"])
    mat_plunger = make_mat("M_Syringe_Blue", palette["Tech_Blue"])

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=1.8, depth=14.0, location=(0.0, 0.0, 0.0))
    barrel = bpy.context.active_object
    barrel.data.materials.append(mat_body)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=2.1, depth=3.5, location=(0.0, 0.0, -7.5))
    cap = bpy.context.active_object
    cap.data.materials.append(mat_cap)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=1.2, depth=5.0, location=(0.0, 0.0, 8.5))
    plunger = bpy.context.active_object
    plunger.data.materials.append(mat_plunger)

    join_and_export("SM_MedicalSyringe", barrel)


def generate_hazmat_helm(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_yellow = make_mat("M_Hazmat_Yellow", palette["Hazmat_Yellow"])
    mat_visor = make_mat("M_Hazmat_Visor", palette["Glass_Tint"], 0.2, 0.2)

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=15.0, location=(0.0, 0.0, 15.0))
    hood = bpy.context.active_object
    hood.data.materials.append(mat_yellow)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -11.5, 16.0), scale=(16.0, 6.0, 10.0))
    visor = bpy.context.active_object
    visor.data.materials.append(mat_visor)

    join_and_export("SM_HazmatHelm", hood)


def generate_hemp_plant(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_stem = make_mat("M_Hemp_Stem", palette["Foliage_Pine"])
    mat_leaf = make_mat("M_Hemp_Leaf", palette["Foliage_Light"])

    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=2.2, depth=95.0, location=(0.0, 0.0, 47.5))
    stem = bpy.context.active_object
    stem.data.materials.append(mat_stem)

    for z_pos, rad in ((35.0, 22.0), (62.0, 28.0), (88.0, 18.0)):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=rad, location=(0.0, 0.0, z_pos))
        tuft = bpy.context.active_object
        tuft.data.materials.append(mat_leaf)

    join_and_export("SM_HempPlant", stem)


def generate_pumpkin_plant(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_pumpkin = make_mat("M_Pumpkin_Orange", palette["Hazmat_Yellow"])
    mat_stem = make_mat("M_Pumpkin_Stem", palette["Foliage_Pine"])

    bpy.ops.mesh.primitive_uv_sphere_add(segments=8, ring_count=6, radius=22.0, location=(0.0, 0.0, 16.0), scale=(1.0, 1.0, 0.72))
    gourd = bpy.context.active_object
    gourd.data.materials.append(mat_pumpkin)

    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=2.5, depth=8.0, location=(0.0, 0.0, 32.0))
    stem = bpy.context.active_object
    stem.data.materials.append(mat_stem)

    join_and_export("SM_PumpkinPlant", gourd)


def generate_locked_crate(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_olive = make_mat("M_Crate_Olive", palette["Foliage_Pine"])
    mat_frame = make_mat("M_Crate_Steel", palette["Metal_Armored"], 0.6, 0.45)
    mat_screen = make_mat("M_Crate_Screen", palette["Tech_Blue"])

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 55.0), scale=(135.0, 95.0, 110.0))
    box = bpy.context.active_object
    box.data.materials.append(mat_olive)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -48.0, 68.0), scale=(32.0, 4.0, 24.0))
    laptop = bpy.context.active_object
    laptop.data.materials.append(mat_screen)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 55.0), scale=(139.0, 99.0, 12.0))
    band = bpy.context.active_object
    band.data.materials.append(mat_frame)

    join_and_export("SM_LockedCrate", box)


def generate_monument_rad_sign(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_post = make_mat("M_Sign_Post", palette["Metal_Rusted"], 0.5, 0.5)
    mat_board = make_mat("M_Sign_Yellow", palette["Hazmat_Yellow"])
    mat_warn = make_mat("M_Sign_Red", palette["Hazard_Red"])

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=3.5, depth=210.0, location=(0.0, 0.0, 105.0))
    post = bpy.context.active_object
    post.data.materials.append(mat_post)

    bpy.ops.mesh.primitive_cylinder_add(vertices=3, radius=42.0, depth=4.0, location=(0.0, -4.0, 185.0), rotation=(1.5708, 0.0, 0.0))
    board = bpy.context.active_object
    board.data.materials.append(mat_board)

    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=10.0, depth=5.0, location=(0.0, -5.0, 182.0), rotation=(1.5708, 0.0, 0.0))
    symbol = bpy.context.active_object
    symbol.data.materials.append(mat_warn)

    join_and_export("SM_MonumentRadSign", post)


def main():
    with open(os.path.join(REPO_ROOT, "Art", "Palette.json"), "r", encoding="utf-8-sig") as f:
        palette = json.load(f)["colors"]

    generate_bolt_action_rifle(palette)
    generate_custom_smg(palette)
    generate_c4_charge(palette)
    generate_medical_syringe(palette)
    generate_hazmat_helm(palette)
    generate_hemp_plant(palette)
    generate_pumpkin_plant(palette)
    generate_locked_crate(palette)
    generate_monument_rad_sign(palette)


if __name__ == "__main__":
    main()
