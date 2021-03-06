using System.Collections.Generic;
using GoddamnEngine.BuildSystem;

public sealed class RuntimeProject : Project
{
    public override string Name { get { return "GoddamnRuntime"; } }
    public override bool IsPlugin { get { return false; } }
    public override ProjectBuildType BuildType { get { return ProjectBuildType.Application; } }
    protected override void InitializeSelf()
    {
        base.InitializeSelf();
        this.Dependencies.Add(CoreProjectDependency.Instance);
        this.Dependencies.Add(EngineProjectDependency.Instance);
    }
}
