# 怪物路径行为保存与释放

SaveMonsterPath已连接原C02/C03 Release到公共移动世界和ActorPlayback，支持行为0/1。原Accept、移动、地面/角色回写与本保存入口均有独立适配，但完整行为工厂、管理器调度和ChapterHost仍未接通。

## 原顺序与拥有者

复用435C20/436610，依次保存last_behavior_id、速度、动画时间、方向和unsigned wander字节；清B8，再从网格移除并删除helper，最后以0.5秒混合选择C01，失败则Z01。C03还按原逻辑启用动画restart。成员暂停（B8为0）时仍允许保存；未注册、失效句柄或错误类型明确拒绝。

调用方提供实际行为内存节点，不能猜测MST文档根。节点及播放状态由调用方拥有；helper由世界删除，保存后句柄、图像引用和空间关联均失效。C03的返回值来自restart setter，即使两个待机动作都缺失也会返回1，不能将它当作待机选择成功证明。

异常遵循原部分更新边界：XML写到中途失败可能仍保留helper；资源读取在网格移除后失败时helper已经删除。调用方必须停止该行为并重建状态，不能假装全部回滚。析构清理和显式保存应区分，后续工厂须将保存错误传回场景层。

## XML类型与恢复

WriteBinaryXmlInt/Float复用原XmlAttributeRecord::SetInt/SetFloat，保留已有CHAR/INT/FLOAT/STRING/BINARY类型；缺失键新增INT/FLOAT。只更新首个同名属性，其他属性和子节点保留。数值地址空间只用于一次转换，真实存储是现代C++拥有的缓冲，没有宿主指针截断。原共享空串地址改为头文件内联常量，避免为该常量引入整份旧字符串存储实现。

修正BinaryXmlAttribute::ToFloat遗漏的BINARY分支：至少4字节取首个IEEE word，1～3字节替换默认值低字节，空数据保留默认值；与原5F43E0一致。ToInt继续使用同一有界raw读取助手。

重新Accept保留原规则：行为ID匹配才恢复动画时间；wander先保存原字节，读回时按大于零转为布尔值。待机混合尚未结束时当前动作仍可显示为旧动作，不能提前判为切换失败。

## 验证与限制

主机及安卓合同覆盖五种XML类型、短/完整二进制值、保留节点、同名首项、C01成功/Z01回退/均缺失、暂停成员保存、保存后句柄失效、同内存重新Accept，以及XML中途失败和删除后动画读取失败。Win32原XML属性与字符串合同另行回归。

原M01/1三次class1（Z02→Z01）、M02/1四次class0（C02→Z01）各完成恢复、三个移动帧、受控动画时间、保存/删除、待机混合、重新恢复和姿势采样。源探针使用新建内存、固定随机、空静态候选与模型半径作为受控manager输入；不代表实际管理器现场、完整行为调度、渲染或剧情验收。

主机与安卓独立合同沿用monster_path_restore_contract，原资源探针为：

```
pal4_monster_path_restore_tests cycle-source <gamedata> <缓存根> M01 1 <报告.json>
```

证据位于H:/PAL4/objs/pal4-android/prepared-data/chapter1-original-runtime/path-save-probe，含两端报告、verification.json与IDA记录。用户模型、原资源与探针输出不进入公开运行时包；没有执行CSB或自动推进游戏。

最终主机、安卓合同与两端7个原资源周期报告均通过；Android双ABI公共库及Switch完整构建通过，未在Switch运行本轮探针。原有XML属性/字符串合同已在隔离Win32构建中回归通过。已安装APK/NRO未替换。

路径0/1工厂已接入原单轮任务队列，见[路径行为队列](MONSTER_PATH_BEHAVIOR.md)。class2及管理器重复调度、完整场景宿主仍待接。
