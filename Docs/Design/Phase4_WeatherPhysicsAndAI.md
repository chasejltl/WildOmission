# Phase 4: Extreme Weather, Tornado Structural Physics & Tactical AI

## Overview

Phase 4 elevates Wild Omission's signature natural disasters and environmental threats by integrating Rust-inspired thermal survival, structural wind physics, weather-harvesting deployables, and tactical human/boss AI (`AScientistAICharacter` and `APatrolHelicopterAI`).

All Phase 4 systems are governed by live server admin toggles in [`UServerRules`](../../Source/GameSettings/Public/ServerRules.h):
- `bTemperatureEnabled` (`true` by default)
- `bStructuralWindDestructionEnabled` (`true` by default)
- `bAdvancedAIEnabled` (`true` by default)

---

## 1. Body Temperature Vital & Extreme Weather Hazards

### `UVitalsComponent` Thermal Simulation
- **Homeostasis Baseline:** `37.0°C` (`CurrentBodyTemperatureCelsius`).
- **Hypothermia (`< 35.0°C`):** Drains health proportionally to cold severity (`(35.0 - CurrentBodyTemp) * 0.45 HP/s`).
- **Hyperthermia (`> 39.5°C`):** Drains health (`0.35 HP/s`) and accelerates thirst depletion (`0.8 Thirst/s`).
- **Clothing Insulation & Shelter/Campfire Warmth:**
  - Clothing items provide `ColdInsulationCelsius` (e.g., Burlap/Hide/RoadSign/Hazmat insulation bonuses).
  - Standing indoors under a roof or beside an active Campfire/Furnace (`SetShelteredOrNearFire(true)`) enforces a minimum effective ambient temperature of `21.0°C`, warming hypothermic players back toward `37.0°C`.

### `AExtremeWeatherHazardZone` (`Source/Weather/Public/Actors/ExtremeWeatherEvents.h`)
| Weather Event | Ambient Temp (°C) | Acid Corrosion (HP/s) | Radiation (rads/s) | Visibility Multiplier |
| :--- | :---: | :---: | :---: | :---: |
| **Clear** | `21.0°C` | `0.0` | `0.0` | `1.00x` |
| **Thunderstorm** | `14.0°C` | `0.0` | `0.0` | `0.65x` |
| **Acid Rain** | `16.0°C` | `1.5` (unsheltered) | `2.0` | `0.55x` |
| **Blizzard** | `-18.0°C` | `0.0` | `0.0` | `0.25x` |
| **Sandstorm** | `46.0°C` | `0.0` | `0.0` | `0.20x` |

---

## 2. Tornado Structural Wind Stripping & Weather Deployables

### Tier-Scaled Tornado Wind Stripping (`ABuildingBlock::ApplyWindDamage`)
When `ATornado`'s `UWindSuckerComponent` overlaps player-built structures, [`ABuildingBlock::ApplyWindDamage`](../../Source/Deployables/Private/Deployables/BuildingBlock.cpp) scales structural wind damage by [`FBuildingTierRules::GetWindDamageMultiplierForTier`](../../Source/Deployables/Public/Deployables/BuildingBlock.h):
- **Twig (`3.0x`):** Rapidly ripped apart by passing tornadoes.
- **Wood (`1.5x`):** Vulnerable to sustained direct hits.
- **Stone (`0.25x`):** High wind resistance; survives peripheral passes.
- **Sheet Metal (`0.05x`):** Near-impervious to wind shear.
- **Armored (`0.0x`):** Completely immune to tornado wind stripping.

### Weather Deployables (`Source/Deployables/Public/Deployables/WeatherDeployables.h`)
- **Rain Collector (`ARainCollectorDeployable`):** Passively harvests fresh drinking water (`up to 500 mL`) during rainstorms and restores player Thirst on interaction.
- **Weather Radar (`AWeatherRadarDeployable`):** Consumes `5W` from the Phase 2 electrical grid (`UElectricNodeComponent`) to detect active storms, tornadoes, and extreme weather hazard zones.

---

## 3. Tactical Scientist AI & Patrol Helicopter World Boss

### Monument Scientist AI (`AScientistAICharacter`)
- **Perception:** Sight (`3500 cm`) and distance-attenuated sound perception (`ReportHeardSound` within `2500 cm`).
- **Tactical State Machine (`EvaluateTacticalState`):**
  1. `HealWithSyringe`: Triggered when health `<= 35%` and `RemainingSyringes > 0` (restores `+35 HP`).
  2. `EngageRanged`: Triggered when target is in line of sight within `SightRadiusCm`.
  3. `ThrowGrenade`: Flushes targets that duck behind cover within `2500 cm` (`SecondsSinceTargetSeen < 8.0s`).
  4. `FlankTarget`: Executed when target breaks line of sight for `8.0s - 30.0s`.
  5. `SeekCover` / `Patrol`: Default fallback states.

### Patrol Helicopter (`APatrolHelicopterAI`)
- **Rust Threat Rules (`IsPlayerHostileThreat`):** Ignores naked/primitive players (`0` ranged weapons and `<= 2` clothing pieces); engages players carrying any ranged weapon or wearing `> 2` clothing/armor pieces.
- **Weakspot Rotor Destruction (`ApplyHelicopterDamage`):** Direct hits to the **Main Rotor (`1000 HP`)** or **Tail Rotor (`500 HP`)** deal `2.5x` hull damage (`10000 HP` total). Destroying both rotors immediately crashes the helicopter and spawns `4` high-tier crates.
