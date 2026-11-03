# 原始 CEGUI 对白页

`ChapterUiSession::openDialoguePage()` 从用户 `ui.cpk` 读取图集、字体和原始 `layouts/ScenarioDialog.xml`，在内存中加载。未生成或读取离线布局缓存。

## 实现边界

- 保留原窗口树、相对坐标、背景颜色、图片和序列动画；字体随页面关闭释放。
- 原游戏对白框禁用编辑功能。专用 XML 适配器将该页的 `EditDialog` 映射为 `PAL4/DialogueText`，应用颜色、行距、字体及区域，清除设计期示例文字。通用布局加载仍不支持原 `SDialogEditbox`。
- 选择颜色只校验格式；光标、选择刷和滚动条装饰逐项核对原引用，仅用于被禁用的编辑器，不绘制、不提供编辑交互。未知属性和不支持的引用明确报错。
- 显式字体引用须在字体管理器存在；上游退回默认字体的行为不能掩盖资源缺失。

这不是完整 OIRAMLOOK 编辑器。共享对白服务、运行时头像与左右镜像已由 `ChapterDialogueUi` 接通；Switch 0.2.13 正式对白入口已替换并安装。安卓 CEGUI 构建已通过应用独立样本的绘制与 A 确认测试，Java 不再重复排版；手机当前已安装版仍为 Canvas。实际新游戏 CEGUI 流程及 Switch 画面仍待验证。见 [共享对白绑定](CEGUI_DIALOGUE_BINDING.md)。

## 验证

安卓模拟器使用原 `ui.cpk`、800×600、FreeType 2.3.1，原 XML 的正文区域为 `(38,483)`、695×108，三个展示控件禁用状态正确。两次会话绘制一致，关闭后窗口、图集和默认字体引用均清理。未知属性、非法颜色、关闭场景格式、NaN 行距、缺失字体和错误编辑装饰六项注入测试均拒绝，失败不破坏已打开页面。ARM64 与 Switch 桥接库编译通过，尚无 Switch 本项实机证据。

PC 原 DLL 加载同一布局、同一字体和“你好你 A”，序列显式选择首帧。排除 SDK 的 RW 标记矩形 `(20,540)-(90,580)` 后，477200 像素中 3 个 RGB 像素不同、最大通道差 1。箭头区域 `(741,568)-(768,593)` 一致；严格逐像素一致仍未通过。

![PC 原 DLL 与安卓原始对白页](images/cegui-scenario-page-comparison.png)

安卓诊断：`PAL4_CEGUI_SCENARIO_PAGE_RGBA=/data/local/tmp/scenario-page.rgba`。PC 使用 `image_widget` 模式、`PAL4_CEGUI_SCENARIO_CHINESE=1`、`PAL4_CEGUI_SEQUENCE_PROBE=1`、`PAL4_CEGUI_SEQUENCE_STEPS=1`，字体路径指向从同一原包核验的探针字体。图像输出仅为测试证据，不参与游戏资源装载。
