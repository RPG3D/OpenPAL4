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
