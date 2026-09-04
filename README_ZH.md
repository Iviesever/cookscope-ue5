# CookScope

CookScope 是一个 source-only 的 Unreal Engine 5.8 Editor 插件、Commandlet 与最小 Sample Project，用于确定性的 Asset Registry 依赖分析和真实 Cook 预算审计。

> 已发布 source-only [`v0.1.0`](https://github.com/Iviesever/cookscope-ue5/releases/tag/v0.1.0)，[PR #1](https://github.com/Iviesever/cookscope-ue5/pull/1) 采用普通 merge commit 合并。Core、UE Automation、Commandlet、真实 Cook diff、报告、Editor UI 与插件打包均在本机验证；GitHub Release 没有自定义附件。

| 真实 UE 5.8 Editor Tab | 真实离线 HTML 报告 |
|---|---|
| ![CookScope Editor](docs/images/editor.png) | ![CookScope HTML](docs/images/report.png) |

最短完整审计命令：

```powershell
UnrealEditor-Cmd.exe SampleProject/CookScopeSample.uproject -run=CookScopeAudit -config=Plugins/CookScope/Config/CookScopeRules.json -output=Artifacts/Reports -source-sha=<40位SHA>
```

Sample 中的 Primary Asset 可以解释为 `DA_Primary → DA_Target` 的 Soft/Manage 路径。受控 baseline `66256ac` 与 candidate `f204b3b` 之间只新增 `DA_Candidate`，真实 Development Asset Registry 测得其 Cook 大小为 **892 字节**。

主要入口：

```powershell
pwsh -File scripts/Test.ps1
pwsh -File scripts/Test-Unreal.ps1
pwsh -File scripts/Cook.ps1
pwsh -File scripts/Build-Plugin.ps1
```

项目不会把 package/source 大小冒充 actual Cook 大小；缺失数据保持 `unavailable`。Git 中不包含 Binaries、Intermediate、Saved、Cook 输出、打包插件或日志。AI 参与了实现、调试和文档编写，所有行为声明均以可执行测试或真实 UE/浏览器证据为准。完整说明见 [英文 README](README.md) 与 [已知限制](docs/KNOWN_LIMITATIONS.md)。
