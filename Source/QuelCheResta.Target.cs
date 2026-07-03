using UnrealBuildTool;
using System.Collections.Generic;

public class QuelCheRestaTarget : TargetRules
{
	public QuelCheRestaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("QuelCheResta");
	}
}
