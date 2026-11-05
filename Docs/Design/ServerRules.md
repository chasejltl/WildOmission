# Server Rules & Admin Toggle Subsystem (`UServerRules`)

## Overview
All Rust-inspired survival mechanics in Wild Omission are controlled via `UServerRules` (`Source/GameSettings/Public/ServerRules.h`), a `UGameInstanceSubsystem` backed by `Config=Game` (`[/Script/GameSettings.ServerRules]` in `DefaultGame.ini` / `Game.ini`).

## Configurable Properties
| Property | Default | Description |
|---|---|---|
| `ResourceGatherMultiplier` | `1.0` | Global multiplier applied on top of tool-tier harvesting yields. |
| `bEnforceToolTierGates` | `true` | Requires appropriate minimum tool tiers (e.g. Metal Pickaxe for HQM). |
| `bTwigDecayEnabled` | `true` | Causes Twig-tier building blocks to slowly decay over time. |
| `TwigDecayDamagePerMinute` | `2.0` | Durability drained per minute on decaying Twig structures. |
| `bUpkeepEnabled` | `true` | Enables Tool Cupboard (TC) resource upkeep tax. |
| `UpkeepIntervalHours` | `24.0` | Period over which TC upkeep tax is consumed. |
| `UpkeepCostMultiplier` | `1.0` | Scales TC resource upkeep costs. |
| `bBuildingDecayEnabled` | `true` | Triggers decay on structures lacking TC upkeep resources. |
| `bPvPEnabled` | `true` | Enables player-versus-player damage. |
| `bOfflineRaidProtection` | `false` | Protects bases whose authorized owners are offline. |
| `bWorkbenchTierGatingEnabled` | `true` | Requires Workbench Tier 1/2/3 proximity for tiered crafting recipes. |
| `DayNightSpeedMultiplier` | `1.0` | Scales the 40-minute default day-night cycle speed. |
| `TorchBurnTickSeconds` | `3.0` | Seconds per 1 durability point drained while a torch is lit. |
| `bRadiationEnabled` | `true` | Enables radiation accumulation in monument Rad-Zones. |
| `bTemperatureEnabled` | `true` | Enables freezing/overheating body temperature mechanics. |
| `bStructuralWindDestructionEnabled` | `true` | Allows tornadoes and severe winds to damage unsecured Twig/Wood panels. |
| `bAdvancedAIEnabled` | `true` | Enables Scientist cover/flanking/grenade/healing behaviors. |

## Console & Admin Commands
- `wo.rules.list` — Prints all current server rules and their live values to the log/console.
- `wo.rules.set <RuleName> <Value>` — Updates any boolean or float rule live, persists it to `Game.ini`, and broadcasts `OnServerRuleChanged`.
