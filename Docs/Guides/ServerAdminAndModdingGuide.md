# Wild Omission: Rust-Style Admin Control Panel & Server Guide

Wild Omission features a comprehensive **Rust-Style Admin Control Panel** (accessible via the in-game **Admin / Item Spawner UI**, **`/chat` commands**, and **`~` console commands**) alongside the [`UServerRules`](../../Source/GameSettings/Public/ServerRules.h) live server configuration subsystem.

---

## 1. Opening the In-Game Admin Control Panel (UI)

1. Press **`C`** (or **`Tab`**) while in-game to open your Inventory & Secondary Menu.
2. In the top-right corner of the menu, click the **`Admin / Item Spawner`** button to switch between the **Crafting Menu** and the **Admin Control Panel** (`UCreativeMenuWidget`).
3. Inside the **Admin Control Panel**, click the top-right **`Commands` / `Items`** toggle button to switch between:
   - **Items Tab (Item Spawner & Inventory Manager):** Browse all items by category (`All`, `Items`, `Resources`, `Tools`, `Building`, `Weapons`, `Ammo`, `Medical`, `Clothing`), spawn 1, spawn a full stack, type **any custom quantity (`1–100,000`)** to spawn or delete, or clear your inventory.
   - **Commands Tab (Rust F1 Admin Actions):** One-click buttons for **Creative / Free-Build Mode**, **God Mode**, **Fly / Noclip**, **Full Heal**, **Gather Rate Multipliers (`1x`–`100x`)**, **`ent kill` (Delete Target Under Crosshair)**, **Base Repair (`50m`)**, **Base Upgrade to Armored (`50m`)**, **Time of Day**, **Weather**, and **Python Base Spawners**.

---

## 2. Admin Panel UI Controls Reference

### A. Items Tab (Spawner & Item Deletion)
| UI Control | Action |
| :--- | :--- |
| **Category Filter Bar (`All`, `Weapons`, `Ammo`, `Building`, etc.)** | Filters the item grid. Defaults to **`All`** so every item is immediately visible. |
| **Give Single (`x1`)** | Spawns `1` of the selected item into your inventory. |
| **Give Stack (`xStackSize`)** | Spawns a full stack of the selected item into your inventory. |
| **Custom Quantity Box (`1–100000`)** | Type any custom quantity (default `100`). |
| **Give Custom Quantity** | Spawns the exact custom quantity of the selected item (automatically splitting across slots if larger than stack size). |
| **Delete Custom Qty** | Removes the typed custom quantity of the selected item from your inventory. |
| **Delete All of Selected** | Removes every instance of the selected item from your inventory. |
| **Clear Entire Inventory** | Wipes all slots in your player inventory clean. |

### B. Commands Tab (Player, World, Gathering & Base Controls)
| UI Button | Action |
| :--- | :--- |
| **Toggle Creative / Free-Build** | Toggles `bCreativeFreeBuildEnabled` + Creative Mode + God Mode. Place unlimited building blocks/deployables without consuming items, upgrade/repair with a Hammer for 0 resources, craft any recipe without ingredients or workbenches, and bypass Tool Cupboard build restrictions. |
| **Toggle Fly / Noclip** | Toggles flying movement (`Space` to ascend, `Ctrl` to descend). Also bound to the **`V`** key. |
| **Full Heal & Cure Vitals** | Restores `100` Health, `300` Thirst, `500` Hunger, cures all Bleeding and Radiation, and resets Body Temperature to `37.0°C`. |
| **Enable / Disable God Mode (Self / All)** | Enables or disables complete invincibility and infinite vitals. |
| **Gather Rate: `1x` / `2x` / `5x` / `10x` / `50x` / `100x`** | Immediately scales resource yields from trees, ore nodes, animals, and ground pickups across the server. |
| **Delete Target (`ent kill`)** | Destroys any building block, deployable, resource node, dropped item, or NPC under your crosshair (up to `100m`). |
| **Repair Base (`50m`)** | Restores `100%` durability to all structures and deployables within `50` meters. |
| **Upgrade Base Armored (`50m`)** | Instantly upgrades all building blocks within `50` meters to **Tier 4 (Armored)**. |
| **Clear Dropped World Items** | Deletes all dropped ground item bags (`AWorldItem`) in the world to remove clutter. |
| **Give Full Combat & Build Kit** | Spawns an Assault Rifle, Combat Shotgun, Bolt-Action Rifle, ammo, C4, Storage Barrel, Solar Panel, Lights, Armored building pieces, Medical Syringes, and Hazmat Suit. |
| **Spawn Base: Starter 2x2 / Clan Fortress / Industrial Compound** | Spawns the corresponding Python-generated base blueprint directly in front of you, snapped to terrain height. |
| **Time of Day (`Morning`, `Noon`, `Evening`, `Midnight`, `Freeze` / `Unfreeze`)** | Sets or freezes the solar time of day immediately. |
| **Weather (`Clear`, `Rain`, `Tornado`, `Spawn Storm`)** | Changes active weather or spawns a storm cell. |

---

## 3. Complete In-Game `/chat` & `~` Console Command Reference

Press **`Enter`** to type any `/chat` command, or press **`~`** (tilde) to run any console command:

### Item Spawning & Inventory Management
| Chat Command | Console Command (`~`) | Description |
| :--- | :--- | :--- |
| `/give <ItemID> [Qty]` | `AdminGive <ItemID> <Qty>` or `GiveItem <ItemID> <Qty>` | Spawns `<Qty>` of `<ItemID>` in your inventory (e.g. `/give AssaultRifle 1`, `/give Wood 5000`). |
| `/kit` or `/giveall` | `GiveStarterKit` | Spawns a full combat, raiding, electrical, lighting, and armored building kit. |
| `/delete <ItemID> [Qty]` | `AdminDeleteItem <ItemID> <Qty>` | Deletes `<Qty>` of `<ItemID>` from your inventory (omit `[Qty]` to delete all of that item). |
| `/clearinv` or `/clear` | `AdminClearInventory` | Deletes all items from your inventory. |
| `/clearground` | `AdminClearWorldItems` | Deletes all dropped items (`AWorldItem`) on the ground across the map. |

### Creative Mode, God Mode, Movement & Vitals
| Chat Command | Console Command (`~`) | Description |
| :--- | :--- | :--- |
| `/creative` or `/freebuild` | `AdminToggleCreativeMode` | Toggles **Creative / Free-Build Mode** (infinite building placement, free Hammer upgrades/repairs, free crafting without ingredients/workbenches, and God Mode). |
| `/god` or `/godmode` | `AdminToggleGodMode` | Toggles **God Mode** (invincibility, no hunger/thirst drain, no radiation or bleeding). |
| `/fly` or `/noclip` | `AdminToggleFly` | Toggles **Fly Mode** (`Space` = up, `Left Ctrl` = down). Also toggleable with **`V`**. |
| `/speed <Multiplier>` | `AdminSetSpeed <Multiplier>` | Sets walking, sprinting, swimming, and flying speed multiplier (`0.25` to `20.0`, e.g. `/speed 3`). |
| `/heal` or `/cure` | `AdminHeal` | Restores `100%` Health, Thirst, Hunger, and cures all Bleeding and Radiation. |
| `/tp` | `AdminTeleportToLook` | Teleports you to the surface under your crosshair (up to `500m`). |
| `/tp <X> <Y> <Z>` | `AdminTeleportXYZ <X> <Y> <Z>` | Teleports you to exact world coordinates `(X, Y, Z)`. |

### Gathering Multiplier & Server Rules
| Chat Command | Console Command (`~`) | Description |
| :--- | :--- | :--- |
| `/gather <Multiplier>` | `AdminSetGatherRate <Multiplier>` or `gather.rate <Multiplier>` | Sets `ResourceGatherMultiplier` immediately (e.g. `/gather 10` for `10x` yields on all trees, ore nodes, and ground pickups). |
| — | `wo.rules.list` | Lists all active `UServerRules` settings and values in the console/log. |
| — | `wo.rules.set <Key> <Value>` | Sets any `UServerRules` property at runtime (e.g. `wo.rules.set bUpkeepEnabled false`). |
| — | `wo.rules.reset` | Resets all `UServerRules` properties to default values. |
| — | `wo.rules.save` | Saves current `UServerRules` settings to `Game.ini`. |

### Structure Management & Python Base Builder
| Chat Command | Console Command (`~`) | Description |
| :--- | :--- | :--- |
| `/entkill` or `/destroy` | `AdminEntKill` | Destroys the building block, deployable, resource node, or NPC under your crosshair (`100m` range). |
| `/repairbase [RadiusCm]` | `AdminRepairBase [RadiusCm]` | Repairs all structures within `RadiusCm` (default `5000` = `50m`) to `100%` health. |
| `/upgradebase [Tier 1-4] [RadiusCm]` | `AdminUpgradeBase <Tier> <RadiusCm>` | Upgrades all building blocks within radius to `1` (Wood), `2` (Stone), `3` (Sheet Metal), or `4` (Armored). |
| `/buildbase <Blueprint>` | `BuildBase <Blueprint>` | Spawns a Python-generated JSON base blueprint (`starter_2x2`, `honeycomb_clan_fortress`, `industrial_compound`, or custom). |
| `/exportbase <Name> [RadiusCm]` | `ExportBase <Name> <RadiusCm>` | Exports all structures within `RadiusCm` of you to `Saved/BaseBlueprints/<Name>.json`. |

### Time of Day & Weather
| Chat Command | Console Command (`~`) | Description |
| :--- | :--- | :--- |
| `/time <Hour 0-24>` | `AdminSetTime <Hour 0-24>` | Sets time of day in hours (`6` = Morning, `12` = Noon, `18` = Evening, `0` = Midnight). |
| `/weather <clear\|rain\|tornado>` | `AdminSetWeather <Type>` | Sets active weather to `Clear`, `Rain`, or `Tornado`. |

---

## 4. Complete `DefaultGame.ini` / `Game.ini` Reference

Add or edit the `[/Script/GameSettings.ServerRules]` section in `Config/DefaultGame.ini` or `Saved/Config/WindowsServer/Game.ini`:

```ini
[/Script/GameSettings.ServerRules]
; --- Phase 1: Core Economy, Gathering & Creative Free-Build ---
ResourceGatherMultiplier=1.000000
bEnforceToolTierGates=True
bCreativeFreeBuildEnabled=False

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
