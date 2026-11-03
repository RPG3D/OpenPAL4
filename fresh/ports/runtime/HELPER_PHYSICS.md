# 行为辅助对象物理核心

已把原Win32辅助对象适配器中的物理组合提到公共BehaviorHelperPhysics，供Win32、Android与Switch共用。当前只完成受控外力积分；真实路径/随机转向、碰撞查询、角色回写及生产行为工厂尚未接通，不能算作洞穴玩法完成。

## 复用与边界

本轮ownership gate后重新核对IDA五条游戏函数：65F100限制低速方向，65F1E0限制外力、除质量、平滑并积分速度/位置，65F540提交速度方向，65F5A0更新曲率与历史位置，65F7A0预测位置。继续调用已有GridManager、ObjectMovementScalars/Vectors与ObjectTransformBasis实现，不新增另一套积分公式。

公共入口接受GridManagerState和真实向量引用，不将宿主地址截断为32位。旧虚拟槽的float参数只传受检查常量token，向量通过适配器引用提供。原表ID、nested_grid等字段仍是数值；本核心不解引用它们。

只接受原路径端点/随机辅助对象两种表ID。elapsed必须有限且非负，请求向量必须有限。预测在局部副本执行，不改原图像。GridManagerState仅186字节；与更大的ObjectMovementState通过字节视图复制，原Win32指针壳仅拷回这段前缀，保留派生路径、端点、随机范围等尾部。

旧共享UpdatePhysics对零位移采用已有保护，与原版除零行为仍有差异；本次未改它，不能据此宣称所有浮点细节与原版等价。没有猜测原配置默认值或增加空操作移动后备。

## 验证

- 主机CTest：pal4_behavior_helper_physics_contract。
- 安卓模拟器x86_64：独立behavior_helper_physics_contract，不启动游戏。
- Win32：独立编译旧NativeBehaviorHelperPhysics壳与同一核心，逐帧比较预测/积分图像及前后哨兵。native/package也注册pal4_native_behavior_helper_physics_tests，使用同一测试和PAL4_TEST_NATIVE_HELPER_PHYSICS开关；该完整包目标本轮未运行。
- 两种表ID各120帧受控外力；质量2、外力上限4、速度上限1时，2秒位置Z=1.67425978，末速度1，三端输出相同。
- 检查首帧解析预期、外力/速度限制、低速方向限制、基向量/曲率、只读预测、无效输入与类型拒绝、重定位结果、opaque字段和派生尾部不变。

证据：H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/helper-physics-probe/verification.json。受控合同不证明真实场景自主移动、接敌或战斗；尚未替换APK/NRO交付包。Android双ABI公共库和Switch完整构建也已通过；这些是构建证据，Switch未执行新核心。安卓重新push后执行权限被重置，首份报告失败；chmod 755后同一程序通过，失败报告保留为verification-before-chmod.json。

下一步按原配置/路径/碰撞链提供真实IMonsterBehaviorActionFactory，然后接管理器帧与世界注册，保持非空怪物场景门禁直到所需行为完整实现。

原路径读取与查询拥有者已完成，见[场景路径](SCENE_PATHWAYS.md)；后续辅助对象可消费这些原路径，原C02/C03 Accept/config现有独立公共适配，见[路径行为恢复](MONSTER_PATH_RESTORE.md)；生产转向、碰撞与场景调度仍待接入。
