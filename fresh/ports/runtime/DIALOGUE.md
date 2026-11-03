# 原 CSB 剧情对白

`DialogServices` 处理240 `giTalk`和123 `giSetPortrait`。公共 C++ 层拥有脚本等待令牌、对白、头像、打字进度及分页状态；Android 使用 Canvas，Switch 测试版使用 SDL_ttf 与 GLES 绘制。两者均非 CEGUI 页面，尚未证明与原版像素一致。

## 复用的原逻辑

- `ScriptTalkHandler`：240登记等待处理器，只在窗口隐藏时设置文本；窗口关闭后处理器释放脚本。
- `ScriptScenarioPortrait`：123只在窗口隐藏时设置头像。
- `ScenarioDialog::HandleTextDisplay`：确认键依次补全当前页、翻页或关闭；关闭后清理头像，再恢复脚本。
- `ScenarioDialog::SetPortrait/Layout`：复用原编号imageset扫描、缓存/默认图及左右布局，不把64位指针写入原32位地址槽。
- 原 `OLSDialogEditbox::updateSelf`（OIRAMLOOK RVA `0xB8AB0`）累计时间严格超过间隔时清零，单次调用至多触发一次更新。公共服务的 `ConfigureRich` 已接入字符、节点及行转换状态；最后一个字出现后仍完成转换，确认只补全当前页，再次确认才翻页或关闭。原 DLL 使用共享静态计时，当前服务仍按对白实例计时，完整等价尚未验证。原版轨迹和验证范围见 [对白逐字状态](DIALOG_TYPEWRITER.md)。

本轮 IDA 复核：`0x5DFB10`、`0x5D5760`、`0x4B8450`；所有权分类均为待人工核验，已确认调用链为游戏侧适配，字体/XML/图形库不进行私有重写。

## 用户资源与平台接口

通过已验证的 `PackageReader` 直接读取用户 `gamedata/ui.cpk` 中的 `layouts/ScenarioDialog.xml`、字体描述、编号头像imageset、PNG及字体文件。TinyXML使用公开源码和现代STL，来源见[授权说明](../../third_party/tinyxml-2.5.3/PROVENANCE.md)。

资源层保留源图集像素区域，并依据 `NativeHorzRes/NativeVertRes/AutoScaled` 计算800×600界面坐标下的逻辑尺寸；以当前 CPK 描述为准，不混用此前修改版或开发解包目录。Android 从 `ui.cpk` 在内存读取字体和图片，字体通过匿名内存文件供系统字体接口使用。教程预览已在模拟器验证；实际对白及不同安卓内核尚需验证。新游戏开始时先准备字体和对白背景，头像按需读取。

Android用原字体及公共 `StaticLayout` 测量行与页，回传页面结束的Unicode字符位置。公共层校验边界并掌握显示进度；generation和对白ID拒绝旧布局/输入。触屏、Enter、Space和手柄A映射到同一确认入口，按键自动重复不触发额外确认。GL线程停止时，打字和脚本时钟一同停止。

## 当前边界

当前处理普通 UTF-8 及 colour、dc0～dc3 颜色标签，另保留文本节点边界供 CEGUI 排版；未知标记明确失败，特殊控制和图像标记仍待接入。剧情语音目前关闭，保存原 CSB 的语音参数，不声称已经播放配音。背景、头像和提示图使用原资源，提示图的完整序列动画、原字体/打字/分页像素等价仍需对照。

Switch 测试版教程隐藏时释放其内存图集、字体和 GPU 场景；对白关闭时释放图集和对应 GPU 资源，并以代次及对白 ID 区分重开的页面。此生命周期修改尚未实机验证；菜单和 FPS 文字使用的字体仍在 UI 存续期间保留。

## 验证

主机和 Android 模拟器合同测试覆盖严格间隔边界、大时间步至多推进一字符、等待恢复、补全文字、翻页、关闭、头像清理、可见时头像门控、过期输入、非法页面边界及高清图集缩放。原UI包读取检查使用 `pal4_portable_dialog_tests 原ui.cpk 输出.json`。

完整设备路径使用 `verify-media.ps1 -VerifyDialogue`；对白部分也可在首条对白等待时单独运行 `verify-dialogue.ps1`。验证器按原CSB旧引擎记录核对前二十句文本/语音参数，通过实际按键、绘制事件、截图和字体/图片输入SHA256检查显示与恢复，并核对解除HOLD与动作结束等待，继续到下一个未绑定服务。章节验收仍为false。
