// Copyright Telephone Studios. All Rights Reserved.

using UnrealBuildTool;

public class Crafting: ModuleRules
{
    public Crafting(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Engine", "UMG", "Inventory" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "CustomUI", "GameSettings" });
    }
}