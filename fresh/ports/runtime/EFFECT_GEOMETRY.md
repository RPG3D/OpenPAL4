# 公共特效几何与GLES绘制

`BuildCylinder`复用原GeoCylinder::Create（IDA643A60），将其几何/材质/Atomic/Frame接口边界转换为公共Scene。CPU几何和纹理归返回值所有；GLES资源由GlesRenderer持有。实例变换与颜色使用model矩阵及DrawAppearance，不必重建网格，Android/Switch共用。

## 原几何

保留plain/bordered、交替UV、居中高度、弧角、上下半径和边框宽度。纹理存在时使用原0.5/宽高的半像素偏移。每个扇区原四顶点/两三角形，上传时按原(0,1,2)/(1,3,2)展开，保留白色顶点色与原材质RGBA=0xfeffffff（alpha254）。不将这个RGBA值误解为裁剪标志。

公共句柄只记录有检查的几何/材质/根帧关系，不存宿主指针，不模拟SDK对象布局。中间构建引用释放后，最终Scene仍拥有顶点及纹理；异常由容器回收。几何预算3..4096边，拒绝非有限参数、空尺寸/错误长度纹理。范围基于实际生成顶点。

原RpClumpAddAtomic使枚举顺序反转；独立Win32 SDK null后端创建8个Atomic后，RpClumpForAllAtomics输出[7,6,5,4,3,2,1,0]。公共扇区按此顺序输出。报告`H:/PAL4/objs/pal4-android/cylinder-sdk-oracle/order.json`；该目录保留独立来源和构建文件。只证明公开API枚举顺序，不证明所有SDK数值等价。

## 绘制状态

`DrawEffect`复用同一个顶点/纹理/矩阵提交后段；调用者显式提供混合因子、深度测试和深度写入。特效按资源顺序提交，不套世界透明面排序。支持标准透明和加法等常用混合因子；源alpha饱和因子尚未接入。

绘制强制使用加法混合方程，并在返回时恢复原RGB/alpha混合因子、两条方程、深度写入及blend/depth/cull开关。shader/VAO/纹理绑定仍由各绘制器提交，不承诺保存整套GL上下文。普通DrawScene继续原有透明排序/深度规则。

## 验证

- 主机49/49：普通/带边框、两种UV、居中、半像素偏移、材质alpha、顶点共享/三角形顺序、范围及无效资源预算。
- 原H_077圆柱尺寸的主机夹具：8个Atomic、32个原顶点、16个三角形、48个展开顶点。纹理为2×2测试纹理；未将其称为原HY/914_1.png效果画面。报告prepared-data/H077-cylinder-geometry-host.json。
- GLES实际像素检查：标准透明/加法在蓝底产生不同预期像素；混合/深度/剔除状态恢复；depth_write=false不遮挡随后较远几何；原生成器输出的无纹理圆柱提交16三角形并产生至少100个有效像素。
- Android APK61FAE5667892006E38989A64AAE40206C950B3888EE0E015753587B4B68E56B2，图形进程6690；七个场景共9个快照/上下文1..9，同进程切换和后台恢复通过，GL0。每次新上下文重新执行GPU检查；status的gpu_effect_geometry_contract为true。源码对照6893通过。
- Android双ABI、Switch完整GLES/probe ELF交叉编译通过；本轮未部署Switch设备。

后续已从H_077原配置创建独立CylinderObject，读取原DDS并在安卓验证显隐、淡出、复位和EGL保持，见[EFFECT_CYLINDER.md](EFFECT_CYLINDER.md)。圆柱四任务投影已接原缩放/颜色处理器，见[EFFECT_TASKS.md](EFFECT_TASKS.md)；完整效果播放、粒子和GOB生命周期仍待连接。原SDK数值完全等价、原效果画面和章节验收均未完成。最近完整剧情证据仍为302C的136调用基线，保存在emulator/baselines/through-effect97-302C。
