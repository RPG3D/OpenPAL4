# 教程数据、窗口与等待

## 已实现

`TutorialServices`复用原`UiFrameManagerSingleton::ShowTutorialDialog`与`ScriptTutorialWait`。2026-10-27归属分类及IDA复查4C0260/5E7F90/5E8040：检查选项和已读标记，必要时建窗口，保存标记后显示；登记36字节mode1等待，窗口为空或隐藏后才释放。首个剧情教程为10。

`Invoke(309)`保留32位编号并复制token。平台关闭须匹配generation/id，`Advance`只返回一次原token；禁用/已读教程也在下一帧完成等待。目录、下一层服务由创建方持有，晚于服务销毁；`Cancel`清等待并拒绝迟到关闭。128位本次游戏已读标记用标准bitset，保存由实际设置owner回调承担。

`TutorialPresentation`是两端共用的窗口数据：原背景、按钮、正文矩形、字体、文字与内嵌图标。布局、字体及图集直接读用户`ui.cpk`；扩展现有`DialogAssets`读取任意已指定图集，未重复写图集解析器。统一坐标同时支持相对父窗口比例和绝对偏移。原26条记录仅包含dc0复位和image标签，保留换行，图标用明确占位符和图片记录交给平台；未知标签明确失败。

Android `TutorialView`用Canvas/StaticLayout显示原图片与字体，支持图标混排、正文裁剪、触摸滚动、上下键10单位滚动、确定按钮和Enter/Escape关闭。输入由模态窗口接收，不透传给底层资源按钮。预览窗口使用独立generation0及递增id；切换资源或新游戏时关闭预览。Java位图与文字状态独立于游戏GL上下文。

## 资源和发布

`prepare_android_tutorial.py`复用已有数据库解析器，从用户Pal4db.db生成P4TU v1，保留CP936文字及全部标记。读取端复用CP936解码，拒绝截断、重复编号、NUL、异常长度和尾随数据。26条缓存约12KiB，只放用户`PAL4/cache/data/tutorials.p4t`，不进运行时发布包。

章节宿主现在使用64位安全的公共数据库读取器直接解析原 database.cpk；上述转换文件仅用于旧版和独立测试。尚未在设备上验证新路径。

## 验证

- 主机与安卓原生CLI已验证原26条缓存、持续可见时等待、明确关闭后恢复相同上下文，以及禁用、重复、32位编号、过期关闭和取消。短测试脚本不替换游戏CSB。
- 两份用户UI包均解析全部26条教程；窗口4个背景层、两枚内嵌图标和原字体可读。独立XML坐标校验最大差约3.8e-8，另一份为0。
- Android独立预览已目视。原`ui.cpk.original`正文高302、文字高315，可滚到底13；切后台再回来，GL上下文1→2，滚动后的正文像素不变，确定按钮关闭通过。测试始终game_calls=0、GL0，未推进游戏剧情。测试后原包逐字节哈希核对恢复。
- 主机教程/对白/脚本栈3项回归通过，Android APK与Switch完整构建通过。当前没有验证整个教程窗口与PC逐像素等价；原滚动条皮肤和按钮悬停/按下外观仍未绘制。

原数据与布局证据：`H:/PAL4/objs/pal4-android/prepared-data/tutorial-original`。设备预览及恢复报告：`emulator/tutorial-ui-verification.json`；精确APK和图片另存`emulator/baselines/tutorial-ui-C4A6114E`。

## 实际剧情边界

309已在ChapterHost接入真实TutorialServices、设置保存与脚本等待；Android与Switch使用同一教程展示数据，窗口关闭后由下一帧完成原等待token。没有UI能力的宿主仍明确停309。当前最后完整剧情证据仍为175调用/309，新接线尚待手动实际流程验证，不能据此宣称已跨过教程。75解锁已接入并通过主机玩家测试，真实教程关闭到75的流程、自由移动、场景事件和洞穴战斗仍待验证或完成。

Switch 0.2.9增加模态教程窗口，A/B关闭、上下键滚动；不自动跳视频或推进对白。实机部署和验证结果另记。

## 设置与解锁整合测试

`pal4_portable_tutorial_flow_tests`把真实FileSettingsStore、TutorialServices、GameServices与ScriptStack连在一起。独立v1样本在显示前保存成32字节v3（兼容读取v1/v2），影片位保持0x80000001；窗口关闭前等待不释放，generation/id不匹配不生效；关闭后下一次Tick才执行75。已读和禁用分支正常释放等待。用非空备份目录制造真实写盘失败时，脚本保持failed，不显示窗口、不产生完成token、不调用75。

测试内短脚本仅用于模块整合，不复制到用户游戏资源、不替换原CSB。教程/整合/设置/玩家4项定向CTest通过；这不替代真实309→75实测。日志：`H:/PAL4/objs/pal4-android/tutorial-flow-build.log`。
