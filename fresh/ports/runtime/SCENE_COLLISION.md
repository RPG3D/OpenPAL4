# 场景移动碰撞

`SceneCollision`复用原ConfigComposite分组、ConfigCompositeCollision球体/位移处理和PalMovementCollisionRouter。新增的是世界坐标三角形与原接口之间的适配，内部仅使用数值句柄，不截断64位指针。独立模块不读取平台输入，不写玩家位置，也不依赖GL。

## 行为与数据

- intri与wall按原固定属性登记；配置对象由调用者提供原owner、plugin、mode、硬阻挡、近地探测及事件标识。未知plugin默认不通过筛选，owner必须明确。
- 上下两个球体、三组检查顺序及wall切向位移保留由原处理器决定。几何查询使用三角形包围盒筛选、原最近点回调与标准浮点归一化；未复刻RW私有空间树。
- 对象拥有其世界坐标几何，可以替换或移除。事件中移除当前对象时，原链表更新和短路标记立即生效，节点存储延至查询结束释放，避免旧回调访问悬空节点。没有事件服务时明确报错。
- 当前输入是已变换的三角形；没有验证非均匀缩放、SDK球体遍历边界精度或完整动态场景与原PC相同。

## 验证

主机与安卓原生CLI通过空场景、贴墙切向移动、穿墙硬阻挡、反向退出、intri检查、owner排除、plugin/patch筛选、进入/离开、替换几何和事件中移除自身。原配置碰撞测试也接入便携测试集。

N01/Q01用户原碰撞缓存分别按atomic登记wall并执行查询，传入模拟器后的SHA256相同。该检查只证明真实缓存可进入新适配器，不证明实际玩家轨迹正确；两份缓存都没有提供intri或配置对象，这些空分组不能被视为已核验原包不存在。

8项定向回归、Android双ABI及模拟器测试记录：`H:/PAL4/objs/pal4-android/emulator/baselines/scene-collision-8224c2f6`。曾有一次ADB覆盖测试程序后丢失执行权限，日志另存adb-before-chmod-log.json；修复权限后的完整成功记录为adb-final-log.json。

## 真实章节尚缺

SceneMovement已支持按原CheckActive回写地面落点，PlayerFreeMovement已连接独立的输入/碰撞/位置提交。N01/Q01独立探针已从原CPK核验intri缺失；SceneObjects层已完成真实GOB配置对象绑定与双平台探针验证；主运行时仍需连接原包检查、该绑定、事件owner和章节每帧提交。真实控制器使用+0x24默认18作为球体半径和点击距离门限；角色身高来自原角色包围球直径，两者不能混淆。

ChapterHost已接入场景创建与GOB绑定，已接玩家自由移动但尚未在实际剧情验收。构建及独立测试不能算作实际自由移动、场景事件、石沉溪洞战斗或整章通过。发布包不加入这些原资源缓存；它们只由用户原文件在本机准备。

后续原资源单帧链路和当前证据见[玩家自由移动](PLAYER_FREE_MOVEMENT.md)，不将独立探针算作实际章节输入验收。

GOB几何、8450动态许可与生命周期见[物件碰撞](OBJECT_COLLISION.md)。

事件体登记、原root+88遍历中止与CSB压栈已接入ChapterHost，见[场景事件](SCENE_EVENTS.md)；类型4相机与次移动适配器仍待接。
