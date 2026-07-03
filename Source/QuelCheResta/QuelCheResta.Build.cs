using UnrealBuildTool;

public class QuelCheResta : ModuleRules
{
	public QuelCheResta(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore",
			"AIModule", "GameplayTasks", "NavigationSystem"
		});
	}
}
