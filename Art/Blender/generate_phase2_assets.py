"""
Headless Blender 5.2 generator for Phase 2 Wild Omission industrial, base upkeep, and electrical assets.
Follows the flat-shaded untextured low-poly style (<= 1.5k tris per prop, 8-sided silhouettes, Palette.json colors).
Generates:
  - SM_WorkbenchT1, SM_WorkbenchT2, SM_WorkbenchT3, SM_ResearchTable
  - SM_KeyLock, SM_LargeStorageBox, SM_LargeFurnace, SM_OilRefinery
  - SM_SolarPanel, SM_AutoTurret, SM_ShotgunTrap
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


def generate_workbench(asset_name: str, top_hex: str, frame_hex: str, accent_hex: str, top_metal: float = 0.0):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    top_mat = make_mat(f"M_{asset_name}_Top", top_hex, top_metal, 0.5)
    frame_mat = make_mat(f"M_{asset_name}_Frame", frame_hex, 0.3, 0.6)
    accent_mat = make_mat(f"M_{asset_name}_Vice", accent_hex, 0.5, 0.5)

    # Tabletop
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 82.0), scale=(150.0, 75.0, 8.0))
    top = bpy.context.active_object
    top.data.materials.append(top_mat)

    # 4 legs
    for lx in (-66.0, 66.0):
        for ly in (-30.0, 30.0):
            bpy.ops.mesh.primitive_cube_add(size=1.0, location=(lx, ly, 40.0), scale=(8.0, 8.0, 80.0))
            leg = bpy.context.active_object
            leg.data.materials.append(frame_mat)

    # Backboard & vice/tool rack
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 34.0, 118.0), scale=(146.0, 5.0, 64.0))
    back = bpy.context.active_object
    back.data.materials.append(frame_mat)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=11.0, depth=24.0, location=(-45.0, -18.0, 94.0))
    vice = bpy.context.active_object
    vice.data.materials.append(accent_mat)

    join_and_export(asset_name, top)


def generate_research_table(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Wood_Dark", palette["Wood_Dark"])
    mat_blue = make_mat("M_Tech_Blue", palette["Tech_Blue"])
    mat_metal = make_mat("M_Metal_Sheet", palette["Metal_Sheet"], 0.5, 0.5)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 78.0), rotation=(0.15, 0.0, 0.0), scale=(135.0, 75.0, 6.0))
    desk = bpy.context.active_object
    desk.data.materials.append(mat_wood)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 82.5), rotation=(0.15, 0.0, 0.0), scale=(95.0, 52.0, 1.5))
    blueprint_sheet = bpy.context.active_object
    blueprint_sheet.data.materials.append(mat_blue)

    for lx in (-58.0, 58.0):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(lx, 0.0, 38.0), scale=(8.0, 62.0, 76.0))
        leg = bpy.context.active_object
        leg.data.materials.append(mat_metal)

    join_and_export("SM_ResearchTable", desk)


def generate_key_lock(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_iron = make_mat("M_Metal_Rusted", palette["Metal_Rusted"], 0.4, 0.6)
    mat_wood = make_mat("M_Wood_Dark", palette["Wood_Dark"])

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 0.0), scale=(12.0, 6.0, 16.0))
    body = bpy.context.active_object
    body.data.materials.append(mat_wood)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=3.5, depth=8.0, location=(0.0, 0.0, 10.0))
    shackle = bpy.context.active_object
    shackle.data.materials.append(mat_iron)

    join_and_export("SM_KeyLock", body)


def generate_large_storage_box(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Wood_Medium", palette["Wood_Medium"])
    mat_band = make_mat("M_Metal_Sheet", palette["Metal_Sheet"], 0.5, 0.5)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 38.0), scale=(145.0, 78.0, 76.0))
    chest = bpy.context.active_object
    chest.data.materials.append(mat_wood)

    for bx in (-55.0, 0.0, 55.0):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(bx, 0.0, 39.0), scale=(6.0, 80.0, 78.0))
        band = bpy.context.active_object
        band.data.materials.append(mat_band)

    join_and_export("SM_LargeStorageBox", chest)


def generate_large_furnace(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_stone = make_mat("M_Stone_Medium", palette["Stone_Medium"])
    mat_dark = make_mat("M_Stone_Dark", palette["Stone_Dark"])
    mat_glow = make_mat("M_Fire_Glow", palette["Hazard_Red"])

    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=145.0, radius2=62.0, depth=290.0, location=(0.0, 0.0, 145.0))
    chimney = bpy.context.active_object
    chimney.data.materials.append(mat_stone)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=152.0, depth=48.0, location=(0.0, 0.0, 24.0))
    base = bpy.context.active_object
    base.data.materials.append(mat_dark)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -125.0, 42.0), scale=(58.0, 36.0, 52.0))
    mouth = bpy.context.active_object
    mouth.data.materials.append(mat_glow)

    join_and_export("SM_LargeFurnace", chimney)


def generate_oil_refinery(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_rusted = make_mat("M_Metal_Rusted", palette["Metal_Rusted"], 0.45, 0.55)
    mat_sheet = make_mat("M_Metal_Sheet", palette["Metal_Sheet"], 0.6, 0.45)
    mat_pipe = make_mat("M_Copper_Wire", palette["Copper_Wire"], 0.65, 0.4)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=52.0, depth=135.0, location=(-28.0, 0.0, 68.0))
    drum = bpy.context.active_object
    drum.data.materials.append(mat_rusted)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=24.0, depth=175.0, location=(42.0, 0.0, 88.0))
    tower = bpy.context.active_object
    tower.data.materials.append(mat_sheet)

    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=6.0, depth=85.0, location=(8.0, 0.0, 125.0), rotation=(0.0, 1.25, 0.0))
    pipe = bpy.context.active_object
    pipe.data.materials.append(mat_pipe)

    join_and_export("SM_OilRefinery", drum)


def generate_solar_panel(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_frame = make_mat("M_Metal_Sheet", palette["Metal_Sheet"], 0.5, 0.5)
    mat_cell = make_mat("M_Tech_Blue", palette["Tech_Blue"], 0.3, 0.25)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 75.0), rotation=(0.45, 0.0, 0.0), scale=(160.0, 95.0, 5.0))
    frame = bpy.context.active_object
    frame.data.materials.append(mat_frame)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -1.0, 77.5), rotation=(0.45, 0.0, 0.0), scale=(150.0, 86.0, 2.0))
    cells = bpy.context.active_object
    cells.data.materials.append(mat_cell)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=7.0, depth=75.0, location=(0.0, 0.0, 37.5))
    post = bpy.context.active_object
    post.data.materials.append(mat_frame)

    join_and_export("SM_SolarPanel", frame)


def generate_auto_turret(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_armored = make_mat("M_Metal_Armored", palette["Metal_Armored"], 0.7, 0.4)
    mat_hazard = make_mat("M_Hazmat_Yellow", palette["Hazmat_Yellow"])
    mat_laser = make_mat("M_Hazard_Red", palette["Hazard_Red"])

    # Tripod base
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=55.0, radius2=14.0, depth=65.0, location=(0.0, 0.0, 32.5))
    tripod = bpy.context.active_object
    tripod.data.materials.append(mat_armored)

    # Swivel head
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 78.0), scale=(34.0, 42.0, 28.0))
    head = bpy.context.active_object
    head.data.materials.append(mat_hazard)

    # Barrel & sensor eye
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=4.5, depth=52.0, location=(0.0, -38.0, 78.0), rotation=(1.5708, 0.0, 0.0))
    barrel = bpy.context.active_object
    barrel.data.materials.append(mat_armored)

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=5.5, location=(0.0, -22.0, 88.0))
    sensor = bpy.context.active_object
    sensor.data.materials.append(mat_laser)

    join_and_export("SM_AutoTurret", tripod)


def generate_shotgun_trap(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Wood_Dark", palette["Wood_Dark"])
    mat_metal = make_mat("M_Metal_Rusted", palette["Metal_Rusted"], 0.5, 0.5)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 18.0), scale=(48.0, 58.0, 36.0))
    mount = bpy.context.active_object
    mount.data.materials.append(mat_wood)

    for bx in (-6.0, 6.0):
        bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=3.2, depth=46.0, location=(bx, -32.0, 28.0), rotation=(1.5708, 0.0, 0.0))
        barrel = bpy.context.active_object
        barrel.data.materials.append(mat_metal)

    join_and_export("SM_ShotgunTrap", mount)


def main():
    with open(os.path.join(REPO_ROOT, "Art", "Palette.json"), "r", encoding="utf-8-sig") as f:
        palette = json.load(f)["colors"]

    generate_workbench(
        "SM_WorkbenchT1",
        palette["Wood_Medium"],
        palette["Wood_Dark"],
        palette["Metal_Rusted"],
        0.0,
    )
    generate_workbench(
        "SM_WorkbenchT2",
        palette["Metal_Sheet"],
        palette["Tech_Blue"],
        palette["Hazmat_Yellow"],
        0.55,
    )
    generate_workbench(
        "SM_WorkbenchT3",
        palette["Metal_HQM"],
        palette["Metal_Armored"],
        palette["Hazard_Red"],
        0.75,
    )
    generate_research_table(palette)
    generate_key_lock(palette)
    generate_large_storage_box(palette)
    generate_large_furnace(palette)
    generate_oil_refinery(palette)
    generate_solar_panel(palette)
    generate_auto_turret(palette)
    generate_shotgun_trap(palette)


if __name__ == "__main__":
    main()
