"""
Headless Blender 5.2 generator for Phase 4 Wild Omission assets:
  - SM_RainCollector, SM_WeatherRadar, SM_ScientistHazmatRifle, SM_PatrolHelicopter, SM_FragGrenade
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


def generate_rain_collector(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_wood = make_mat("M_Rain_Wood", palette["Wood_Medium"])
    mat_tarp = make_mat("M_Rain_Tarp", palette["Tech_Blue"])
    mat_barrel = make_mat("M_Rain_Barrel", palette["Metal_Rusted"], 0.45, 0.6)

    # Central catch barrel
    bpy.ops.mesh.primitive_cylinder_add(vertices=10, radius=32.0, depth=75.0, location=(0.0, 0.0, 37.5))
    barrel = bpy.context.active_object
    barrel.data.materials.append(mat_barrel)

    # Inverted cone tarp funnel
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=30.0, radius2=72.0, depth=42.0, location=(0.0, 0.0, 95.0))
    tarp = bpy.context.active_object
    tarp.data.materials.append(mat_tarp)

    # 4 corner wooden support posts
    for sx, sy in ((-52.0, -52.0), (52.0, -52.0), (-52.0, 52.0), (52.0, 52.0)):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(sx, sy, 60.0), scale=(7.0, 7.0, 120.0))
        post = bpy.context.active_object
        post.data.materials.append(mat_wood)

    join_and_export("SM_RainCollector", barrel)


def generate_weather_radar(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_cabinet = make_mat("M_Radar_Steel", palette["Metal_Armored"], 0.6, 0.4)
    mat_dish = make_mat("M_Radar_Dish", palette["Medical_White"], 0.2, 0.5)
    mat_screen = make_mat("M_Radar_Screen", palette["Foliage_Light"])

    # Console base cabinet
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 45.0), scale=(70.0, 55.0, 90.0))
    cabinet = bpy.context.active_object
    cabinet.data.materials.append(mat_cabinet)

    # Radar sweep screen
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -28.0, 62.0), scale=(44.0, 4.0, 32.0))
    screen = bpy.context.active_object
    screen.data.materials.append(mat_screen)

    # Mast & parabolic dish
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=5.0, depth=75.0, location=(0.0, 0.0, 125.0))
    mast = bpy.context.active_object
    mast.data.materials.append(mat_cabinet)

    bpy.ops.mesh.primitive_cone_add(vertices=10, radius1=42.0, radius2=6.0, depth=22.0, location=(0.0, -12.0, 162.0), rotation=(1.2, 0.0, 0.0))
    dish = bpy.context.active_object
    dish.data.materials.append(mat_dish)

    join_and_export("SM_WeatherRadar", cabinet)


def generate_scientist_figure(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_suit = make_mat("M_Sci_Suit", palette["Tech_Blue"])
    mat_visor = make_mat("M_Sci_Visor", palette["Hazmat_Yellow"])
    mat_gear = make_mat("M_Sci_Gear", palette["Charcoal_Black"])

    # Torso & hazmat legs proxy
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 95.0), scale=(46.0, 28.0, 95.0))
    torso = bpy.context.active_object
    torso.data.materials.append(mat_suit)

    # Hooded head
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=18.0, location=(0.0, 0.0, 162.0))
    head = bpy.context.active_object
    head.data.materials.append(mat_suit)

    # Gold reflective visor
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -14.0, 162.0), scale=(20.0, 6.0, 11.0))
    visor = bpy.context.active_object
    visor.data.materials.append(mat_visor)

    # Tactical chest rig
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -15.0, 112.0), scale=(38.0, 6.0, 34.0))
    rig = bpy.context.active_object
    rig.data.materials.append(mat_gear)

    join_and_export("SM_ScientistHazmatRifle", torso)


def generate_patrol_helicopter(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_hull = make_mat("M_Heli_Hull", palette["Metal_Armored"], 0.65, 0.35)
    mat_rotor = make_mat("M_Heli_Rotor", palette["Charcoal_Black"], 0.5, 0.4)
    mat_glass = make_mat("M_Heli_Glass", palette["Glass_Tint"], 0.2, 0.2)
    mat_warn = make_mat("M_Heli_Warn", palette["Hazard_Red"])

    # Main fuselage
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 120.0), scale=(165.0, 420.0, 135.0))
    hull = bpy.context.active_object
    hull.data.materials.append(mat_hull)

    # Cockpit nose
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, -235.0, 110.0), scale=(135.0, 95.0, 95.0))
    cockpit = bpy.context.active_object
    cockpit.data.materials.append(mat_glass)

    # Tail boom
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 340.0, 145.0), scale=(42.0, 320.0, 48.0))
    tail = bpy.context.active_object
    tail.data.materials.append(mat_hull)

    # Main rotor blades
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, 205.0), scale=(680.0, 24.0, 6.0))
    main_rotor = bpy.context.active_object
    main_rotor.data.materials.append(mat_rotor)

    # Tail rotor weakspot
    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=48.0, depth=8.0, location=(26.0, 485.0, 165.0), rotation=(0.0, 1.5708, 0.0))
    tail_rotor = bpy.context.active_object
    tail_rotor.data.materials.append(mat_warn)

    join_and_export("SM_PatrolHelicopter", hull)


def generate_frag_grenade(palette: dict):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    mat_olive = make_mat("M_Grenade_Olive", palette["Foliage_Pine"], 0.35, 0.6)
    mat_steel = make_mat("M_Grenade_Steel", palette["Metal_Sheet"], 0.65, 0.35)

    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=5.5, location=(0.0, 0.0, 5.5))
    body = bpy.context.active_object
    body.data.materials.append(mat_olive)

    bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=2.0, depth=3.5, location=(0.0, 0.0, 11.5))
    fuse = bpy.context.active_object
    fuse.data.materials.append(mat_steel)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(2.8, 0.0, 7.5), scale=(1.2, 2.2, 8.5))
    spoon = bpy.context.active_object
    spoon.data.materials.append(mat_steel)

    join_and_export("SM_FragGrenade", body)


def main():
    with open(os.path.join(REPO_ROOT, "Art", "Palette.json"), "r", encoding="utf-8-sig") as f:
        palette = json.load(f)["colors"]

    generate_rain_collector(palette)
    generate_weather_radar(palette)
    generate_scientist_figure(palette)
    generate_patrol_helicopter(palette)
    generate_frag_grenade(palette)


if __name__ == "__main__":
    main()
