# Development Log

Chronological record of the Rust-styled survival work. One entry per working session / feature branch.
Format: **what** changed, **why**, **how it was verified**, **what is next**. Design docs for individual systems live in `Docs/Design/`.

---

## 2026-11-04 - Phase 0 kickoff

**Branch:** `chore/p0-docs-and-lfs`

### Done
- Reviewed the Steam page, patch notes, discussions and guides, the Rust design brief and the code base. Findings are in `Docs/ROADMAP.md` (sections 1-3, 9).
- Installed tooling: Unreal Engine 5.8.4, Blender 5.2, Visual Studio 2022 Build Tools (C++ game workload, Win11 SDK), git-lfs.
- Added roadmap, design brief and art reference to `Docs/`.
- Enabled git-lfs for new source art/audio (`.fbx .blend .wav .mp3 .ogg .png .tga .psd`). `.uasset/.umap` are *not* migrated: existing history stays untouched and no force-push is needed.

### Decisions (from project owner)
- Rust features are **server toggles**, adjustable from an admin panel and easy to mod.
- Work happens on feature branches, merged via PR into `main` of the `chasejltl/WildOmission` fork only.
- UE 5.8 upgrade stays on its own branch until stable.

### Findings
- Upstream has an abandoned `Unreal_5-7_Migration` branch (dev rolled back in June 2026 citing instability). Used as reference for API fixes.
- Upstream `beta-1-4-0` has unreleased character-customization work; not merged.

### Next
- `feature/p0-ue58-upgrade`: get the project compiling on UE 5.8.4.

---

## 2026-11-05 - Phase 0: UE 5.8.4 Engine Upgrade & Asset Pipeline

**Branch:** `feature/p0-ue58-upgrade`

### What changed & why
1. **Project & Target Rules (`WildOmission.uproject`, `Source/*.Target.cs`):**
   - Associated project with Unreal Engine `5.8` (`5.8.4`).
   - Removed obsolete `bUseIris` and `bUsesSteam` properties from `TargetRules`.
   - Upgraded `DefaultBuildSettings` from `BuildSettingsVersion.V4` to `BuildSettingsVersion.V7` so `WildOmissionEditorTarget` matches `UnrealEditor`'s warning and floating-point semantics.
2. **Breaking Circular Module Dependency (`Structures` module split):**
   - UE 5.8's `UnrealBuildTool` enforces stricter module cycle validation and rejected the `WorldGeneration <-> Deployables` cycle caused by `ULootSpawnComponent` spawning `ADeployable`.
   - Extracted `AStructure` and `ULootSpawnComponent` into a new `Structures` runtime module (`Source/Structures/`) with explicit `CircularlyReferencedDependentModules` and added `[CoreRedirects]` in `Config/DefaultEngine.ini` so existing Blueprints (`/Script/WorldGeneration.Structure` and `/Script/WorldGeneration.LootSpawnComponent`) resolve seamlessly.
3. **UE 5.8 C++ API & Strictness Fixes:**
   - Replaced removed `TArray::SetNum(int32, bool)` overload with `EAllowShrinking::No` in `UInventoryComponent`.
   - Added explicit `#include "Engine/OverlapResult.h"` in `ATornado`, `UWindSuckerComponent`, and `ARocketProjectile`.
   - Updated `SteamHelpers` and `Workshop` to include `steam/steam_api.h` via the `Steamworks` module's public system include path and exposed `Steamworks` publicly on `Workshop`.
   - Fixed compile-time `FString::Printf` / `UE_LOG` format string validation errors in `ULoadingMenuWidget` (`%%` escape) and `AWildOmissionGameMode` (missing `%s` specifiers).
   - Linked `CoreOnline` in `Locks`, `Deployables`, `MenuSystem`, and `WildOmissionCore` for `FUniqueNetIdWrapper::ToString()`.
   - Fixed `C4996` deprecation warnings (`USkeletalMeshComponent::SetAnimInstanceClass` and `AActor::SetNetUpdateFrequency`).
4. **Headless Blender 5.2 Pipeline & Build Tooling:**
   - Added `Art/Palette.json` (24-color low-poly flat-shaded palette), `Art/Blender/export_fbx.py`, and `Art/Blender/generate_metal_ore_node.py`, producing `SM_MetalOreNode.blend` and `SM_MetalOreNode.fbx` (tracked via Git LFS).
   - Added `Tools/Build.bat` wrapper for CLI builds.

### How it was verified
- Full `WildOmissionEditor Win64 Development` build succeeded on UE 5.8.4 (`Result: Succeeded`, producing `Binaries/Win64/UnrealEditor-*.dll` for all 40 project modules).
- Verified headless Blender 5.2.3 LTS generation and FBX export (`Art/Export/SM_MetalOreNode.fbx`).

### Next
- Smoke-test editor/game runtime on `feature/p0-ue58-upgrade`, implement `UServerRules` (admin-configurable server toggles), and merge `feature/p0-ue58-upgrade` to `main` once verified stable.

---

## 2026-11-05 - Phase 0 Completion (`UServerRules` & QoL) and Phase 1 (`feature/p1-core-economy`)

**Branches:** `feature/p0-server-rules-and-qol`, `feature/p1-core-economy` (merged to `main`)

### What changed & why
1. **Phase 0 Server Rules & Community QoL (`feature/p0-server-rules-and-qol`):**
   - Added `UServerRules` (`Source/GameSettings/Public/ServerRules.h` & `Private/ServerRules.cpp`), a `UGameInstanceSubsystem` backed by `Config=Game` with live console/RCON commands (`wo.rules.list`, `wo.rules.set <RuleName> <Value>`) so every Rust-inspired survival feature is toggleable by server admins and modders.
   - Doubled the default day/night cycle length in `ATimeOfDayManager` (from 20 minutes to 40 minutes) and added `SetDayNightSpeedMultiplier`.
   - Tripled the default torch burn duration in `ATorchItem` (`BurnTickInterval = 3.0f` seconds instead of `1.0f`).
   - Added design documentation in `Docs/Design/ServerRules.md`.
2. **Phase 1: Core Economy, Primitives, and Basic Combat (`feature/p1-core-economy`):**
   - **Tool-Tier Yield Scaling & Node Gates (`EToolTier`, `FHarvestTierMath`):** Implemented `Hand` (`0.5x`), `Stone` (`1.0x`), `Metal` (`1.5x`), and `Salvaged` (`2.0x`) yield multipliers and minimum tool tier gates on `AHarvestableResource`, `UHarvestableComponent`, and `AToolItem`.
   - **Raw Resource Nodes & Animal/Smelter Loops:**
     - Added `AWoodLogNode`, `AStoneOreNode`, `AMetalOreNode` (with HQM quality-tool bonus drop), `ASulfurOreNode`, and `AHQMOreNode` (`Source/GatherableResources/Public/Actors/OreResourceNodes.h`).
     - Configured `ARagdoll` (`Ragdoll.cpp`) to drop `AnimalFat` (`10`), `BoneFragments` (`15`), and rare `Cloth` (`5`) when harvested.
     - Configured `AItemSmelterBase` (`ItemSmelterBase.cpp`) with default `Wood -> Charcoal` byproduct and smelting for `MetalOre -> MetalFragments`, `SulfurOre -> Sulfur`, and `HighQualityMetalOre -> HighQualityMetal`.
   - **Data-Driven Item & Workbench-Gated Recipe Registries:**
     - Registered all 26 Phase 1 resources, tiered tools (`StoneHatchet`, `StonePickaxe`, `MetalHatchet`, `MetalPickaxe`), primitive weapons, and ammo types in `UInventoryComponent` (`InventoryComponent.cpp`).
     - Added `RequiredWorkbenchTier` (`0..3`) to `FCraftingRecipe` (`CraftingRecipe.h`) and registered all Phase 1 crafting recipes in `UCraftingComponent` (`CraftingComponent.cpp`).
   - **Primitive Weapons & Ammo (`Source/Weapons/`):**
     - Added `ASpearItem` (`SpearItem.h/.cpp` for long-reach Wooden and Stone spears), `ACrossbowItem` (`CrossbowItem.h/.cpp` with 1.35x velocity multiplier and `WoodenArrow`/`BoneArrow` selection), `AHandmadeRevolverItem` (`HandmadeRevolverItem.h/.cpp`, 6-shot `PistolBullet` cylinder), and `ABoneArrowProjectile` (`BoneArrowProjectile.h/.cpp`).
   - **4+1 Building Block Tiering, Stability & Twig Decay (`ABuildingBlock`):**
     - Added `EBuildingTier` (`Twig -> Wood -> Stone -> SheetMetal -> Armored`) and `FBuildingTierRules` (`50 / 250 / 500 / 1000 / 2000 HP`, upgrade costs, hand-tool damage immunity on `Stone+` tiers, periodic decay on `Twig`, and support-hop structural stability calculation).
   - **Low-Poly Blender 5.2 Assets (`Art/Blender/generate_phase1_assets.py`):**
     - Generated flat-shaded low-poly `.blend` and `.fbx` assets for `SM_SulfurOreNode`, `SM_HQMOreNode`, `SM_WoodenSpear`, `SM_Crossbow`, `SM_HandmadeRevolver`, and `SM_BoneArrow`.

### How it was verified
- Compiled `WildOmissionEditor Win64 Development` cleanly on UE 5.8.4 (`Result: Succeeded`).
- Executed headless UE 5.8 automation test suite (`WildOmission.Phase1` in `Source/WildOmissionCore/Tests/Phase1EconomyTests.cpp`) via `UnrealEditor-Cmd.exe -NullRHI`:
  - `WildOmission.Phase1.HarvestTierScaling`: **Success**
  - `WildOmission.Phase1.BuildingTiers`: **Success**
  - `WildOmission.Phase1.ItemsAndRecipes`: **Success**

### Next
- **Phase 2 (`feature/p2-industrial-and-upkeep`):** Workbenches (Tier 1/2/3), Tool Cupboard 24h Upkeep & Territory Privilege enforcement, Code/Key Locks, Furnaces/Refineries, and Electricity Grid (`Generator`, `Switch`, `Splitter`, `Auto-Turret`, `Shotgun Trap`).

---

## 2026-11-05 - Phase 2: Industrialization, Workbenches, Base Upkeep & Electricity

**Branch:** `feature/p2-industrial-and-upkeep`

### What changed & why
1. **Workbenches (Tiers 1, 2, 3), Research Table & Scrap Blueprints (`Source/Deployables/Public/Deployables/WorkbenchDeployable.h`, `Source/Crafting/`):**
   - Added `AWorkbenchDeployable` (`AWorkbenchTier1`, `AWorkbenchTier2`, `AWorkbenchTier3`) with automatic `400 cm` proximity detection hooked into `UCraftingComponent::GetEffectiveWorkbenchTier()`.
   - Added `ResearchScrapCost` to `FCraftingRecipe` and `UnlockBlueprint` / `IsBlueprintUnlocked` / `ResearchRecipeWithScrap` / `CanCraftRecipe` to `UCraftingComponent`, respecting `UServerRules::bWorkbenchTaxEnabled`.
   - Added `AResearchTableDeployable` allowing players to spend `Scrap` to permanently unlock industrial/electrical recipes.
2. **Deployables Expansion (`Source/Locks/Public/Locks/KeyLock.h`, `Source/Deployables/Public/Deployables/IndustrialDeployables.h`):**
   - Added `AKeyLock` (`100 Wood` early-game lock with direct owner lock/unlock interaction).
   - Added `ALargeStorageBox` (`30` slots), `ASmallFurnaceDeployable` (`6` slots, `3.0s` smelt cycle), `ALargeFurnaceDeployable` (`18` slots, `1.5s` smelt cycle = 2x speed), and `AOilRefineryDeployable` (`1 CrudeOil -> 3 LowGradeFuel`).
3. **Tool Cupboard 24-Hour Upkeep & Starvation Decay (`Source/Deployables/Public/Deployables/ToolCupboard.h`, `BuildingBlock.h`):**
   - Added 18-slot `UpkeepInventory` to `AToolCupboard` and bracketed daily upkeep taxation (`10% / 15% / 20% / 33%` scaled by `UServerRules::UpkeepRateMultiplier`).
   - Implemented `ProcessUpkeepCycle(float HoursElapsed)` and `GetProtectedHoursRemaining()`, deducting `Wood`, `Stone`, `MetalFragments`, and `HighQualityMetal` and applying tier-scaled decay damage (`Wood=3h`, `Stone=5h`, `SheetMetal=8h`, `Armored=12h`) to starving `ABuildingBlock` actors when `UServerRules::bBuildingDecayEnabled` is active.
4. **Electricity Node Graph & Automated Base Defenses (`Source/Deployables/Public/Components/ElectricNodeComponent.h`, `ElectricalDeployables.h`):**
   - Implemented `UElectricNodeComponent` supporting `Generator`, `Battery`, `Switch`, `Splitter`, and `Consumer` nodes with cycle-safe power propagation and `UServerRules::bElectricityEnabled` support.
   - Added `ASolarPanelDeployable` (`20W`), `ASmallBatteryDeployable` (`10W` output), `AElectricSwitchDeployable`, `AElectricSplitterDeployable` (3-way), `AAutoTurretDeployable` (`10W` consumer, `35` damage, TC/turret authorization check), and `AShotgunTrapDeployable` (`0W` mechanical trap, `120` buckshot damage).
5. **Blender 5.2 Low-Poly Assets (`Art/Blender/generate_phase2_assets.py`):**
   - Generated `.blend` and `.fbx` meshes for `SM_WorkbenchT1`, `SM_WorkbenchT2`, `SM_WorkbenchT3`, `SM_ResearchTable`, `SM_KeyLock`, `SM_LargeStorageBox`, `SM_LargeFurnace`, `SM_OilRefinery`, `SM_SolarPanel`, `SM_AutoTurret`, and `SM_ShotgunTrap`.

### How it was verified
- Compiled `WildOmissionEditor Win64 Development` on UE 5.8.4.
- Added and executed headless UE 5.8 automation tests (`WildOmission.Phase2` in `Source/WildOmissionCore/Tests/Phase2IndustrialTests.cpp`):
  - `WildOmission.Phase2.WorkbenchesAndResearch`
  - `WildOmission.Phase2.ToolCupboardUpkeep`
  - `WildOmission.Phase2.ElectricityAndTraps`

### Next
- **Phase 3 (`feature/p3-ballistics-vitals-and-monuments`):** Military-grade firearms (Bolt-Action Rifle, Custom SMG, Assault Rifle recoil/ballistics, C4 & Rocket raiding), Medical & Hazmat/Armor clothing modifiers, Radiation & Bleeding vitals, Hemp/Corn/Pumpkin crop genetics, and procedural Monuments (Airfield, Power Plant, Military Tunnels).