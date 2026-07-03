using UnrealBuildTool;
using System.Collections.Generic;

public class QuelCheRestaEditorTarget : TargetRules
{
	public QuelCheRestaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("QuelCheResta");
	}
}
