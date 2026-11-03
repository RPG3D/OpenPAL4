# 原圆柱对象与纹理

公共 `EffectCylinder` 复用原 CylinderObject 的配置、复位、显隐、变换、颜色与定时 alpha 处理器，通过受检查的模型/文档/帧句柄连接公共 Scene。Android 与 Switch 共用这层 C++ 和 GLES 绘制接口；不保存宿主指针，不复制 RenderWare 私有对象布局。

## 数据与所有权

从用户原 effect.cpk 读取 H_077，ReadEffectPlan 选择 yzt3 模板。文档节点拷贝归实例所有，网格持有 CPU 顶点/纹理数据，纹理缓存保存原字节和不可变解码结果；实例借用 EffectTextures 和 EffectObjectCounts，两者必须晚于实例销毁。EGL 重建只上传 GPU 数据，不能重建 CPU 对象。构造失败回滚资源和 live 计数，离开预览先销毁实例，再释放缓存/包/文件流。

原纹理名 HY\914_1.png 按原 PC 适配路径优先找 tex\HY\914_1.dds，再找 PNG。该原 DDS 为 5616 字节、64×64、DXT5，RGBA CRC32=2635429839；原色 [251,238,0,254]，位置 [20,8,19]，缩放 [0.2,0.2,0.2]，16 三角形。显示不重建网格，DrawCylinder 使用实例矩阵/颜色及原 model.property 选择透明或加法混合。

## DDS 范围

DDS 容器读取支持 BC1/DXT1、BC2/DXT3、BC3/DXT5 和 32 位 RGBA mask；仅解码顶层，拒绝立方体、体积、截断和超预算尺寸，处理非四倍数边缘。块解码使用固定上游提交的 MIT bcdec v0.985，来源和哈希见 third_party/bcdec/PROVENANCE.md；PNG 沿用 LodePNG。

Pillow 独立解码与本实现的 16384 分量中 14245 相同，2139 个 RGB 分量本实现高 1，4096 个 alpha 全同。报告 prepared-data/914_1-dds-decoder-comparison.json 保留差异；这不证明原 SDK 数值完全等价。空纹理和 XML 文本转数值、非空 world 挂载仍明确失败，避免伪造支持。

## 验证

主机 51/51 通过：DDS 格式/块边缘/错误输入，纹理缓存与失败重试，实例计数/构造回滚，变换、颜色、隐藏冻结、定时 alpha 和复位。H077-cylinder-owner-host.json 直接由用户原包生成，原 Effect.cpk 的主机与设备 SHA256 一致。

历史独立实例验证：Android APK BCE4039BD756DCAAACF268626DB7471152FC298B258F08FED97A7F93B38445E2，emulator-5556 进程7803，GL0。verify-effect-cylinder.ps1 核对安装包/纹理/配置/矩阵，显隐、自然淡出、复位、后台冻结及 EGL 恢复；上下文1→5，created 始终1，切Q01后 live=0且进程不变。中央区域黄色像素：visible/reset=203，hidden/faded=0；截图已目视检查。淡出中alpha167，进入后台后132，恢复采样50，没有重置回原色。诊断预览每0.1秒输出状态以观察原短时淡出，其余场景仍每秒输出。

报告位于 H:/PAL4/objs/pal4-android/emulator/cylinder-verification.json 和 cylinder-pixel-verification.json。初次验证遇到状态文件未生成，已改为限时等待；第二次每秒采样漏过淡出窗口，保留失败日志，并提高独立预览采样频率后通过。

## 尚未接通

这只是原 H_077 的一个独立 CylinderObject，已接原Show/Scale/ChangeColor/Hide四行任务的独立投影，见[EFFECT_TASKS.md](EFFECT_TASKS.md)；后续已执行三对象/十任务，见[EFFECT_SEQUENCE.md](EFFECT_SEQUENCE.md)，原Dialog/SceneObjects/GobEffect生命周期仍未接。H_077未引用Clump模板；Q01 其他环境效果需要嵌套模型。完整剧情仍以302C包21句/136调用、停97类型8为基线，不据此放行97。

继续以原 CPK→CSB→执行为主线，暂不实现 CS→CSB。后续接原对象工厂、粒子和任务调度，再验证原97的显隐、逐帧画面及切场/停止生命周期。洞穴、战斗、章节出口和正式无资产发布仍未完成。
