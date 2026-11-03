# CEGUI 读档页视觉对照

## 捕获版本与状态

- PC 使用原 DLL `I:/Games/original/CEGUIBase.dll`（SHA-256 `24f48dc0a94e6ca29e38cdc204e5084d36a08edf5622ac33095cdc4a31aed2d8`）与 `I:/Games/original/OIRAMLOOK.dll`（SHA-256 `a4f903680ede5bde6efe9dc714eea4c85cfd4d5451eb6d12f7aac5358ea89a60`）。探针 `pal4_cegui_system_probe.exe` SHA-256 `8757dba9a98ec554657edefef563ccf00601c0264f52bf00eea99c37e2474c52`。
- PC 资源入口是 `I:/Games/original/gamedata/ui.cpk`。Android 新页探针读取 `I:/Games/original/gamedata/ui.cpk.original`，92,405,369 字节；两个文件 SHA-256 均为 `a81ae971616af98dd8550100bc9607fd2f4238a097f2ea1db55f24754bee7522`，本轮已逐文件核实同源资源。
- PC 可复查命令：设置 `PAL4_CEGUI_MENU_LOAD_PAGE=1`、`PAL4_CEGUI_MENU_RGBA=H:/PAL4/objs/pal4-android/emulator/cegui-load-page-pc-same-source-20261029.rgba`、`PAL4_CEGUI_MENU_FONT_ROOT=H:/PAL4/objs/pal4-android/emulator/font-oracle` 后运行 `pal4_cegui_system_probe.exe I:/Games/original image_widget`。退出码 0，日志确认 99 个四边形、391397 个可见像素、纹理释放，并记录桌面焦点和光标保持。字体根中 `simsun.ttc` 通过探针与原 CPK 字节比较；另一解包字体目录不匹配原包，已被探针以退出码 58 拒绝，未用于截图。
- PC RGBA：`H:/PAL4/objs/pal4-android/emulator/cegui-load-page-pc-same-source-20261029.rgba`。Android RGBA：`H:/PAL4/objs/pal4-android/emulator/cegui-load-page-android-20261029.rgba`，设备端 `PAL4_CEGUI_LOAD_PAGE_ONLY=1`，输出 390937 个可见像素。两帧均为 800×600 top-down RGBA、1,920,000 字节。PC 帧 SHA-256 `9c9ea1da3abac1963b30de2deb22946b40d068c7b4b96d01f8b675b8c2e9b0c5`；Android 帧 SHA-256 `2a91ba92aa05c1f4fde13a976caf05bfb02891168c5cf2c4035e78bc21fb352e`。
- 页面状态是原始 `loadWindow.xml` 布局，没有运行游戏存档列表初始化。两端都不是已填充真实存档的读档列表。PC 仍绘制 XML 示例列文字和右侧滚动条；Android 显示同一背景、标题和空槽位，但截图中未绘制该列文字与右侧滚动条。PC SDK 左下角 RW 标记也在完整画面对比范围内，不属于游戏 CEGUI 页面。
- PC 原 DLL 布局日志的 `List` 相对位置为 `(0,36)`、尺寸 `636×394`，列表含两个表头段；`ListHeader` 相对位置 `(2,2)`、尺寸 `600×22`，段宽分别为 20 和 337 像素。垂直滚动条相对位置 `(604,0)`、尺寸 `32×392`、可见，`DocumentSize=0`；水平滚动条相对位置 `(0,362)`、尺寸 `604×32`、隐藏。探针绘制的是布局加载后的静态模型，不是存档初始化后的列表。

## 比较器

`fresh/ports/tools/cegui041_probe/compare_load_page_capture.py` 对两份 800×600 RGBA 原始帧报告整帧 RGB 完全相同像素数、RGB 差异数、最大通道差、平均通道绝对差和 Alpha 差异数；可另给一个半开区间 ROI。脚本只报告数据，不定义通过阈值。

```powershell
python fresh/ports/tools/cegui041_probe/compare_load_page_capture.py `
  H:/PAL4/objs/pal4-android/emulator/cegui-load-page-pc.rgba `
  H:/PAL4/objs/pal4-android/emulator/cegui-load-page-android.rgba `
  --region 80 55 718 496
```

PC 帧自比检查通过，整帧和 `(80,55)-(718,496)` 均为零差异。有效 PC/Android 比较的结果如下；RGB 差异是不同像素数，MAE 是每个 RGB 通道的平均绝对差，不把 alpha 通道混入 RGB 指标。

| 区域 | 像素 | RGB 不同 | 最大 RGB 通道差 | RGB 通道 MAE | Alpha 不同 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 整帧 `(0,0)-(800,600)` | 480000 | 15260 | 255 | 3.276758 | 3210 |
| 主列表框 `(80,55)-(718,496)` | 281358 | 12546 | 255 | 4.716764 | 1237 |
| 列表内部 `(90,96)-(685,486)` | 232050 | 1036 | 246 | 0.317633 | 183 |
| 标题 `(625,0)-(800,60)` | 10500 | 33 | 1 | 0.001079 | 0 |
| 底部控件 `(80,505)-(800,600)` | 68400 | 1385 | 247 | 2.090195 | 1245 |

整帧中 464740/480000 像素 RGB 完全一致。主列表框的差异主要反映当前两端原始布局绘制状态不同；内部大面积填充区域较接近，标题也几乎一致。极大通道差出现在局部差异像素，不能据此推断大范围颜色失真或整体失败。

## 覆盖边界

本次证据只覆盖同资源包、同分辨率下原 DLL 与 Android 隔离探针的一帧静态原始布局。它不覆盖真实存档行/列内容、槽位标签、滚动条交互、选择/悬停/按下状态、存档确认或读取流程、屏幕缩放以及动态游戏场景；像素差异也未做配准或颜色管理归一化。当前结果不能声称同页逐像素一致或功能等价。
