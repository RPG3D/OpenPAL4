# 原视觉特效任务

`EffectTasks` 将原 Dialog 初始化、GOM 调度器和 Showobj/Hideobj/Scale/ChangeColor 处理器接到公共视觉对象。配置仍直接来自用户 Effect.cpk；不生成或发布原资产、恢复脚本。

## 架构

- `EffectTaskBinding` 拷贝原 XML 配置，借用 `EffectVisualTarget`。对象必须晚于调度器销毁。
- 原 `DialogObject::InitializeTasks`（629C60）读取等待/持续参数并绑定目标；原 `EffectSystemTaskObject::SetupState`（63FCF0）保留继承前一任务时间的语义。
- 原 `GomTaskContainer::Update`（5ECEE0）处理等待、开始、更新、结束和下一遍清理。不改成按总时间直接设置最终姿态。
- 真实 `GomTaskBaseState` 保存基类。受检查句柄映射任务/链表/名字分配，派生处理器通过可平凡复制数值的对象表示副本读写；不将数组强转成其他类，不截断64位指针。GCC的class-memaccess警告通过显式字节视图复制解决，没有关闭警告。
- `EffectCylinder` 通过原变换/颜色处理器提供视觉接口。颜色模式回调沿用原5A6010空操作，不误改为定时淡出。随机回滚只有绑定有效15位随机服务才可运行。

已支持等待模式1..3、负预期时间的持续运行与显式Stop。不支持的类、缺目标、文本转数值和非法时间明确报错。当前只接受已创建的视觉对象；原全局效果禁用/虚拟对象跳过分支仍待接。Decress已通过原限量发射接口连接粒子，拒绝无该facet的目标。

## 原 H_077 圆柱投影

开发预览从原计划选择TaskId=2、ObjectId=yzt3的四行，参数未改写：

| 顺序 | 原任务 | 等待模式/时间 | 持续时间 |
| --- | --- | --- | --- |
| 1 | ShowobjTask | 2 / 0 | 0 |
| 2 | ScaleTask | 2 / 0 | 0.5秒 |
| 3 | ChangeColorTask | 2 / 0.3秒 | 0.2秒 |
| 4 | HideobjTask | 1 / 0 | 0 |

Scale_Result=0，所以(20,5,20)是每秒变化量。颜色任务仅将alpha从254减到0，保留RGB。Hide等待前一任务自然结束；开始和更新分属不同帧，任务全部进入结束阶段后下一遍才清理。

这四行投影用于验证公共执行器，不能替代完整H_077的十任务/三对象，也不放行剧情97。后续已连接三对象十任务，见[EFFECT_SEQUENCE.md](EFFECT_SEQUENCE.md)。H_077未引用Clump模板；父级变换和原Dialog/GOB停止/切场生命周期仍待连接。

## 验证与命令

主机测试覆盖等待/开始/更新分帧、并行任务、串行隐藏、继承时间、原缩放/alpha处理器、无限持续和停止、缺目标/未知类及失败清理。原包测试60Hz下33帧自然完成，独立参数计算最大缩放误差约0.000004。

主机直接原包：`pal4_portable_effect_tasks_tests.exe Effect.cpk report.json`。独立核对工具`verify_effect_tasks.py`从原文档报告重新取参数，检查逐帧缩放、alpha、最终隐藏和四任务释放；没有证明原SDK数值完全等价。

Android：`verify-effect-tasks.ps1 -Device emulator-5556`。在原圆柱开发预览中通过不带scene的`action=tasks`启动；逐帧文件`effect-task-frames.jsonl`，状态中新增`effect_tasks`。脚本检查后台冻结、同进程EGL保持、自然结束、复位及切场释放。先等待实际任务帧再切后台，避免在动作刚排队尚未更新时误判。报告保存在本地objs/emulator，不进入APK。

前轮基线：APK 50515F83969491F6AF6DB94D13951918E1072D0C12629E2F460AC7BFFF06099C。主机52/52、Android双ABI、Switch完整ELF/NRO通过，未部署Switch设备。安卓任务进程9235、EGL1→3，32帧/0.579076409秒，后台0.27秒处暂停，四项任务全部开始/完成/删除；实际缩放最大误差0.000002823。圆柱像素回归进程9398、图形9607、源码对照9804均通过。本包完整21句剧情未重跑，完整剧情继续使用302C基线。

前轮2EA0包圆柱四任务回归进程10494通过。后续9771包已验证Decress/Rtk控制及三对象十任务，见EFFECT_SEQUENCE.md。
