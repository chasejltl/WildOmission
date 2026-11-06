"""
Generates 3 ready-to-spawn Wild Omission bases using the Python Base Builder SDK:
1. `starter_2x2`             - Stone 2x2 starter base with Tool Cupboard, Small Furnace, Storage Barrel, Large Box, Sleeping Bag & Ceiling Light.
2. `honeycomb_clan_fortress` - 3x3 2-story Sheet Metal & Armored fortress with interior stairs, shooting floor windows, Solar Panels, Auto Turrets, Storage Barrels & Workbenches.
3. `industrial_compound`     - Walled industrial compound with High External Stone Walls, Large Furnaces, Oil Refinery, Weather Radar, Rain Catcher, Searchlights & Storage Drums.
"""

from wild_omission_base import BaseBlueprint


def build_starter_2x2() -> None:
    base = BaseBlueprint("starter_2x2", grid_size=300.0, wall_height=300.0)
    base.add_box_fortress(width=2, length=2, floors=1, tier="stone", include_roof=True)

    # Interior deployables (Tool Cupboard, furnaces, storage barrels, sleeping bag, lighting)
    base.add_deployable("cupboard.tool", x=80.0, y=-220.0, z=30.0, yaw=0.0)
    base.add_deployable("furnace", x=80.0, y=80.0, z=30.0, yaw=180.0)
    base.add_deployable("StorageBarrel", x=-200.0, y=-220.0, z=30.0, yaw=90.0)
    base.add_deployable("storage.box.large", x=-200.0, y=80.0, z=30.0, yaw=0.0)
    base.add_deployable("WorkbenchT1", x=80.0, y=-80.0, z=30.0, yaw=180.0)
    base.add_deployable("sleepingbag", x=-80.0, y=-80.0, z=30.0, yaw=0.0)
    base.add_deployable("ceiling.light", x=-150.0, y=-150.0, z=285.0, yaw=0.0)
    base.save()


def build_honeycomb_clan_fortress() -> None:
    base = BaseBlueprint("honeycomb_clan_fortress", grid_size=300.0, wall_height=300.0)
    base.add_box_fortress(width=3, length=3, floors=2, tier="metal", include_roof=True)

    # Ground floor core vault deployables
    base.add_deployable("cupboard.tool", x=0.0, y=-250.0, z=30.0)
    base.add_deployable("WorkbenchT3", x=0.0, y=220.0, z=30.0, yaw=180.0)
    base.add_deployable("ResearchTable", x=220.0, y=0.0, z=30.0, yaw=270.0)
    base.add_deployable("ArmoredLocker", x=-220.0, y=220.0, z=30.0, yaw=90.0)

    # Row of Storage Barrels & Large Boxes
    for i, y_pos in enumerate([-200.0, -80.0, 40.0]):
        base.add_deployable("StorageBarrel", x=-240.0, y=y_pos, z=30.0)
        base.add_deployable("FuelBarrel", x=240.0, y=y_pos, z=30.0)

    # Smelting & Lighting on 1st and 2nd floor
    base.add_deployable("furnace", x=180.0, y=200.0, z=30.0)
    base.add_deployable("furnace", x=80.0, y=200.0, z=30.0)
    base.add_deployable("ceiling.light", x=0.0, y=0.0, z=285.0)
    base.add_deployable("ceiling.light", x=0.0, y=0.0, z=585.0)

    # Roof Solar Panels, Battery Bank, Weather Radar & Auto Turrets
    base.add_deployable("SolarPanel", x=-250.0, y=-250.0, z=620.0, yaw=45.0)
    base.add_deployable("SolarPanel", x=-250.0, y=250.0, z=620.0, yaw=135.0)
    base.add_deployable("LargeBattery", x=0.0, y=0.0, z=620.0)
    base.add_deployable("WeatherRadar", x=200.0, y=0.0, z=620.0)
    base.add_deployable("AutoTurret", x=-300.0, y=0.0, z=620.0, yaw=180.0)
    base.add_deployable("AutoTurret", x=300.0, y=0.0, z=620.0, yaw=0.0)
    base.save()


def build_industrial_compound() -> None:
    base = BaseBlueprint("industrial_compound", grid_size=300.0, wall_height=300.0)
    # Central 2x2 Armored core
    base.add_box_fortress(width=2, length=2, floors=1, tier="armored", include_roof=True)

    # Outer perimeter of High External Stone Compound Walls & Searchlights
    for offset in [-600.0, 0.0, 600.0]:
        base.add_deployable("highwall.stone", x=750.0, y=offset, z=0.0, yaw=0.0)
        base.add_deployable("highwall.stone", x=-950.0, y=offset, z=0.0, yaw=180.0)
        base.add_deployable("highwall.stone", x=offset, y=750.0, z=0.0, yaw=90.0)
        base.add_deployable("highwall.stone", x=offset, y=-950.0, z=0.0, yaw=270.0)

    # Outdoor industrial smelters, refinery, rain catchers & street lamps
    base.add_deployable("LargeFurnace", x=450.0, y=450.0, z=10.0)
    base.add_deployable("OilRefinery", x=-550.0, y=450.0, z=10.0)
    base.add_deployable("RainCollector", x=450.0, y=-550.0, z=10.0)
    base.add_deployable("WindTurbine", x=-550.0, y=-550.0, z=10.0)
    base.add_deployable("PoweredSearchLight", x=600.0, y=600.0, z=10.0, yaw=45.0)
    base.add_deployable("PoweredSearchLight", x=-700.0, y=-700.0, z=10.0, yaw=225.0)
    base.save()


if __name__ == "__main__":
    build_starter_2x2()
    build_honeycomb_clan_fortress()
    build_industrial_compound()
