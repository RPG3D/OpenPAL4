# 界面序列动画

对白翻页提示的 `OiramLook/StaticSequence` 已接入 CEGUI 会话。`SegImage=sanjiao` 直接读取用户 `ui.cpk` 内的 `sequence_images/sanjiao.seqimage`，解析到内存，再引用已有图集中的图片。原包定义为两帧，每帧 0.2 秒，循环次数 0 表示持续循环。

## 模块与生命周期

- 公共 `ui_sequence` 解析定义并推进共享播放状态，不依赖 CEGUI 或平台。
- `SequenceImageWindow` 实现 `SegImage` 属性，经 CEGUI 时间更新选择图集帧。绑定不会立即改变原 `Image`，第一次更新（包括 0 秒）才选择当前帧。
- 会话按资源名称共享播放实例；多个控件更新同一实例时，会共同推进时钟。缓存只保留弱引用，最后一个控件销毁后释放状态。
- CEGUI 延迟销毁窗口。会话关闭页时清除绘制队列并清理窗口销毁池，再释放图集；自定义工厂由会话拥有，存活时间覆盖整个 CEGUI 系统，避免工厂提前析构。

## 原版行为与验证

原 DLL 公共接口已核对持续循环及人工定义的 1、2 次循环样本：每次更新最多扣一个完整周期，大时间步可留下超过一个周期的剩余时间，下一次更新继续处理；有限循环结束时保留此前选中的帧，不强制切到最后一帧。共享实例测试确认一个控件推进后，另一个控件的 0 秒更新会显示同一当前帧。私有状态读取仅用于 PC 隔离诊断，不进入便携运行时。

主机和安卓合同覆盖 15 步原版帧轨迹、有限循环停止、坏 XML、缺帧/重复编号、非法时长及无效时间。合同使用人工资源名，不附带游戏序列文件。

安卓模拟器直接读取原 CPK，经 CEGUI 窗口绘制两帧。在同一原背景上，对照 PC 原控件区域 `(741,568)-(768,593)`，两帧各 675 个像素的 RGB 均逐像素一致，严格比较通过。区域包含背景渐变，不用背景像素数量代替动画证据；两帧图像不同，帧轨迹同时校验图片身份。

![第一帧：PC、安卓及差异像素](images/cegui-sequence-comparison.png)

实际会话另验证 CPK 解析器、同名资源共享、系统时间脉冲和关页释放；两次创建销毁、已有页面、对白文字/分页及 GL 上下文重建回归通过。Android ARM64 与 Switch 章节桥接库编译通过，尚未实机运行。

## 复查与边界

PC 设置 `PAL4_CEGUI_SEQUENCE_PROBE=1`、原对白字体/页面诊断参数；`PAL4_CEGUI_SEQUENCE_STEPS=1/3` 分别截取两帧，`PAL4_CEGUI_SEQUENCE_SHARED=1` 检查共享时钟。`PAL4_CEGUI_SEQUENCE_FILE` 接收本地人工定义，供有限循环诊断使用。

安卓设置 `PAL4_CEGUI_SEQUENCE_RGBA_PREFIX` 导出两帧；运行 `pal4_portable_ui_sequence_tests` 验证公共逻辑。严格比较用 `compare_dialogue.py PC.rgba 安卓.rgba --region 741 568 768 593`。`runtime_probe ui.cpk --read 成员名` 可直接审阅原包 XML，不生成运行时缓存。

完整 `ScenarioDialog.xml` 仍因缺少 `SDialogEditbox` 工厂及其属性支持而无法加载，当前游戏未切换到整页 CEGUI。提示的显示/隐藏、触摸确认和真实剧情生命周期仍需接线；现有动画验证不证明完整 OIRAMLOOK 控件库已完成。
