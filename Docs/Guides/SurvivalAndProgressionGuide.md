# Wild Omission: Rust-Styled Survival & Progression Guide

This guide covers all survival, base-building, industrial, electrical, monument, agricultural, weather, and combat systems introduced across Phases 0–4 of the Rust-styled survival expansion.

---

## 1. Resource Gathering & Tool Tiers (Phase 1)

Wild Omission features a 4-tier harvesting progression (`EToolTier`):
| Tool Tier | Yield Multiplier | Unlocks Harvesting For |
| :--- | :---: | :--- |
| **Hand / Rock (`Hand`)** | `0.5x` | Wood Logs, Stone Nodes |
| **Stone Tools (`Stone`)** | `1.0x` | Metal Ore (`MetalOre`), Sulfur Ore (`SulfurOre`) |
| **Metal Tools (`Metal`)** | `1.5x` | High Quality Metal Ore (`HighQualityMetalOre`) + bonus HQM on Metal Ore |
| **Salvaged Tools (`Salvaged`)** | `2.0x` | Maximum yield across all nodes |

### Smelting & Fuel Loop
- **Charcoal:** Produced automatically as a byproduct whenever `Wood` burns inside a Campfire or Furnace (`1 Wood -> 1 Charcoal`).
- **Low Grade Fuel:** Crafted from `3 AnimalFat + 1 Cloth` (harvested from animal ragdolls) or refined from `CrudeOil` at an **Oil Refinery** (`1 CrudeOil -> 3 LowGradeFuel`).
- **Furnaces:**
  - `SmallFurnace`: 6 slots, `3.0s` smelt interval (`MetalOre -> MetalFragments`, `SulfurOre -> Sulfur`, `HighQualityMetalOre -> HighQualityMetal`).
  - `LargeFurnace`: 18 slots, `1.5s` smelt interval (`2x` smelting speed).

---

## 2. Base Building, Upkeep & Fortification (Phases 1 & 2)

### 4+1 Structural Building Tiers (`ABuildingBlock`)
| Tier | Max HP | Upgrade Cost | Hand-Tool Breakable | Tornado Wind Multiplier | Starvation Decay Time |
| :--- | :---: | :--- | :---: | :---: | :---: |
| **0. Twig** | `50` | `25 Wood` | Yes | `3.00x` | `1 hour` |
| **1. Wood** | `250` | `150 Wood` | Yes | `1.50x` | `3 hours` |
| **2. Stone** | `500` | `200 Stone` | No | `0.25x` | `5 hours` |
| **3. Sheet Metal** | `1000` | `150 MetalFragments` | No | `0.05x` | `8 hours` |
| **4. Armored** | `2000` | `20 HighQualityMetal` | No | `0.00x` (Immune) | `12 hours` |

### Structural Stability
- Foundations touching terrain have `100%` (`1.0`) stability.
- Each unsupported hop away from a foundation reduces stability by `15%` (`0.15`). Blocks below `5%` stability collapse.

### Tool Cupboard (TC) 24-Hour Upkeep Tax
- Place a **Tool Cupboard (`AToolCupboard`)** inside your base and authorize yourself to claim building privilege within its radius.
- Deposit `Wood`, `Stone`, `MetalFragments`, and `HighQualityMetal` into the TC's 18-slot upkeep inventory.
- Daily tax brackets scale with base size:
  - `1–15 blocks`: `10%` of total upgrade cost per 24 hours
  - `16–50 blocks`: `15%` per 24 hours
  - `51–100 blocks`: `20%` per 24 hours
  - `101+ blocks`: `33%` per 24 hours

---

## 3. Workbenches, Scrap Research & Electricity (Phase 2)

### Workbenches & Research Table
- Stand within `4 meters` (`400 cm`) of a **Workbench Tier 1**, **Tier 2**, or **Tier 3** to craft gated recipes.
- Use a **Research Table (`AResearchTableDeployable`)** and spend `Scrap` to permanently unlock industrial, electrical, and military blueprints on your character.

### Base Electricity & Automated Traps
- **Power Sources:** `SolarPanel` (`20W` peak output) and `SmallBattery` (`10W` output, `150W-min` capacity).
- **Routing:** `ElectricSwitch` (on/off toggle) and `ElectricSplitter` (divides input power equally across up to 3 connected outputs).
- **Automated Defenses:**
  - `AutoTurret`: Requires `10W` power; fires at unauthorized players within `25 meters` (`35` damage per shot).
  - `ShotgunTrap`: Requires `0W` (mechanical trigger); blasts unauthorized intruders in front of doorways (`120` buckshot damage).

---

## 4. Military Ballistics, Raiding, Monuments & Genetics (Phase 3)

### Military Firearms & Raiding Explosives
- **Assault Rifle (`AAssaultRifleItem`):** `450 RPM`, 30-round `RifleBullet` magazine, deterministic S-curve recoil pattern.
- **Bolt-Action Rifle (`ABoltActionRifleItem`):** `35 RPM`, 4-round `RifleBullet` magazine, `1.6x` muzzle velocity for long-range precision.
- **Custom SMG (`ACustomSMGItem`):** `600 RPM`, 24-round `PistolBullet` magazine for close-quarters monument clearing.
- **Raiding:**
  - `TimedExplosiveCharge` (`C4`): `550` structural damage (`10s` fuse).
  - `Rocket`: `275` structural splash damage.
  - When `bOfflineRaidProtection` is enabled on the server, bases whose authorized owners are all offline take `50%` (`0.5x`) explosive damage.

### Procedural Monuments & Radiation
| Monument | Tier | Radiation (`rads/s`) | Recommended Gear |
| :--- | :---: | :---: | :--- |
| **Airfield (`AAirfieldMonument`)** | Tier 1 | `5.0 rads/s` | Burlap / Hide Clothing (`15–25%` rad resist) |
| **Power Plant (`APowerPlantMonument`)** | Tier 2 | `15.0 rads/s` | Road Sign Armor or Hazmat Suit |
| **Military Tunnels (`AMilitaryTunnelsMonument`)** | Tier 3 | `25.0 rads/s` | **Hazmat Suit (`50%` rad resist)** + `AntiRadPills` |

- **Hackable Locked Crates:** Found at Monument centers; initiating the laptop hack starts a **15-minute (`900s`)** countdown before unlocking high-tier military and raiding loot.

### 6-Gene Crop Crossbreeding (`FCropGeneSequence`)
- **Hemp (`AHempCropSeedling`)**, **Corn (`ACornCropSeedling`)**, and **Pumpkins (`APumpkinCropSeedling`)** carry a 6-gene sequence:
  - Green beneficial alleles (`0.6` weight): `G` (Growth rate `+25%`), `Y` (Harvest yield `+30%`), `H` (Hardiness).
  - Red recessive/negative alleles (`1.0` weight): `W` (Increased water consumption), `X` (Empty/null trait).
- Crossbreeding adjacent plants selects the highest-summed allele at each of the 6 loci (`0..5`).

---

## 5. Extreme Weather, Thermal Survival & Tactical AI (Phase 4)

### Body Temperature & Extreme Weather
- Core body temperature starts at `37.0°C`.
- **Blizzards (`-18°C`)** cause **Hypothermia (`< 35.0°C`)** unless you wear insulated clothing or shelter near a campfire/indoors (`>= 21°C` effective warmth).
- **Sandstorms (`44°C`)** cause **Hyperthermia (`> 39.5°C`)** and rapid thirst drain.
- **Acid Rain** deals `2.5 HP/s` chemical corrosion and `1.5 rads/s` radiation to unsheltered players without Hazmat protection.
- Deploy **Rain Collectors (`ARainCollectorDeployable`)** outdoors to harvest fresh drinking water during storms, and wire a **Weather Radar (`AWeatherRadarDeployable`, `5W`)** into your base grid to track active weather hazard zones.

### Monument Scientists & Patrol Helicopter
- **Scientists (`AScientistAICharacter`):** Patrol monuments using sight (`35m`) and gunfire/footstep hearing (`25m`). They take cover, flush hiding players with **Frag Grenades**, flank after `8s` of lost line of sight, and heal with **Medical Syringes** when below `35%` HP.
- **Patrol Helicopter (`APatrolHelicopterAI`):** Ignores primitive survivors (`0` ranged weapons and `<= 2` clothing pieces). Engages geared players on sight. Target the **Main Rotor (`1000 HP`)** and **Tail Rotor (`500 HP`)** weakspots for `2.5x` critical damage to bring it down and claim `4` crash-site crates.
