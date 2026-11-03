# 主菜单 3D 背景进度

历史状态（2026-10-29，后续更新见末尾）：主菜单 `ZJM` 背景由 portable `ReadUiWorld` 从调用方提供的 UI CPK 成员读取器解码，读取 BSP、透明 DFF 和贴图均不需要落盘。原版包包含 `uiWorld\\zjm\\ZJM_trans.uva`，但 portable 菜单不再自动解析该字典。

此前的解析器把每帧 `prevFrame` 字段当作数组索引，并用两帧特例推断节点数。RenderWare SDK 将该成员定义为 `RpUVAnimKeyFrame*`；原始 `.uva` 中 `4287668740` 不能据此解释为数组下标。当前没有证据说明该流字段如何重定位，也没有证明帧到节点的分组规则，因此删除了猜测型解析器及 `ReadUiWorld` 的自动读取。菜单 BSP/DFF 仍可独立加载，未声称 UV 动画已播放。

重新接入前需要对原版 SDK 实际读出的动画结构与字节流建立可核对映射，并对主菜单材质完成 PC/portable 同时间片 UV 变换比对；在此之前，UVA 保持未接入，不得因成员存在就阻塞共享菜单。

主机回归：Debug `pal4_bsp_scene_tests` 在无参数契约路径及原版 Steam、原版配音和 Switch stage 三份 `ui.cpk` 上通过。三份原包均得到 ZJM BSP 17640 顶点、58 draws、7 贴图，以及透明 DFF 36 顶点、1 draw、1 贴图；测试同时断言 `ReadUiWorld` 没有请求 `.uva`。Android 模拟器画面验证由共享设备轮次负责，本次没有启动或重置 AVD，也没有向设备安装 APK。

透明排序方面，PC 原版调用顺序和 SDK 透明队列路径已有 IDA 与原 CPK GPU 夹具证据，见 `fresh/native/renderware/ui_world_prerender_native.md`。该证据验证的是 native RenderWare 路径，不代表 portable GLES 菜单当前已接入相同透明队列。

2026-11-02 production入口已更新：共享ChapterMainMenuUi使用原版GPU确认的4:3相机（near=5、far=45000、view_scale=0.5）和显式CW剔除，保留完整天空材质。Android已显示莲花、台阶、山景。透明DFF按world → ZJM效果 → trans → CEGUI顺序绘制；这不等于所有材质排序都已验收。

原ZJM.eft的4个非dummy粒子对象已通过独立ChapterMenuEffects接入既有EffectPlayback，位置(16,209,-70)、缩放1.2、无限重复。Android冷启动、HOME恢复、Introduction页面打开/关闭均保持单组4对象；Linux菜单至新游戏首句无GL错误。效果随机相位与PC尚未同步，不能声称逐帧一致。Switch仅共享源码，未构建/实机复验。

SDK参考阶段（接线结果见下段）：SDK公共API诊断工具已从原trans/water字典取得1节点、2帧及14个时间片的矩阵结果：trans周期100秒、U=-t/100；water周期约33.333335876秒、U=-t/d、V=t/d。首帧异常前驱指针在SDK结果中仍存在，第二帧指向第0帧，不能将任意流字段当作有效下标。对应工具`../tools/uv_animation_oracle`，记录`H:/PAL4/objs/menu-uva-sdk-validation/`。下一步复用合适的库实现并对照上述矩阵，仍须验证材质绑定和实际云层运动，禁止恢复猜测解析器。视觉记录及每次两帧差异图见`../android/主菜单3D背景修复_20261102.md`。

2026-11-02后续：UVA已通过固定版本MIT librw接入共享CPU适配层。ReadUiWorld默认仍不读取UVA；production菜单显式启用字典读取，从原CPK内存构造、保留材质命名绑定、通过DrawAppearance/GLES双UV通道绘制。未附加water clump。原trans/water字典14个矩阵采样与SDK差0，旋转/缩放/斜切参数样例最大差2.4e-7。CPK绑定、损坏流拒绝、周期/双通道/单位矩阵恢复和P4SC兼容测试通过。安卓已安装并确认实际云层运动及生命周期；共享源码不等于Switch实机通过。实现边界、库补丁和证据见`UV_ANIMATION.md`。同帧视觉、粒子随机相位及完整游戏交互仍未验收。

后续云量定位：保留6个atomic边界、按原版球心距离排序并修正alpha阈值后，U=-0.547913194的同相位对照仍缺大云。核对librw的Geometry流读取后发现DFF前两个索引在文件中为v1/v0，portable环境绘制未换回，剔除了朝向相机的云面。现仅调整渲染索引，碰撞旧索引保持不变；独立合成DFF契约验证标准绕序与碰撞隔离通过。同相位生产菜单离屏图的大云已恢复，均值RGB差从14.851474降至3.520415（含粒子与版本文字差，不能作整页验收）。证据`H:/PAL4/objs/android-original-pair-20261102/menu-cloud-winding-host/`；Android正在复验，此修正不在已发送的Switch0.2.22包中。
