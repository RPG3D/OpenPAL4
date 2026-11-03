# 对白富文本

原 CSB 的文本保持原样读取，不随运行时发布恢复的脚本。公共 DialogServices 同时保留 `text` 原文与 `display_text`，颜色范围使用 Unicode 码点索引和 ARGB；脚本等待、分页和打字统计均按可见字符处理。

解析结果另保留 XML 文本节点范围，并经 `DialogView.text_nodes` 交给 CEGUI。颜色区间仍可合并同色内容，节点边界不合并：原控件按节点宽度排版，同色嵌套也会改变后续文字位置。节点元数据仅存在运行时内存，不生成资源缓存。

## 解析与绘制

- TinyXML 负责标签语法；支持 colour 的 red/green/blue/alpha 四个 0..255 通道及 dc0..dc3。
- dc0 恢复默认色，dc1/2/3 分别为红/绿/蓝。拒绝未知标签、错误 XML、过深嵌套和无效范围，不静默删标签。
- 在创建原脚本等待之前完成解析；原处理器调用异常时清理临时状态，后续对白仍可执行。
- Android 通过 SpannableString/ForegroundColorSpan 和 StaticLayout 绘制；逐行裁剪保留原打字/分页状态。
- Switch 使用相同文本与颜色范围，经 SDL_ttf 分段栅格化；颜色不会进入 CPK/CSB 或平台脚本逻辑。

## 证据与边界

用户原 OIRAMLOOK 公共 API 探针已确认单标签：默认 CC336699、colour FFFFBB00、dc0 CC336699、dc1 FFFF0000、dc2 FF00FF00、dc3 FF0000FF。此前独立混合/嵌套探针因初始化前提失败；现由完整 CEGUI/字体/原 ScenarioDialog 布局验证 `<dc0>你好<dc1>你 A</dc1></dc0>` 及同色 dc0 嵌套，实际保留两个文本节点，换行后有三个片段。安卓实验 CEGUI 的节点位置与 PC 一致，RGB 仍有最大 1 级差异；不证明全部标签、节点或计时等价。详见 `CEGUI_PORT.md`。探针仅用于本地验证，不发布 DLL/字体。

主机覆盖中英文、空白、XML 实体、混色、预设色、坏标签和失败后再执行合法对白。Android 新游戏已通过包含原颜色标签的第 22、24 句及全部 25 句；设备证据见 `emulator/npc-walk-*`。Switch 仅完成交叉编译，未实机验证本次文本变化。

配音、任意原 UI 标签、原字体像素完全等价、多页剧情分支仍未验收。下一真实剧情边界是 81 相机模式，完整章节尚未完成。
