# CookScope UE5.8 完整交付

创建、实现、验证、审计并正式发布一个新的公开作品集仓库：

- GitHub：`Iviesever/cookscope-ue5`
- 本地目录：在 `D:\program` 下选择不会覆盖任何现有目录的位置
- 引擎：本机已安装的 Unreal Engine 5.8
- 主要语言：C++
- 项目形式：UE 5.8 Editor Plugin + Commandlet + Sample Project
- 目标版本：`v0.1.0`
- 最终期限：2026 年 9 月 5 日 15:30（UTC+8）

这是一项完整交付任务，不是只创建规则列表、UI 原型或 README。必须持续推进到真实 Editor 加载、真实 Asset Registry 扫描、真实 Cook、真实报告、自动化验证、独立审计和 source-only Release。

若真实验证无法满足全部 P0，必须保留准确的 Alpha/WIP 状态，不得通过生成大量文档伪装为完成。

---

## 0. 开始前恢复真实状态

开始任何写入前：

1. 检查 GitHub 登录身份，必须是 `Iviesever`。
2. 检查目标 GitHub 仓库是否存在：
   - 不存在则创建公开仓库；
   - 已存在则读取默认分支、HEAD、PR、Release、Actions 和真实文件状态。
3. 检查目标本地目录：
   - 不存在则创建；
   - 存在则读取真实 Git 状态，不得删除、覆盖或重置未知文件。
4. 检查本机：
   - UE 5.8；
   - MSVC；
   - Windows SDK；
   - PowerShell 7；
   - MQB；
   - Python；
   - Git；
   - 当前 UE、UBT、RunUAT、Cook、ShaderCompileWorker 和测试进程。
5. 不得结束与本项目无关的进程。
6. 同一仓库只能有一个写入 Agent；独立审计 Agent 只能只读。
7. 创建并遵守：
   - `AGENTS.md`
   - `.agents/AGENTS.md`
   - `docs/PRODUCT_CONTRACT.md`
   - `docs/ARCHITECTURE.md`
   - `docs/RULE_MODEL.md`
   - `docs/ACCEPTANCE_MATRIX.md`
   - `tasks/<timestamp>-cookscope-0.1/goal-objective.md`
   - `tasks/<timestamp>-cookscope-0.1/progress.md`
   - `tasks/<timestamp>-cookscope-0.1/handoff.md`
8. 保存本 `/goal` 原文，不得在后续工作中静默降低验收标准。

---

## 1. 最高产品目标

构建一个面向真实 UE 内容生产和 CI 的资产依赖与 Cook 审计工具：
```text
Asset Registry
→ Asset Manager / Primary Assets
→ Dependency Graph
→ Configurable Validation Rules
→ Real Cook Snapshot
→ Baseline / Candidate Diff
→ Size and Dependency Budgets
→ JSON / SARIF / JUnit / HTML
→ Editor Slate UI
→ Commandlet / CI Exit Code
→ Reproducible Evidence
```

项目名称：

> **CookScope — UE 5.8 Asset Dependency and Cook Budget Auditor**

它必须能够回答以下实际问题：

- 为什么某个资产会进入最终 Cook？
- 哪条 Hard/Soft/Manage 引用路径把它带进来了？
- 哪个提交使包体增加？
- 哪些资产跨越了禁止的目录或模块边界？
- 哪些 Primary Asset、Bundle 或 Chunk 配置错误？
- 哪些纹理、网格、音频超过预算？
- 哪些依赖形成环？
- 哪些意外资产被 Cook？
- 哪些问题应该阻断 CI？
- 两次 Cook 或资产快照之间到底发生了什么变化？

它不是简单包装一个已有 UE 控制台命令，也不是重新实现 Cooker、UnrealPak 或 Asset Registry。

---

## 2. 不可违反的 MQB 构建政策

### 2.1 MQB 优先

本项目必须优先尝试使用本机 `mqb`。

尽早执行一次有边界的能力验证，确认 MQB 能否构建：

1. 独立纯 C++ 规则引擎；
2. 确定性差异和报告生成核心；
3. 命令行辅助工具；
4. UE Plugin 模块；
5. UE Sample Project；
6. 含 UHT 生成代码的目标；
7. Editor/Commandlet 所需模块。

### 2.2 MQB 成功时

只要 MQB 对某个目标构建成功、结果正确并可重复：

- 必须优先使用 MQB；
- 不得优先使用 Clang、GCC、普通 CMake/Ninja、手工 `cl.exe` 或其他替代工具；
- 默认 `scripts/Build.ps1` 应先进入 MQB 路径；
- README 应把 MQB 作为该目标的首选本地构建入口；
- 必须验证：
  - clean build；
  - incremental/no-op build；
  - 失败退出码；
  - 产物路径；
  - 产物身份；
  - 与后续测试使用的二进制一致。

### 2.3 UE 官方工具边界

以下 UE 专用任务若必须使用 UBT/RunUAT，可以回退：

- UHT；
- Editor Target；
- Commandlet Target；
- BuildPlugin；
- Cook；
- Stage；
- Pak；
- IoStore；
- BuildCookRun；
- UE Automation。

优先顺序：
```text
MQB 可以正确直接构建
→ 使用 MQB

MQB 可作为稳定入口编排 UE 官方构建工具
→ 使用 MQB 入口

MQB 经验证不支持该目标
→ 记录准确原因，回退到 UBT / RunUAT
```

不得无证据宣称 MQB 已经替代 UBT。

### 2.4 不使用 Clang 抢占主线

如果 MQB 和 MSVC 主线已经能完成构建：

- 不得为了形式上的“多编译器矩阵”优先投入 Clang；
- 不得让 Clang、GCC 或其他编译器延误 UE Plugin、Commandlet、Cook 和报告闭环；
- Clang 只允许在所有 P0 已完成后，对确实独立于 UE 的纯 C++ Core 进行可选附加验证；
- 即便增加 Clang 验证，MQB 仍是首选路径。

若 MQB 对完整 UE 目标失败，必须保存精确失败证据，但继续使用 MQB 构建它能成功承担的纯 C++ 部分。

---

## 3. 不可违反的 Release 发布政策

必须执行真实本地：

- BuildPlugin；
- Editor 加载；
- Commandlet；
- UE Automation；
- Asset Registry 扫描；
- Data Validation；
- Cook；
- Sample Project 验证；
- clean extraction 或 clean checkout smoke。

但是：

> **不得将任何打包作品或工具包上传到 GitHub Release。**

Release 禁止上传：

- Plugin ZIP；
- Sample Project ZIP；
- Win64 工具；
- `.exe`、`.dll`、`.pdb`；
- Cooked Content；
- Pak/IoStore 文件；
- 报告压缩包；
- 手工 Source ZIP；
- 大型测试日志；
- Trace；
- 大型资产快照；
- 其他自定义 Release Asset。

最终 Release 只保留 GitHub 自动生成的：

- `Source code (zip)`
- `Source code (tar.gz)`

Release `assets` 必须为空。

本地 Plugin Package、Sample Package 和报告包仍需生成并验证，但保存在被 `.gitignore` 排除的 `Artifacts/` 或 `artifacts/` 目录中。PR 和 Release Notes 只记录：

- 对应 Git SHA；
- 本地产物路径；
- 文件大小；
- SHA-256；
- 验证命令；
- 运行结果。

仓库可提交少量真实且体积可控的：

- Sample JSON；
- Sample SARIF；
- Sample JUnit；
- Self-contained HTML 示例；
- Editor 截图；
- 依赖图截图；
- 小型违规 fixture。

不得提交真实项目的大型 Cook 输出。

---

## 4. 明确架构

优先采用清晰分层：
```text
CookScope Model / Rule Core
    ↓
Asset Snapshot and Dependency Model
    ↓
Rule Engine
    ↓
Cook Snapshot / Diff Engine
    ↓
Report Writers
    ↓
UE Adapter
    ├─ Asset Registry
    ├─ Asset Manager
    ├─ Data Validation
    ├─ Commandlet
    └─ Editor Slate UI
```

建议模块或等价职责：
```text
CookScopeCore
CookScopeEditor
CookScopeCommandlet
CookScopeTests
CookScopeSample
```

### Core 原则

- 规则、快照、差异和报告格式尽可能使用确定性数据结构；
- 可以独立于 Editor UI 测试的逻辑不得埋进 Slate Widget；
- 不得让报告生成逻辑依赖当前 UI 状态；
- 不得让 Commandlet 和 Editor 各自实现一套不同规则；
- 同一份规则和快照模型必须被：
  - Editor；
  - Commandlet；
  - CI；
  - HTML 报告；
  - 测试
    共同使用。

### UE 边界

- Editor-only API 不得泄露进 Runtime；
- 非 Editor 构建不得意外依赖 UnrealEd；
- Commandlet 必须能在无交互模式运行；
- Sample Project 必须可在 clean checkout 中复现；
- 不得修改 UE 安装目录。

---

## 5. 明确非目标

禁止：

- 重新实现 Cooker；
- 重新实现 UnrealPak；
- 重新实现 Asset Registry；
- 重新实现 DDC；
- 自制资源格式；
- 大型资产管理平台；
- 云端服务；
- 数据库服务器；
- Web 后端；
- 用户登录；
- 商业付费功能；
- 自动删除用户资产；
- 自动修改真实项目资产；
- 大规模第三方美术资源；
- 与作品集无关的框架扩展；
- 创建额外技术仓库。

本项目默认只读分析。任何“修复”功能只能生成建议，不得未经明确命令直接修改资产或配置。

---

## 6. PACT-00：仓库、插件和最小闭环

完成：

1. 创建公开仓库。
2. 建立 UE 5.8 Plugin。
3. 建立极小 Sample Project。
4. 插件可被 Editor 加载。
5. 创建一个最小 Editor Tab。
6. 创建一个最小 Commandlet。
7. Commandlet 输出结构化 JSON。
8. Commandlet 成功和失败有稳定退出码。
9. 建立构建脚本。
10. 执行 MQB 能力验证。
11. 建立真实 `main` 基线。
12. 从真实 `origin/main` 创建：
    - `feat/cookscope-0.1`
13. 创建 Draft PR。
14. 保存真实构建和加载证据。

PACT-00 未通过前，不得先制作复杂 UI。

---

## 7. PACT-10：版本化快照与规则模型

实现版本化、确定性的核心数据模型。

### Asset Snapshot 至少包含

- Object Path；
- Package Name；
- Asset Class；
- Package Path；
- Primary Asset ID，若存在；
- Disk Size；
- Cooked Size，若可获得；
- Chunk ID；
- Asset Bundle；
- Tags/Metadata；
- Hard Dependencies；
- Soft Dependencies；
- Manage Dependencies；
- Searchable Name Dependencies；
- Source provenance；
- Snapshot schema version。

### Rule Model 至少包含

- 稳定 Rule ID；
- 名称；
- 描述；
- Severity；
- 适用范围；
- Include/Exclude；
- 参数；
- 预算；
- 例外；
- Baseline 行为；
- Fail threshold；
- 文档链接。

规则配置必须：

- 版本化；
- 严格解析；
- 未知字段失败关闭或明确警告；
- 拒绝重复 Rule ID；
- 拒绝非法阈值；
- 有稳定 canonical JSON；
- 相同输入产生字节一致输出或明确说明不可避免字段。

---

## 8. PACT-20：Asset Registry 与依赖解释

使用 UE Asset Registry 和 Asset Manager 官方接口建立依赖图。

必须区分：

- Hard；
- Soft；
- Manage；
- Searchable Name。

不得把所有引用混成一个无类型边。

必须实现：

1. 指定资产的直接依赖；
2. 反向引用者；
3. 完整依赖路径；
4. “为什么被 Cook”解释链；
5. 最短路径；
6. 可选全部路径，必须有边界；
7. 循环依赖检测；
8. 跨目录依赖；
9. 跨插件/模块依赖；
10. Primary Asset 管理关系；
11. Asset Bundle 关系；
12. Chunk 归属；
13. 未解析或缺失依赖；
14. 最大节点/边/深度限制；
15. 图过大时失败关闭或输出截断说明。

依赖遍历必须稳定，排序规则必须明确，不能依赖哈希容器随机顺序。

---

## 9. PACT-30：规则引擎与 Data Validation

至少实现以下 P0 规则：

### 命名与路径

- 资产命名规则；
- 类型前缀；
- 禁止目录；
- 临时目录资产；
- Editor-only 资产泄露；
- 重复或模糊命名。

### 依赖边界

- 禁止依赖；
- 跨层依赖；
- 跨模块/插件依赖；
- 循环依赖；
- Runtime 内容依赖 Editor 内容；
- 软引用被意外替换成硬引用；
- 超过最大依赖深度或扇出。

### 资源预算

按可获得的真实 UE 元数据检查：

- Texture 尺寸、Mip、格式或预算；
- Static/Skeletal Mesh 规模或预算；
- SoundWave 长度、格式或预算；
- 单资产磁盘大小；
- 单资产 Cook 大小；
- 目录预算；
- 类型预算；
- 项目总预算。

必须诚实区分：

- Source disk size；
- Package disk size；
- Estimated size；
- Actual cooked size；
- 不可获得。

不得把估算值写成实际 Cook 值。

### Asset Manager / Cook

- Primary Asset 配置缺失；
- Primary Asset 类型冲突；
- Bundle 错误；
- Chunk 冲突；
- 多 Chunk 重复；
- 意外 Cook；
- 应 Cook 未 Cook；
- NeverCook/AlwaysCook 规则冲突；
- Editor-only 泄露；
- 重定向器；
- 缺失引用。

必须与 UE Data Validation 集成，使 Editor Validate Assets 能复用同一规则。

---

## 10. PACT-40：真实 Cook 快照和差异

必须真正执行 Sample Project Cook，不得只读取源资产大小。

实现两个版本化快照之间的 deterministic diff：
```text
Baseline Snapshot
        vs
Candidate Snapshot
```

至少报告：

- 新增资产；
- 删除资产；
- 修改资产；
- 新增依赖；
- 删除依赖；
- 引用类型变化；
- 新增 Cook 大小；
- 减少 Cook 大小；
- 目录预算变化；
- 类型预算变化；
- Chunk 变化；
- Bundle 变化；
- Primary Asset 变化；
- 新增违规；
- 已修复违规；
- Severity 变化；
- Baseline 中已有但未恶化的问题。

必须明确处理：

- 相同资产重命名；
- Redirector；
- 数据缺失；
- 不同引擎版本；
- 不同平台；
- 不同 Cook 配置；
- 不可比较快照；
- Schema 迁移；
- 路径大小写；
- Unicode；
- 稳定排序。

Sample Project 中必须包含可控变更，用自动测试证明 diff 正确。

---

## 11. PACT-50：报告输出

必须从同一份 canonical result 生成：

### JSON

- 版本化 Schema；
- 稳定排序；
- 完整机器可读数据；
- 配置、环境和源 SHA provenance；
- 结果汇总；
- 每条 finding；
- 依赖路径；
- 预算变化；
- 截断和限制。

### SARIF

- 合法 SARIF；
- 稳定 Rule ID；
- 文件/资产定位；
- Severity 映射；
- 帮助文本；
- 可用于代码扫描或 CI 展示。

### JUnit XML

- 每条规则或规则组映射为稳定 testcase；
- 失败、跳过、错误语义明确；
- 可被常见 CI 读取。

### Self-contained HTML

必须：

- 无 CDN；
- 无外部字体；
- 无外部 JS/CSS；
- 可离线打开；
- 支持：
  - 总览；
  - Severity 筛选；
  - Rule 筛选；
  - Asset 类型筛选；
  - 路径搜索；
  - Baseline/Candidate 对比；
  - 依赖链展开；
  - 大小变化排序；
  - Chunk/Bundle 查看；
  - 响应式窄屏；
- Console Error/Warning 为 0；
- 嵌入的数据必须与 JSON 语义一致；
- 提供桌面与窄屏真实浏览器验证。

---

## 12. PACT-60：Editor Slate 工具

提供真正可用的 Editor Tab。

至少支持：

1. 选择项目或扫描范围；
2. 加载规则配置；
3. 执行扫描；
4. 查看进度；
5. 按 Severity、Rule、Class、Path 筛选；
6. 搜索资产；
7. 展开 finding；
8. 展开完整依赖链；
9. 显示“为什么被 Cook”；
10. 对比 Baseline/Candidate；
11. 显示大小变化；
12. 显示 Chunk/Bundle；
13. 定位到 Content Browser；
14. 打开资产；
15. 导出 JSON/SARIF/JUnit/HTML；
16. 不阻塞 Editor 主线程，或对长任务提供可取消/有边界执行；
17. 扫描结束后无悬挂异步任务；
18. Editor 关闭时正确清理。

UI 不是独立实现规则的地方。所有结果必须来自共享核心。

---

## 13. PACT-70：Commandlet、CI 和完整测试

提供命令，例如：
```text
UnrealEditor-Cmd.exe Sample.uproject
-run=CookScopeAudit
-config=<path>
-baseline=<path>
-output=<path>
```

具体参数可调整，但必须：

- 文档完整；
- 参数严格解析；
- 未知参数失败；
- 输出目录明确；
- 稳定退出码；
- 发生违规时可配置是否阻断；
- 发生内部错误时与普通违规使用不同退出码；
- 超时失败关闭；
- 日志中不泄露无关本地隐私路径，或提供归一化策略。

### 必须测试

- 配置解析；
- Canonical JSON；
- Graph traversal；
- Cycle；
- Typed dependency；
- Rule matching；
- Include/Exclude；
- Budget；
- Baseline suppression；
- Snapshot diff；
- Rename/redirector；
- SARIF；
- JUnit；
- HTML；
- Unicode；
- 路径大小写；
- 边界节点/边/深度；
- 无资产；
- 损坏快照；
- Schema mismatch；
- Commandlet success；
- Commandlet violation；
- Commandlet internal error；
- Data Validation；
- Editor Tab；
- Content Browser 定位；
- 取消扫描；
- Plugin unload；
- 真实 Cook；
- clean checkout。

### UE 验证

- Editor Development；
- Game Development/Shipping，仅用于证明 Runtime 边界未污染，若插件本身是 Editor-only则应正确拒绝或排除；
- BuildPlugin；
- UE Automation；
- Commandlet E2E；
- Sample Project Cook；
- Baseline/Candidate diff；
- 本地插件打包；
- fresh extracted Plugin smoke；
- 源 SHA 与产物绑定。

本地插件包和 Sample Project 包不得上传 Release。

---

## 14. Sample Project 设计

Sample Project 必须极小、可自动生成或完全提交源配置，不依赖大型第三方资产。

必须包含受控 fixture，例如：

- 一个合法纹理；
- 一个超预算纹理；
- 一个合法目录依赖；
- 一个禁止跨目录依赖；
- 一个 Hard 引用；
- 一个 Soft 引用；
- 一个 Manage 引用；
- 一个 Searchable Name 引用，若可稳定建立；
- 一个循环依赖；
- 一个 Primary Asset；
- 一个缺失或错误 Bundle；
- 一个 Chunk 冲突；
- 一个意外 Cook；
- 一个命名违规；
- 一个已有 baseline 违规；
- 一个候选新增违规；
- 一个已修复违规。

每个 fixture 必须有稳定 ID 和对应测试，不得靠人工猜测结果。

---

## 15. P1 条件功能

只有全部 P0、完整 Cook、Plugin Build、Commandlet、Editor UI、报告、clean-source、独立审计均通过后，才允许从以下选择最多一项：

- 读取 Asset Loading Insights Trace；
- 分析同步/异步加载顺序；
- 加载优先级异常；
- Memory Insights 预算；
- Chunk/Bundle 交互式图；
- 生成 GitHub PR Markdown 摘要；
- 与 MQB 的构建产物/依赖报告进行实验性关联。

不得因 P1 破坏 P0，不得在 P0 未通过时先做高级 Insights 集成。

---

## 16. 作品集和面试文档

必须提供：

- `README.md`
- `README_ZH.md`，若时间允许
- `docs/ARCHITECTURE.md`
- `docs/ASSET_REGISTRY_MODEL.md`
- `docs/ASSET_MANAGER_AND_PRIMARY_ASSETS.md`
- `docs/COOK_PIPELINE.md`
- `docs/RULE_MODEL.md`
- `docs/SNAPSHOT_SCHEMA.md`
- `docs/DIFF_MODEL.md`
- `docs/REPORT_FORMATS.md`
- `docs/EDITOR_TOOLING.md`
- `docs/COMMANDLET_AND_CI.md`
- `docs/BUILD_SYSTEM.md`
- `docs/TESTING.md`
- `docs/KNOWN_LIMITATIONS.md`
- `docs/AI_ASSISTANCE.md`
- `docs/CODE_WALKTHROUGH.md`
- `docs/INTERVIEW_GUIDE.md`
- `docs/LIVE_CHANGE_DRILLS.md`
- `docs/RELEASE_NOTES_0.1.0.md`

README 首屏必须包含：

- 一张真实 Editor 截图；
- 一张真实 HTML 报告截图；
- 一条最短 Commandlet 命令；
- 一个示例依赖解释；
- 一个示例 Cook 大小差异；
- 验证结果摘要；
- 项目边界；
- source-only Release 说明；
- AI assistance 说明。

Interview Guide 至少覆盖：

- Asset Registry；
- Asset Manager；
- Primary/Secondary Asset；
- Hard/Soft/Manage 引用；
- Cook；
- Pak/IoStore 基础；
- Chunk；
- Bundle；
- Data Validation；
- Commandlet；
- Editor/Runtime 模块隔离；
- Slate；
- 异步扫描；
- deterministic diff；
- SARIF/JUnit；
- CI 阻断；
- 估算大小与实际 Cook 大小的区别。

Live Change Drills 至少包含：

1. 新增一条资产命名规则；
2. 新增一个目录预算；
3. 修改依赖边界并增加测试；
4. 新增一个报告字段并完成 Schema 迁移。

---

## 17. Git 与 GitHub 工作流

1. 建立最小可验证基线并推送 `main`。
2. 从真实 `origin/main` 创建：
   - `feat/cookscope-0.1`
3. 创建 Draft PR。
4. 每个 PACT 必须：
   - 先建立失败 fixture 或 RED 测试；
   - 实现；
   - 编译；
   - 测试；
   - 真实 Editor/Commandlet/Cook 运行；
   - 保存证据；
   - 更新 progress；
   - 提交；
   - 推送。
5. 不得将“代码存在”当作“功能通过”。
6. 所有 README 数字必须来源于真实日志或报告。
7. 失败尝试必须保留在任务进度中。
8. 最终执行一次新的独立只读全仓审计。
9. 只修复确认问题。
10. 修复后完整重新执行：
    - 构建；
    - 测试；
    - Editor；
    - Commandlet；
    - Cook；
    - Diff；
    - 报告；
    - Plugin Package；
    - fresh extraction；
    - 文档检查。
11. 若全部 P0 通过：
    - PR 标记 Ready；
    - 使用正常 merge commit 合并；
    - 删除功能分支；
    - 创建 annotated tag `v0.1.0`；
    - 创建 GitHub Release。
12. Release 不上传任何自定义资产，只使用 GitHub 默认源代码归档。

---

## 18. AI 辅助与作者身份

必须明确说明：

- 用户定义职业方向、项目方向、约束、期限、发布策略和验收目标；
- Codex GPT-5.6 Sol 完成架构细化、代码、测试、调试、Cook、报告、审计和文档；
- 用户未独立手写本次交付；
- 不得把项目描述成完全手写；
- 用户应在面试前理解 Asset Registry、Cook、规则引擎和报告链路，并完成至少一次 Live Change Drill。

不得删除 AI assistance 文档以掩盖开发方式。

---

## 19. 最终完成条件

只有同时满足以下条件，才能宣布 `CookScope v0.1.0` 完成：

- UE 5.8 插件可加载；
- Editor Tab 可用；
- Commandlet 可用；
- Asset Registry 类型化依赖；
- “为什么被 Cook”路径；
- 规则引擎；
- Data Validation；
- 真实 Cook；
- Baseline/Candidate Diff；
- JSON；
- SARIF；
- JUnit；
- Self-contained HTML；
- Sample Project 正负 fixture；
- 稳定退出码；
- CI/本地阻断逻辑；
- UE Automation；
- BuildPlugin；
- 本地 Plugin Package；
- fresh extraction smoke；
- clean-source 重验；
- 文档与事实一致；
- 独立审计无遗留 Blocker/High；
- PR 合并；
- annotated tag；
- source-only GitHub Release；
- Release 无任何自定义附件。

会话中的消息提示使用简体中文。

持续自主工作，最终需要满足这些条件。不要在只完成规划、只完成一个任务、只在 Debug 中运行或只创建 Draft PR 时提前结束。