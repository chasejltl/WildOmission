# Phase 2 Design: Industrial Crafting, Base Upkeep & Electricity

## Overview
Phase 2 expands Wild Omission from primitive survival into Rust-style base industrialization, territory upkeep taxation, and automated electrical defenses. All mechanics respect the server toggles in `UServerRules` (`bWorkbenchTaxEnabled`, `bUpkeepEnabled`, `UpkeepRateMultiplier`, `bBuildingDecayEnabled`, `bElectricityEnabled`).

## 1. Workbenches (Tier 1–3) & Research Table Blueprints
- **Source:** `Source/Deployables/Public/Deployables/WorkbenchDeployable.h`, `Source/Crafting/Public/Components/CraftingComponent.h`
- **Workbench Tiers:**
  - `AWorkbenchTier1` (`WorkbenchT1`): 500 Wood, 100 Metal Fragments, 50 Scrap (500 HP).
  - `AWorkbenchTier2` (`WorkbenchT2`): 500 Metal Fragments, 20 High-Quality Metal, 500 Scrap (750 HP).
  - `AWorkbenchTier3` (`WorkbenchT3`): 1000 Metal Fragments, 100 High-Quality Metal, 1250 Scrap (1000 HP).
- **Proximity & Interaction Detection:**
  - `AWorkbenchDeployable::GetHighestWorkbenchTierAtLocation` hooks directly into `UCraftingComponent::SetProximityWorkbenchQuery`, so players standing within `400 cm` of any placed workbench automatically gain its crafting tier in `UCraftingComponent::GetEffectiveWorkbenchTier()`.
- **Scrap Blueprint Research (`AResearchTableDeployable`):**
  - Advanced industrial and electrical recipes (`LargeFurnace`, `OilRefinery`, `SolarPanel`, `SmallBattery`, `ElectricSwitch`, `ElectricSplitter`, `ShotgunTrap`, `AutoTurret`) specify a `ResearchScrapCost` on `FCraftingRecipe`.
  - When `UServerRules::bWorkbenchTaxEnabled` is `true`, players must unlock the blueprint via `AResearchTableDeployable::ResearchItemForActor` or `UCraftingComponent::ResearchRecipeWithScrap` before crafting.

## 2. Industrial Deployables & Locks
- **Source:** `Source/Locks/Public/Locks/KeyLock.h`, `Source/Deployables/Public/Deployables/IndustrialDeployables.h`
- **`AKeyLock` (`KeyLock`):** Crafted with 100 Wood; authorizes its owner directly on placement/interaction without requiring a 4-digit keypad code.
- **`ALargeStorageBox` (`LargeStorageBox`):** 30-slot storage container (400 HP) with `ULockComponent` support.
- **`ASmallFurnaceDeployable` & `ALargeFurnaceDeployable`:**
  - Small Furnace: 6 slots, `3.0s` per smelt cycle.
  - Large Furnace: 18 slots, `1.5s` per smelt cycle (`2x` smelting throughput).
- **`AOilRefineryDeployable` (`OilRefinery`):** Distills `1 CrudeOil -> 3 LowGradeFuel` (`2.5s` cycle) using `Wood -> Charcoal` fuel.

## 3. Tool Cupboard (TC) 24-Hour Upkeep & Starvation Decay
- **Source:** `Source/Deployables/Public/Deployables/ToolCupboard.h`, `Source/Deployables/Public/Deployables/BuildingBlock.h`
- **Bracketed Daily Tax Rates (`FBuildingTierRules::GetUpkeepTaxBracketRate`):**
  - `1–15` blocks: `10%` of upgrade cost per 24 hours
  - `16–50` blocks: `15%` per 24 hours
  - `51–100` blocks: `20%` per 24 hours
  - `101+` blocks: `33%` per 24 hours
  - Scaled by `UServerRules::UpkeepRateMultiplier` and disabled when `UServerRules::bUpkeepEnabled == false`.
- **Upkeep Storage & Starvation Decay (`AToolCupboard::ProcessUpkeepCycle`):**
  - `AToolCupboard` owns an 18-slot `UpkeepInventory` storing `Wood`, `Stone`, `MetalFragments`, and `HighQualityMetal`.
  - When a required upkeep resource is depleted and `UServerRules::bBuildingDecayEnabled == true`, connected `ABuildingBlock` actors of that material tier take proportional hourly decay damage (`Wood = 3h`, `Stone = 5h`, `SheetMetal = 8h`, `Armored = 12h` to decay from 100% to 0%).

## 4. Electricity Grid & Automated Defenses
- **Source:** `Source/Deployables/Public/Components/ElectricNodeComponent.h`, `Source/Deployables/Public/Deployables/ElectricalDeployables.h`
- **Circuit Graph (`UElectricNodeComponent`):**
  - Cycle-safe recursive propagation supporting `Generator` (`ASolarPanelDeployable`, 20W), `Battery` (`ASmallBatteryDeployable`, 10W output / 150 charge), `Switch` (`AElectricSwitchDeployable`), `Splitter` (`AElectricSplitterDeployable`, equal division up to 3 outputs), and `Consumer` (`AAutoTurretDeployable`, requires 10W).
- **Automated Base Defenses:**
  - `AAutoTurretDeployable`: Requires 10W power (or operates unpowered when `UServerRules::bElectricityEnabled == false`), scans within `3000 cm`, ignores players authorized on the turret or nearby `AToolCupboard`, consumes `RifleBullet`/`PistolBullet` if stocked, and deals `35` damage per shot.
  - `AShotgunTrapDeployable`: Mechanical trap requiring `0` power, triggers within `500 cm` on unauthorized intruders, dealing `120` buckshot damage.
