# Wild Omission: Server Admin & Modding Guide

Every Rust-styled survival mechanic in Wild Omission is governed by the [`UServerRules`](../../Source/GameSettings/Public/ServerRules.h) subsystem (`UGameInstanceSubsystem`), making it straightforward for server hosts, single-player/co-op admins, and modders to tune or toggle individual features at runtime or via config files.

---

## 1. Live Console & RCON Commands

Open the in-game console (`~`) or dedicated server console to inspect, modify, or persist server rules in real time:

| Command | Usage | Description |
| :--- | :--- | :--- |
| `wo.rules.list` | `wo.rules.list` | Prints all current `UServerRules` keys and values to the log/console. |
| `wo.rules.set` | `wo.rules.set <Key> <Value>` | Updates a rule immediately and broadcasts `OnServerRuleChanged` to all active gameplay systems. |
| `wo.rules.reset` | `wo.rules.reset` | Resets all rules to standard Rust-styled survival defaults. |
| `wo.rules.save` | `wo.rules.save` | Saves current rules to `Game.ini` (`[/Script/GameSettings.ServerRules]`). |

### Examples
```text
wo.rules.set ResourceGatherMultiplier 2.0
wo.rules.set bUpkeepEnabled false
wo.rules.set bOfflineRaidProtection true
wo.rules.set DayNightSpeedMultiplier 0.5
wo.rules.save
```

---

## 2. Complete `DefaultGame.ini` / `Game.ini` Reference

Add or edit the `[/Script/GameSettings.ServerRules]` section in `Config/DefaultGame.ini` or `Saved/Config/WindowsServer/Game.ini`:

```ini
[/Script/GameSettings.ServerRules]
; --- Phase 1: Core Economy & Gathering ---
ResourceGatherMultiplier=1.000000
bEnforceToolTierGates=True

; --- Phase 1 & 2: Building, Upkeep & Decay ---
bTwigDecayEnabled=True
TwigDecayDamagePerMinute=2.500000
bUpkeepEnabled=True
UpkeepIntervalHours=24.000000
UpkeepCostMultiplier=1.000000
bBuildingDecayEnabled=True
bElectricityEnabled=True

; --- Phase 1–3: Combat, Workbenches & Raiding ---
bPvPEnabled=True
bOfflineRaidProtection=True
bWorkbenchTierGatingEnabled=True

; --- Phase 0, 3 & 4: Environment, Weather & AI ---
DayNightSpeedMultiplier=0.500000
TorchBurnTickSeconds=3.000000
bRadiationEnabled=True
bTemperatureEnabled=True
bStructuralWindDestructionEnabled=True
bAdvancedAIEnabled=True
```

---

## 3. Headless Blender 5.2 Art Pipeline for Modders

Wild Omission uses an untextured, flat-shaded low-poly art style backed by a shared 24-color palette ([`Art/Palette.json`](../../Art/Palette.json)).

### Generating or Modifying Meshes
All procedural low-poly generators live in `Art/Blender/` and export clean `1 UU = 1 cm` FBX files to `Art/Export/` via [`Art/Blender/export_fbx.py`](../../Art/Blender/export_fbx.py):
- **Phase 0/1:** [`generate_metal_ore_node.py`](../../Art/Blender/generate_metal_ore_node.py), [`generate_phase1_assets.py`](../../Art/Blender/generate_phase1_assets.py)
- **Phase 2:** [`generate_phase2_assets.py`](../../Art/Blender/generate_phase2_assets.py)
- **Phase 3:** [`generate_phase3_assets.py`](../../Art/Blender/generate_phase3_assets.py)
- **Phase 4:** [`generate_phase4_assets.py`](../../Art/Blender/generate_phase4_assets.py)

Run any generator headlessly from PowerShell:
```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b -P "Art\Blender\generate_phase4_assets.py"
```

---

## 4. Building & Running Automated Verification Tests

1. **Compile `WildOmissionEditor` (`Win64 Development`):**
   ```powershell
   .\Tools\Build.bat WildOmissionEditor Development
   ```
2. **Run All Phase 1–4 Automation Tests Headlessly (`EXIT CODE: 0`):**
   ```powershell
   .\Tools\RunAllTests.bat
   ```
