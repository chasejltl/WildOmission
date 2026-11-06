"""
Wild Omission - Python Base Builder SDK (Blender-style Procedural Base Construction)
====================================================================================
Allows you to script, loop, and procedurally generate multi-story Rust/Wild Omission
bases in Python (just like scripting meshes in Blender's `bpy`) and spawn them
directly in-game using `/buildbase <blueprint_name>` in chat or `BuildBase <name>` in console.

Example Usage:
--------------
    from wild_omission_base import BaseBlueprint

    base = BaseBlueprint("my_custom_fortress", grid_size=300.0, wall_height=300.0)
    base.add_box_fortress(width=3, length=3, floors=2, tier="metal")
    base.add_deployable("cupboard.tool", x=0, y=0, z=50)
    base.add_deployable("furnace", x=120, y=120, z=50)
    base.add_deployable("StorageBarrel", x=-120, y=120, z=50)
    base.add_deployable("ceiling.light", x=0, y=0, z=290)
    base.save()
"""

import json
from pathlib import Path
from typing import List, Dict, Any, Tuple


TIER_MAP = {
    "twig": {
        "foundation": "TwigFoundation",
        "wall": "TwigWall",
        "doorway": "TwigDoorway",
        "window": "wall.window.wood",
        "floor": "TwigFloor",
        "stairs": "TwigStairs",
        "door": "door.wood",
    },
    "wood": {
        "foundation": "foundation.wood",
        "wall": "wall.wood",
        "doorway": "wall.doorway.wood",
        "window": "wall.window.wood",
        "floor": "floor.wood",
        "stairs": "ustairs.wood",
        "door": "door.wood",
    },
    "stone": {
        "foundation": "foundation.stone",
        "wall": "wall.stone",
        "doorway": "wall.doorway.stone",
        "window": "wall.window.stone",
        "floor": "floor.stone",
        "stairs": "ustairs.stone",
        "door": "door.metal",
    },
    "metal": {
        "foundation": "foundation.metal",
        "wall": "wall.metal",
        "doorway": "wall.doorway.metal",
        "window": "wall.window.metal",
        "floor": "floor.metal",
        "stairs": "ustairs.metal",
        "door": "door.metal",
    },
    "armored": {
        "foundation": "ArmoredFoundation",
        "wall": "ArmoredWall",
        "doorway": "ArmoredDoorway",
        "window": "ArmoredWindowWall",
        "floor": "ArmoredFloor",
        "stairs": "ArmoredStairs",
        "door": "ArmoredDoor",
    },
}


class BaseBlueprint:
    """Blender-style procedural base builder for Wild Omission."""

    def __init__(self, name: str, grid_size: float = 300.0, wall_height: float = 300.0):
        self.name = name
        self.grid_size = float(grid_size)
        self.wall_height = float(wall_height)
        self.pieces: List[Dict[str, Any]] = []

    def add_piece(
        self,
        item_id: str,
        x: float = 0.0,
        y: float = 0.0,
        z: float = 0.0,
        pitch: float = 0.0,
        yaw: float = 0.0,
        roll: float = 0.0,
    ) -> "BaseBlueprint":
        """Adds any building block or deployable at exact local (x, y, z) cm offset and rotation."""
        self.pieces.append(
            {
                "item": item_id,
                "offset": [round(float(x), 2), round(float(y), 2), round(float(z), 2)],
                "rotation": [round(float(pitch), 2), round(float(yaw), 2), round(float(roll), 2)],
            }
        )
        return self

    def add_deployable(
        self, item_id: str, x: float, y: float, z: float = 50.0, yaw: float = 0.0
    ) -> "BaseBlueprint":
        """Places a deployable (furnace, storage barrel, workbench, turret, solar panel, light, etc.)."""
        return self.add_piece(item_id, x, y, z, 0.0, yaw, 0.0)

    def add_grid_foundation(self, gx: int, gy: int, tier: str = "stone", z: float = 0.0) -> "BaseBlueprint":
        """Places a foundation at grid cell (gx, gy)."""
        t = TIER_MAP.get(tier.lower(), TIER_MAP["stone"])
        return self.add_piece(t["foundation"], gx * self.grid_size, gy * self.grid_size, z)

    def add_grid_floor(self, gx: int, gy: int, floor_level: int = 1, tier: str = "stone") -> "BaseBlueprint":
        """Places a ceiling/floor tile at grid cell (gx, gy) and story `floor_level`."""
        t = TIER_MAP.get(tier.lower(), TIER_MAP["stone"])
        return self.add_piece(
            t["floor"], gx * self.grid_size, gy * self.grid_size, floor_level * self.wall_height
        )

    def add_grid_stairs(
        self, gx: int, gy: int, floor_level: int = 0, tier: str = "stone", yaw: float = 0.0
    ) -> "BaseBlueprint":
        """Places interior U-stairs at grid cell (gx, gy) leading up from `floor_level`."""
        t = TIER_MAP.get(tier.lower(), TIER_MAP["stone"])
        return self.add_piece(
            t["stairs"],
            gx * self.grid_size,
            gy * self.grid_size,
            floor_level * self.wall_height + 20.0,
            yaw=yaw,
        )

    def add_grid_wall(
        self,
        gx: int,
        gy: int,
        side: str = "north",
        floor_level: int = 0,
        tier: str = "stone",
        wall_type: str = "wall",
        with_door: bool = False,
    ) -> "BaseBlueprint":
        """
        Places a wall, doorway, or window on the specified edge ('north', 'south', 'east', 'west')
        of grid cell (gx, gy) at `floor_level`.
        """
        t = TIER_MAP.get(tier.lower(), TIER_MAP["stone"])
        half = self.grid_size * 0.5
        cx = gx * self.grid_size
        cy = gy * self.grid_size
        cz = floor_level * self.wall_height + 25.0

        side_map: Dict[str, Tuple[float, float, float]] = {
            "north": (cx + half, cy, 0.0),
            "south": (cx - half, cy, 180.0),
            "east": (cx, cy + half, 90.0),
            "west": (cx, cy - half, 270.0),
        }
        wx, wy, yaw = side_map[side.lower()]
        item_key = t.get(wall_type, t["wall"])
        self.add_piece(item_key, wx, wy, cz, yaw=yaw)

        if wall_type == "doorway" and with_door:
            self.add_piece(t["door"], wx, wy, cz, yaw=yaw)
        return self

    def add_box_fortress(
        self,
        width: int = 2,
        length: int = 2,
        floors: int = 1,
        tier: str = "stone",
        include_roof: bool = True,
    ) -> "BaseBlueprint":
        """Procedurally generates a complete multi-story rectangular fortress with doorway, windows, stairs, and roof."""
        half_w = width // 2
        half_l = length // 2
        gx_range = range(-half_w, -half_w + width)
        gy_range = range(-half_l, -half_l + length)

        # 1. Foundations
        for gx in gx_range:
            for gy in gy_range:
                self.add_grid_foundation(gx, gy, tier=tier)

        # 2. Floors & Walls per story
        for level in range(floors):
            for gx in gx_range:
                for gy in gy_range:
                    # South perimeter wall (front entrance on ground floor center)
                    if gx == gx_range[0]:
                        is_entrance = level == 0 and gy == 0
                        wtype = "doorway" if is_entrance else ("window" if level > 0 else "wall")
                        self.add_grid_wall(
                            gx, gy, "south", floor_level=level, tier=tier, wall_type=wtype, with_door=is_entrance
                        )
                    # North perimeter wall
                    if gx == gx_range[-1]:
                        wtype = "window" if level > 0 else "wall"
                        self.add_grid_wall(gx, gy, "north", floor_level=level, tier=tier, wall_type=wtype)
                    # West perimeter wall
                    if gy == gy_range[0]:
                        wtype = "window" if level > 0 else "wall"
                        self.add_grid_wall(gx, gy, "west", floor_level=level, tier=tier, wall_type=wtype)
                    # East perimeter wall
                    if gy == gy_range[-1]:
                        wtype = "window" if level > 0 else "wall"
                        self.add_grid_wall(gx, gy, "east", floor_level=level, tier=tier, wall_type=wtype)

            # Upper floor tiles (leave one stair opening at corner if multi-story)
            if level > 0:
                for gx in gx_range:
                    for gy in gy_range:
                        if gx == gx_range[-1] and gy == gy_range[-1]:
                            continue
                        self.add_grid_floor(gx, gy, floor_level=level, tier=tier)

            # Stairs to next floor
            if level < floors - 1:
                self.add_grid_stairs(gx_range[-1], gy_range[-1], floor_level=level, tier=tier)

        # 3. Roof
        if include_roof:
            for gx in gx_range:
                for gy in gy_range:
                    self.add_grid_floor(gx, gy, floor_level=floors, tier=tier)

        return self

    def save(self) -> List[Path]:
        """Saves the blueprint JSON to both Tools/BaseBuilder/Blueprints and Saved/BaseBlueprints."""
        repo_root = Path(__file__).resolve().parents[2]
        out_dirs = [
            repo_root / "Tools" / "BaseBuilder" / "Blueprints",
            repo_root / "Saved" / "BaseBlueprints",
        ]
        payload = {
            "name": self.name,
            "grid_size": self.grid_size,
            "wall_height": self.wall_height,
            "piece_count": len(self.pieces),
            "pieces": self.pieces,
        }
        saved_paths: List[Path] = []
        for d in out_dirs:
            d.mkdir(parents=True, exist_ok=True)
            p = d / f"{self.name}.json"
            p.write_text(json.dumps(payload, indent=2), encoding="utf-8")
            saved_paths.append(p)
        print(f"[BaseBuilder] Saved '{self.name}' ({len(self.pieces)} pieces) -> {saved_paths[0]}")
        return saved_paths
