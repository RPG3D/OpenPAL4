# Atomic距离裁剪与渐隐

公共模块`atomic_distance`不依赖平台或GPU。Android/Switch共用原处理器；GLES只接收绘制批次的可见性和RGBA，并报告实际提交三角形数。

## 原行为与所有者

- `0x5D5620 → 0x663750`：脚本120通过`ScriptCameraDistanceOptimization`写8450引擎开关。
- `0x663380`：处理整个clump，保存原回调并将几何的所有材质设白；重复安装不重置状态。
- `0x663470/0x663590`：以Atomic首Morph球心、当前骨架/实例矩阵与真实相机位置求距离，复用原`GameVectorMath`近似长度。达到阈值则不提交；阈值内50单位将RGBA四通道一起渐变；重新进入内侧恢复白色。
- 关闭开关会继续调用绘制回调，并保留此前材质颜色。不会擅自恢复白色。
- `0x5CD050/0x5CD150`：原默认档位明确为1，对应1200单位；档位0对应1800。原设置模块的`DefaultState/ScaleDistance`与原Reset/Apply共用这些值；这里只接Atomic距离，不冒充尚未接入的效果系统范围设置。

`DistanceOptimization`持有每个运行时的引擎开关和阈值。`AtomicDistanceInstance`借用不可变Actor，持有每个实例的Atomic插件位和可变材质，保持共享几何/材质槽引用；按原逆流遍历顺序调用`RwPlugin8450`，在原回调点保存该Atomic的实际绘制外观。适配用经过检查的整数句柄，不截断宿主指针，也不模拟SDK私有对象实现。

角色、NPC和GOB分别保留实例；切场释放旧场景实例，玩家实例继续保留。EGL只重建GPU，CPU材质及插件状态继续存在。场景装载恢复原启用开关，脚本120再修改同一状态。BSP世界扇区不经过这个Atomic插件。

## 输入与边界

必须有P4AR v3的Atomic/几何/共享材质/批次映射，详见[角色格式](ACTOR_FORMAT.md)。旧v1/v2明确失败，不能以整个模型的合并球或绘制批次数量猜测。顶点与Atomic矩阵取自同一次动画采样，包含HOLD、混合和实例缩放。

当前支持已装载的骨架动画和首Morph球心；没有绑定Morph目标切换、原SDK所有数值等价、跨原生clump克隆的材质所有权及其他材质插件。这里的通过不代表整个RenderWare或章节完成。

## 验证

主机覆盖逐Atomic远近、实例/相机/骨架变换、50单位渐隐、关闭保留颜色、重新启用、共享几何和共享材质；原GlobalDataSettings独立回归。GLES检查裁剪后无提交、灰度RGBA的实际混合像素以及白色恢复，保留透明深度写入规则。

`atomic-distance-probe.json`保存引擎状态、各实例更新帧数、原回调次数、裁剪数、批次可见性和RGBA；`status.json`记录`gpu_atomic_appearance_contract`及实际对象/NPC提交数量。连续验收核对原CSB调用顺序、120之后66就绪，并比较EGL恢复前后的材质和可见性。设备最终结果以[安卓状态](../android/STATUS.md)为准。
