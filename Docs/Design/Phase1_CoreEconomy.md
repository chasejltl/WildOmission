# Phase 1: Core Economy, Primitives, and Basic Combat

## Overview
Phase 1 establishes the Rust-styled resource loop, tool-tier harvesting math, primitive weapons and ammunition, and 4+1 building block upgrade and stability system.

## 1. Resource Loop & Smelting
- **Raw Resource Nodes (`Source/GatherableResources/Public/Actors/OreResourceNodes.h`):**
  - `AWoodLogNode`: Yields `Wood` (`EToolType::WOOD`, `MinimumToolTier = Hand`).
  - `AStoneOreNode`: Yields `Stone` (`EToolType::STONE`, `MinimumToolTier = Hand`).
  - `AMetalOreNode`: Yields `MetalOre` (`EToolType::STONE`, `MinimumToolTier = Stone`) + bonus `HighQualityMetalOre` when harvested with `EToolTier::Metal` or higher.
  - `ASulfurOreNode`: Yields `SulfurOre` (`EToolType::STONE`, `MinimumToolTier = Stone`).
  - `AHQMOreNode`: Yields `HighQualityMetalOre` (`EToolType::STONE`, `MinimumToolTier = Metal`).
- **Animal Harvesting (`ARagdoll`):**
  - Harvesting animal/corpse ragdolls yields `AnimalFat`, `BoneFragments`, and rare `Cloth`.
- **Furnace / Campfire Smelting (`AItemSmelterBase`):**
  - Burns `Wood` (`1`) to produce `Charcoal` (`1`) as a byproduct while smelting `MetalOre -> MetalFragments`, `SulfurOre -> Sulfur`, and `HighQualityMetalOre -> HighQualityMetal`.

## 2. Tool Tiers & Yield Scaling
- Defined in `EToolTier` (`FHarvestTierMath` in `ToolType.h`):
  - `Hand` (`0.5x` yield)
  - `Stone` (`1.0x` yield) — `StoneHatchet`, `StonePickaxe`
  - `Metal` (`1.5x` yield) — `MetalHatchet`, `MetalPickaxe`
  - `Salvaged` (`2.0x` yield)

## 3. Primitive Weapons & Workbench-Gated Ammo
- **Weapons (`Source/Weapons/Public/Items/`):**
  - `ASpearItem`: `WoodenSpear` and `StoneSpear` (320 cm melee reach).
  - `ABowItem` & `ACrossbowItem`: `HuntingBow` and `Crossbow` (1.35x velocity multiplier, supports switching between `WoodenArrow` and `BoneArrow`).
  - `AHandmadeRevolverItem`: 6-shot primitive revolver firing `PistolBullet`.
- **Projectiles & Ammo (`FCraftingRecipe::RequiredWorkbenchTier`):**
  - `WoodenArrow` (Tier 0, 35 dmg, 4500 cm/s)
  - `BoneArrow` / `ABoneArrowProjectile` (Tier 0, 42 dmg, 3900 cm/s, 12 cm collision radius)
  - `PistolBullet` (Workbench Tier 1, 38 dmg, 12000 cm/s)
  - `RifleBullet` (Workbench Tier 2, 55 dmg, 22000 cm/s)

## 4. 4+1 Building Block Tiering & Stability
- Defined in `EBuildingTier` and `FBuildingTierRules` (`BuildingBlock.h`):
  - `Twig` (`50 HP`, decays at 2 HP/min, breakable by hand tools)
  - `Wood` (`250 HP`, costs `150 Wood`, breakable by hand tools)
  - `Stone` (`500 HP`, costs `200 Stone`, immune to hand/melee tools)
  - `SheetMetal` (`1000 HP`, costs `150 MetalFragments`, immune to hand/melee tools)
  - `Armored` (`2000 HP`, costs `20 HighQualityMetal`, immune to hand/melee tools)
- **Stability:** Foundations have `0` support hops (`1.0` stability); each unsupported hop loses `0.15` stability and collapses below `0.05`.
