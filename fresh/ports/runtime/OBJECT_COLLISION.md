# GOB物件碰撞

SceneObjects已支持向SceneCollision登记真实GOB几何。Android与Switch共用此模块，不依赖平台输入或GL；ChapterHost已接入场景装载绑定，实际章节自由移动仍未验收。

## 数据与生命周期

P4CG v1（.p4g）保存刚性DFF所有atomic的局部三角形、frame、flags和材质索引，包含不可见atomic。prepare_object_collision.py复用既有DFF解析器，不用渲染网格替代碰撞网格；蒙皮GOB明确拒绝。加载时验证预算、索引、有限浮点及与角色缓存的一致性。

P4AD v3在环境列表后增加能力位，bit0要求每个非效果GOB有完整.p4g，bit1要求完整EVF事件碰撞缓存（见SCENE_EVENTS.md）；仍兼容读取v1/v2。旧缓存缺少几何时不能声称已绑定物件碰撞。旧版NS运行时不支持v3，不得单独更新该缓存。

BindMovement接受碰撞owner和逐atomic许可查询，UnbindMovement或析构移除登记。碰撞owner必须比SceneObjects长寿。动画、动作和模型变换更新几何，静态帧不重建；隐藏立即移除形状。GOB与玩家使用不同owner句柄，避免错误自排除。

## 原版边界

原5A6560取GOB +188作为mode，硬阻挡1、近地探测0；虚表+58回调由5A6020返回0。不能把mode当成事件回调。

8450许可通过AtomicDistanceInstance::CollisionAllowed查询，与绘制共享状态；未处理atomic默认许可，已处理则读距离筛选结果。SceneCollision的旧字段plugin是许可兜底值，并不表示文件是否带插件。动态回调存在时以实际查询为准。

## 验证与剩余工作

N01原数据包含3个GOB、7个atomic、920个三角形；Q01为33、46、2606。主机与安卓独立探针验证显隐、动画变换和8450查询：N01形状数1→0→1，Q01为33→30→33。两场景分别43、154份转换输入逐项与用户原CPK的SHA256一致。

报告位于H:/PAL4/objs/pal4-android/gob-collision-verification及prepared-data/gob-collision-runtime；14项定向主机测试、Python转换测试和Android双ABI构建通过。探针未运行剧情、未替换已安装应用。

ChapterHost已在完整缓存能力存在时绑定墙体和GOB碰撞，场景切换与退出先解绑物件，再释放碰撞与距离对象；旧缓存明确记录能力缺失。原包如包含尚未加载的intri则报错，避免静默漏碰撞。双ABI构建通过，尚未用实际章节验收该接线。玩家移动和模态输入已接入章节循环，后续需接事件owner与脚本队列，并手动验收教程关闭后的移动、洞穴剧情及战斗。原资源缓存仅由用户文件生成，不加入公开运行时包。
