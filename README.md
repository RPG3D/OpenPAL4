# 仙剑奇侠传四程序重建

**简体中文** | [English](README.en.md)

本项目使用 C++20 重建《仙剑奇侠传四》的程序行为，并开发 Android、Switch
和 Linux 的可移植运行时。Windows 研究版本通过原版与重编译版的数值快照对照，
逐步验证剧情、脚本、角色、场景和战斗等模块。

项目仍在开发中，尚未完成全流程和全部平台的可玩性验收。源码研究使用 IDA
证据，不宣称已经过独立验证的净室开发。

项目自有代码采用 **GPL-3.0-or-later**：GNU 通用公共许可证第 3 版，或由使用者
选择任何后续版本。许可全文见 [LICENSE](LICENSE)，适用范围见
[LICENSING.md](LICENSING.md)。第三方代码保留各自的版权及许可条款，详见
[第三方许可清单](THIRD_PARTY_NOTICES.md)。

## 代码提交与合并要求

**新增或修复的代码必须以 PAL.exe 原始程序代码为唯一真源，并在与原版对比通过后才能合并。**
本项目当前分析样本的实际名称及版本以 [PAL4Source 来源清单](https://github.com/betesla/PAL4Source/blob/main/manifest.json)
中的文件名和 SHA-256 为准，不得仅凭文件名把不同版本、修改版或重编译版当成同一基线。

每项新增、修复或影响行为的重构，提交/PR必须包含：

1. **原版证据**：对应函数地址、调用关系、关键分支或数据布局；以原始EXE的反汇编和实际行为校验IDA伪代码，不凭命名、旧文档、其他游戏实现或推测补全逻辑。
2. **对照结果**：在相同资源、初始状态和同一份逻辑输入下，对比原版与修改版的输出、状态变化和实际副作用，并修复已发现的分叉。图形/UI改动需保留并排截图和两帧相减的RGB差异图；对无法同步的动画、时钟等注明具体范围，不能用局部一致声称全部通过。
3. **可复验记录**：原版样本与测试构建哈希、测试步骤/输入、预期与实际结果，以及必要的回归测试。仅编译成功、单元测试通过或检查刚写入的字段，不等于原版行为对照通过。
4. **完整接线与范围**：涉及脚本、状态拥有者、资源、渲染、UI、音频、存档等链路时，应验证真正使用这些数据的模块。禁止用空回调、固定返回值、孤立状态字段或绕过检查冒充实现；未完成或未通过原版对照的路径须明确记录，不作为已完成代码合并。同步更新相关文档，清理过时代码与说明。

### 原版分析材料

分析资料集中于公开仓库 **[betesla/PAL4Source](https://github.com/betesla/PAL4Source)**：

- [IDA数据库与EXE下载](https://github.com/betesla/PAL4Source/releases/tag/analysis-2026-11-04)：当前保存的 `launch.exe.i64`、与IDB输入哈希匹配的 `PAL4.exe`，以及IDB路径当前指向的磁盘版本 `launch.on-disk.exe`。两个EXE不可混用。
- [整库C伪代码](https://github.com/betesla/PAL4Source/blob/main/pseudocode/launch.c)：由当前IDA/Hex-Rays数据库批量导出，用于检索和辅助分析；不是恢复的原始C源码，也不保证可以编译。
- [文件哈希和导出记录](https://github.com/betesla/PAL4Source/blob/main/manifest.json)：记录数据库、EXE、伪代码的对应关系和来源差异。

原始EXE的行为高于反编译器推断；如伪代码与反汇编或原版实际执行不一致，应回到原程序核实。
分析材料及原游戏程序的权利归属不因上传而改变，不适用本项目自有代码的GPL授权。

## 当前进度

截至 2026-11-04：

| 部分 | 已有能力与验证边界 |
| --- | --- |
| Windows 原版对照 | 严格数值验收推进至 M15、剧情值 130201；后续切场景仍存在特效任务目标差异，尚未验收 |
| 共享运行时 | 原始资源包读取、场景与角色资源解析、脚本、图形及部分 GUI、输入和媒体服务已有实现 |
| Android / Switch / Linux | 已有平台适配与构建、运行记录；完整第一章和生产怪物、战斗流程尚未完成验收 |
| 独立主机构建 | 无原版资源、无专有 SDK 的源码快照已完成 VS2022 x64 Debug 构建，121/121 项 CTest 通过 |

上述主机测试中的两项可选原版资源子分支明确跳过，已通过的是对应的合成数据
合同测试。组件测试、独立 GUI 探针及设备构建记录不能代替完整游戏验收。

## 快速构建

建议首先构建共享运行时的主机测试。需要 CMake 3.22 或更新版本，以及支持
C++20 的 Visual Studio 2022 C++ 桌面开发工具链；发行审核工具另需 Python 3。
该入口不需要 IDA、原游戏数据、原版 DLL 或专有 RenderWare SDK。

在源码根目录使用 PowerShell：

```powershell
$Pal4Build = Join-Path $env:TEMP 'pal4-portable-build'
cmake -S fresh/ports/runtime -B $Pal4Build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON -DPAL4_ENABLE_GLES=OFF -DPAL4_MODERN_GAMEPAD=OFF
cmake --build $Pal4Build --config Debug --parallel 2
ctest --test-dir $Pal4Build -C Debug --output-on-failure
```

测试使用 Debug 配置以启用断言。请将构建产物放在源码目录之外；本项目开发环境
统一使用 `H:/PAL4/objs/`。此命令运行组件测试，不会启动可玩的游戏。
更多依赖和平台说明见 [BUILDING.md](BUILDING.md)。

## 目录与源码包范围

完整开发仓库中，`fresh/ports/runtime` 为共享运行时，`fresh/ports/android`、
`fresh/ports/switch_game`、`fresh/ports/linux_game` 为平台应用入口，
`fresh/native/renderware` 为需要外部专有库的 Windows 研究路径。

本公开仓库当前发布的主机源码预览，仅包含主机合同构建所需的源码集合、测试、
第三方依赖及发布说明；不包含全部平台应用、专有 SDK、研究归档、图片或 Git
历史。包内的 `SOURCE_MANIFEST.json` 列出实际文件、SHA-256 及导出范围。本次预览不是完整游戏源码发行版或可玩版本。

## 原版资源与发布

运行游戏需要用户自行提供原始游戏文件。本项目的发布包不应附带原版 CPK、
提取脚本、贴图、字体、音乐、视频、专有 DLL 或 SDK 库。项目许可证不授予
这些材料的使用或再分发权。

本公开仓库采用独立历史，原开发历史中的旧测试 EXE 和研究材料未纳入。
项目许可不代替对原版衍生材料和平台依赖的权属审查。
Android 的 `publicRelease` 配置也仍需完整构建、签名和设备验证。
具体证据、已完成事项及剩余检查见 [发布状态](docs/release/STATUS.md)。
