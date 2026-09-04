# Build System

MQB 5.4 builds and tests the portable C++20 Core/CLI surface. `mqb.json` excludes UE adapters, sample sources, generated outputs, and artifacts; discovery currently sees the CLI plus nine Core translation units.

Unreal Build Tool is authoritative for module boundaries, reflection, Unity builds, DLL exports, and Editor loading. `scripts/Build-Unreal.ps1` targets `CookScopeSampleEditor Win64 Development`. `scripts/Cook.ps1` uses RunUAT, and `scripts/Build-Plugin.ps1` uses `BuildPlugin` for Editor plus Win64 Development/Shipping Game targets.

Only `CookScopeCore` is a Runtime module. `CookScopeEditor`, `CookScopeCommandlet`, and `CookScopeTests` are Editor modules and are absent from Game targets.

UBA may fail to bind local port 1345 on this host; UBT then uses its local executor successfully. Scripts use `-WaitMutex` and do not terminate unrelated UE builds.
