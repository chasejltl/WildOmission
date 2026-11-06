# Phase 3 Design: Advanced Ballistics, Raiding, Vitals, Crop Genetics & Monuments

## Overview
Phase 3 introduces military-grade firearms and ballistic recoil/drop physics, C4 and Rocket structural raiding with Offline Raid Protection, Radiation & Bleeding vitals with Medical items and Hazmat/Armor clothing protection, 6-gene agricultural genetics for Hemp/Corn/Pumpkin, and procedural Monuments (`Airfield`, `Power Plant`, `Military Tunnels`).

## 1. Military Firearms, Ballistics & Raiding Explosives
- **Source:** `Source/Weapons/Public/Items/MilitaryFirearms.h`
- **Firearms & Recoil (`FBallisticRecoilProfile`):**
  - `AAssaultRifleItem` (`AssaultRifle`): 450 RPM, 30-round `RifleBullet` magazine, deterministic S-curve horizontal yaw & climbing vertical pitch recoil (`GetAssaultRifleRecoilForShot`).
  - `ABoltActionRifleItem` (`BoltActionRifle`): 35 RPM, 4-round `RifleBullet` magazine, `1.6x` muzzle velocity multiplier for flat long-range trajectories (`CalculateBulletDropCm`).
  - `ACustomSMGItem` (`CustomSMG`): 600 RPM, 24-round `PistolBullet` magazine, compact close-quarters recoil profile.
- **Raiding Explosives (`FRaidExplosiveDamage`, `ATimedExplosiveChargeProjectile`):**
  - `TimedExplosiveCharge` (`C4`): `10.0s` fuse, `550` structural damage (1 C4 destroys Wood [250 HP] or Stone [500 HP]; 2 C4 destroy Sheet Metal [1000 HP]; 4 C4 destroy Armored [2000 HP]).
  - `Rocket`: `275` structural splash damage.
  - Respects `UServerRules::bOfflineRaidProtection` (`0.5x` damage multiplier when target base owner is offline).

## 2. Vitals (Radiation & Bleeding), Medicals & Clothing Protection
- **Source:** `Source/Vitals/Public/Components/VitalsComponent.h`, `Source/Clothing/Public/Clothing/Clothing.h`
- **Clothing & Armor Presets (`FClothingProtectionStats`):**
  - `HazmatSuit`: `50%` radiation resistance (completely blocks up to `25 rads/s` ambient monument radiation), `30%` projectile/melee protection, `+8°C` cold insulation.
  - `RoadSignJacket` / `RoadSignKilt`: Tier 2 armor (`30–35%` projectile, `40–45%` melee protection).
  - `MetalFacemask` / `MetalChestplate`: Tier 3 HQM armor (`50%` projectile, `65–70%` melee protection).
- **Medical Recovery (`UVitalsComponent::ApplyMedicalItem`):**
  - `Bandage`: Clears `CurrentBleeding` to `0` and heals `+5` HP.
  - `MedicalSyringe`: Restores `+35` total HP (`+15` instant + `+20` HoT) and purges `10` Radiation.
  - `AntiRadPills`: Purges `75` Radiation at the cost of `5` Thirst.

## 3. 6-Gene Crop Genetics (Hemp, Corn, Pumpkin)
- **Source:** `Source/Deployables/Public/Deployables/CropGenetics.h`
- **Alleles (`ECropGene`):**
  - Positive genes (`0.6` dominance weight): `G` (`+15%` growth speed), `Y` (`+25%` harvest yield), `H` (`+20%` hardiness).
  - Negative genes (`1.0` dominance weight): `W` (`+25%` water consumption), `X` (null/empty allele).
- **Crossbreeding (`FCropGeneSequence::CrossbreedWithNeighbors`):**
  - Evaluates allele dominance weights per gene slot `0..5` across neighboring plants; two matching beneficial alleles (`0.6 + 0.6 = 1.2`) out-compete a dominant negative `W`/`X` allele (`1.0`).

## 4. Procedural Monuments & 15-Minute Hackable Locked Crates
- **Source:** `Source/Structures/Public/Structures/MonumentStructure.h`
- **Monument Tiers:**
  - `AAirfieldMonument` (Tier 1): `6000 cm` radius, `5 rads/s`, `7500 cm` build-exclusion zone.
  - `APowerPlantMonument` (Tier 2): `8000 cm` radius, `15 rads/s`, `10000 cm` build-exclusion zone.
  - `AMilitaryTunnelsMonument` (Tier 3): `9000 cm` radius, `25 rads/s`, `11500 cm` build-exclusion zone.
- **Hackable Locked Crate (`FHackableLockedCrateState`):**
  - Requires a `900-second` (15-minute) hack countdown (`StartHacking()` / `AdvanceHackTimer()`) before unlocking tiered high-value military and raiding loot.
