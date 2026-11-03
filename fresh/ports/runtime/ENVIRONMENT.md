# 场景树木与天空

共用 ArenaResources / ChapterHost 装载和 GLES 绘制，Android 与 Switch 只调用同一环境绘制入口。资源来自用户自己的 PALWorld 场景包；运行时发布包不含游戏资产。

## 原版依据

2026-10-27 经 ownership classifier 后复查 IDA：660740 装载 BSP 与 trans/clip/clipNA/water/sky；518A40 的 518E0E–518E81 将前三种附加 clump 加入世界。51A1C0/51A230 单独提交 sky（world+132），水另有路径，不能把所有 DFF 当作世界自动绘制。

Q01 的树木、草木位于 Q01_clip.dff（3,646 个可绘制 atomic、24,262 三角形），clipNA 另有 648 三角形。此前只转换 BSP，造成附加植被完全缺失。天空 Q01_sky.dff 有 146 三角形，使用原 baiyun/yj01/yun 贴图。

## 实现边界

prepare_android_environment.py 复用 DFF 解析器，保留原 frame 层级变换、顶点颜色、两组 UV、材质和三角形方向；相同材质静态合批。Q01 的 3,646 个小 atomic 合为 18 次提交，clipNA 为 4 次、天空为 3 次。透明像素仍走共用着色器的 alpha discard 和深度测试。

P4AD v2 明列存在的环境层，缺少可选原模型不会创建假模型；v1 仍兼容。损坏、重复、非法层名及截断元数据拒绝加载。新增 P4SC 缓存放在 environment/ 下，配置文件必须最后更新。ArenaSource 的缓存失效检测仍未实现，更新工具须核验哈希。

天空使用原模型及世界坐标，GPU 将深度放在背景，关闭天空深度写入，避免模型超过普通场景远裁剪距离而消失。GPU 像素回归覆盖远距离天空可见、前景遮挡天空、深度状态恢复。此 GLES 背景处理是移植适配，不声明与原 D3D 管线逐像素等价。

本轮完成静态几何显示；UVA 云层滚动、风动、双天空切换、水面独立绘制仍未绑定，不能标为完整环境动画。

## 验证

转换测试覆盖 frame 平移、UV1/prelight 保留、合批计数、缺失 UV1 与非法 frame 拒绝。Arena 测试覆盖 v1/v2、截断、重复和非法名称。真实 Q01 缓存来自用户 CPK，143 份输入逐项 SHA256 核验；缓存部署也逐文件读回核验。设备画面结果另见 Android STATUS 与 Switch README，不以编译成功代替实机显示通过。

真实天空资源由 tools/environment_gpu_probe 在 Android 原生 GLES3/EGL pbuffer 中单独绘制，640×360 的 230,400 像素可见，蓝天白云远山已目视；静态图层边界差异仍保留。安卓正常新游戏到175调用/309节点截图已确认植被，Switch 0.2.8 只读状态确认额外25次提交/25,056三角形/天空146三角形，GL0。完整场景 EGL 恢复仍未专门实机复测。
