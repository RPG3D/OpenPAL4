# 玩家位置提交

`PlayerPlacement` 复用 `ScriptPlayerPosition::SetCurrent`（IDA 0x5E0B10），由原CSB服务250调用。顺序为：提交位置 → 按跟随标志更新相机/监听器 → 计算模型查询范围 → 选择并检查地面 → 更新地面属性和小地图 → 选择C01 → 注册阶段1通知。

查询范围为模型包围球半径的两倍，再分别乘0.35和2.53；不使用固定身高。原数据来自用户DFF，经[角色v2缓存](ACTOR_FORMAT.md)保留。共享运行时用检查过的数值句柄和拥有所有权的通知名称，不截断宿主指针。

主/次地面选择复用原 `PalMovementAdapter`（0x428D40/0x428E50）。静态查询从线段顶部取最近交点，叠层时选择较高的有效面；保留来源Atomic/材质/三角形索引。当前安卓二级几何尚未绑定。

通知保留原拼写 `Player-TIANHE-Player-Movment`、mode0、enabled1、phase1。位置与旋转共用PlayerNotifications；相同名称/阶段替换旧项，处理器清理已关闭项；这不是人为增加的移动等待。安卓250在成功提交时注册此通知，后续处理器清理已启用项；不额外暂停或跳过脚本。

## 实际边界

原NewGame的250已提交 `(7,7,-22)`。天河初始半径约39.2483101，按原公式得到above=27.4738159、below=198.596451；静态地面命中来源三角形82，y=6.88754797。位置保持脚本给出的y=7，此步骤没有自行吸附地面。

随后读取P4CL v2原色，执行[地面属性](FLOOR_ATTRIBUTES.md)及温度选择回调，确认两个用户场景包均无MiniMap.df后走无活动图标分支，选择C01并注册原phase1通知。成功快照为 `completed=true`、`floor_updated=true`、`grid_updated=true`、`phase=submitted`；通知随后清理。这里grid_updated表示已执行原无层分支，不代表地图绘制完成。跟随105当前为false；启用跟随的位置回调仍明确失败。253旋转、66场景就绪及160/157权限已接入，见[开场入口](SCENE_ENTRY.md)。

`player-placement-probe.json` 记录请求、已提交位置、查询范围、命中面和执行阶段；`status.json` 采样同一CPU对象。安卓完整媒体验证与主机实际N01查询对照，并检查EGL恢复不改变位置或重复执行250。章节尚未验收。

## 验证依据

- 主机覆盖根/子骨架球合并、旧缓存缺失、损坏v2数据、叠层地面、主次切换、位置不吸附、回调顺序、同名通知替换与失败阶段。
- Python测试覆盖未绘制Atomic保留、遍历顺序、非有限包围球拒绝。
- 独立Win32 SDK null后端读取4个原DFF，原始球和v2缓存逐位一致。合并后的数值仍有差异，见 `H:/PAL4/objs/pal4-android/bounds-sdk-oracle/comparison.json`；不宣称原版数值完全等价。
- 当前应用仍为开发包，资源缓存不属于最终发布资产。发布约定见[原始资源](RAW_RESOURCES.md)。
