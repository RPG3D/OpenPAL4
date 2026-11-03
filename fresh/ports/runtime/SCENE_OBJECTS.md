# 场景对象与97显隐

`SceneObjects`借用当前 `ArenaResources` 和 `SceneMinimap`，拥有每个GOB的可变状态与Action动画。脚本、小地图和安卓绘制消费同一集合；不通过重新读原GOB静态值覆盖脚本修改。退出或切场时先释放GPU对象和此集合，再释放场景资源。

## 原处理器与平台边界

| 处理 | 已复用入口 | 适配内容 |
| --- | --- | --- |
| 服务97 | ScriptObjectVisibility，0x5D4640 | 查名；先移除；显示时再置可见并加入；最后刷新小地图。查名失败直接返回 |
| 场景增删 | GameArenaScriptClumpCallbacks，0x51C890/0x51C950 | 保留活动/可见门控、marker刷新顺序、场景/运动/相机登记和绘制准备/恢复的调用顺序 |
| 基础配置/可见 | GobObjectBase，0x59A360/0x59A8A0 | 复用参数facet及隐藏时的效果门控。当前没有GOB对话/效果上下文；尚无创建它们的服务 |
| 服务210配置重载 | GobObjectBase::ApplyParameters，0x59A360 | 通过GameServices绑定当前SceneObjects并从保留的原GOB文档重读Normal(0)/GetItem(3)基础参数；未知名称无操作。Action/Sound/Machine/Marker/Effect派生重载暂不支持并明确失败 |
| Action显隐 | GobAction，0x59AE60 | 零播放次数按holding-end定位首/末帧并暂停；其他次数按default-play切暂停 |
| Action逐帧 | GobAction，0x59ABA0 | 复用更新、完成计数与结束定位；新增窄更新接口，原大接口经转发仍使用同一算法 |
| 原动画顺序 | AnimationScriptLoader，0x66AD40 | 直接读用户原.tab，复用注释和记录解析，按第一条资源选择初始动作；不猜文件排序 |
| 动画定位 | GameObjectEmbeddedAnimation，0x669BC0 | ActorPlayback.Seek复用原时间设置/矩阵更新；宿主边界额外拒绝非有限数及混合期间定位 |

原处理器中的整数地址仅作为检查过的句柄和字段适配，不存放或截断64位宿主指针。VC6字符串/数组内部由标准容器代替。私有RW对象和插件算法不在此重写。

`ArenaSource::ReadSceneMember`直接读用户scenedata.cpk的小地图；`ReadObjectMember`按原directory字段选择palobject.cpk、palweapon.cpk或PALWorld对应包。原.tab/AMF/AMD由这些入口读取，所有成功读取均进入来源报告；缺可选AMF/AMD沿用ActorPlayback装载路径，缺Action模型/tab/首动作明确失败。

## 绘制与仍未接入的行为

安卓为类型0、6、7上传独立GPU资源，按世界登记、可见、活动及绘制准备状态筛选；Action每帧采样原骨架姿势，使用原位置/旋转/scale，和世界/玩家共享相机与深度。EGL重建保留CPU对象、动画时间和脚本栈，重新上传GPU资源。

准备/恢复标记是公共绘制登记。其中8450已复用原处理器实现逐Atomic距离裁剪与RGBA，并由GLES实际消费，见[距离插件](ATOMIC_DISTANCE.md)；其余插件和原SDK蒙皮数值等价仍未完成。运动/相机登记状态已随显隐改变，但动态对象碰撞尚未消费这些集合。原场景静态碰撞继续独立工作。

NPC行为、类型5机关的逐帧任务/绘制、类型3空间声音、类型8特效、交互/事件、其余GOB类型的派生配置重载与对象存档尚未完成。类型8的97调用明确失败。玩家小地图朝向/HUD绘制仍待接入；97刷新的是实际CPU图标集合，不是已绘制HUD。

## 验证

- 合成资源测试覆盖原.tab与缓存排序不同、显示/隐藏登记、零/有限/无限次数、holding-end、default-play、缺tab及非法帧时间；原GobAction测试另行回归。
- 主机从用户CPK和已核对缓存装载Q01，再执行入口三次97：entrance0隐藏，entrance2显示再隐藏；小地图仅剩entrance1，Entry.PNG坐标(619,208)。报告为 `H:/PAL4/objs/pal4-android/prepared-data/Q01-original-objects.json`，来源位于同名 `.source.json`。
- 安卓连续执行、当前APK及实际服务边界以[状态](../android/STATUS.md)和设备验证报告为准，主机测试不替代章节验收。

当前设备身份见[安卓状态](../android/STATUS.md)。21句后136调用完成272，停97对Jeffect001的类型8特效显隐链；对象calls=6、matched/logs=5，前五次入口/调用者显隐成功，第六次明确失败，不能计为特效成功。Q01实际15对象/1722三角形，GL0；室外淡入已完成，EGL6→7保留CPU材质与可见性。完整室外游玩及Action独立时间专项验收未完成。

类型8的原配置、EFT及任务结构读取已接公共模块，详见[特效文档](EFFECT_DOCUMENTS.md)。场景目录报告新增effect参数；实例创建和绘制尚未接入，不改变上述设备边界。
