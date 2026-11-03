# 原版读档页移植进度

## 已实现

`cegui041_probe` 为原布局注册了 `OiramLook/StaticText`。窗口继承 CEGUI 0.4.1 `StaticText`，保留 `Static`/`StaticText` 类型关系、背景与文字颜色、对齐、字体和文本绘制属性，并按原命名建立水平/垂直滚动条子窗口。滚动值和滚动位置沿用 CEGUI 0.4.1 状态与事件。

读档列表现在使用 CEGUI 0.4.1 `MultiColumnList` 数据模型；独立 `OriginalListHeader` 与 segment 工厂提供其抽象子控件，列表工厂建立自动列头和双滚动条。布局的两项 `ColumnHeader` 属性负责创建两个原始列段，`ItemHeightExtent` 作为兼容属性保留；`VScrollVisible`/`HScrollVisible` 映射到 CEGUI 的强制滚动条可见状态。`MenuButton` 现在绘制原按钮图像和标签文字，列头 segment 使用当前 System 字体左对齐绘制列名。滚动条根据页/文档尺寸计算 thumb 位置与尺寸，读取列表上传的 OiramLook 按钮、轨道和竖直 thumb 图像属性绘制。

剩余视觉/输入限制：`MultiColumnList::cacheListboxBaseImagery` 为空，未实现选择刷、列头背景/分隔线/排序图标、横向滑块皮肤和 thumb hover 状态。`updateThumb` 现在只实现布局几何与取值映射，触摸拖动、箭头/轨道点击、滚轮还没有端到端验证。列表没有真实行或单元格数据。

## 模拟器验证

Android x86_64 `pal4_cegui041_gles_probe` 的 `PAL4_CEGUI_LOAD_PAGE_ONLY=1` 分支打开 `loadWindow.xml`，检查 `StaticMask` 类型关系、`List` 两列/零行、列段和滚动条树，再执行 GLES 绘制及同一 session 的关闭、重新打开和清理。仅供同源截图比较的实验适配应用 `MinimumSegmentPixelWidth`，将列段设置为 `20×22`/`337×22` 并重叠在共同起点，得到水平条不可见和 `pixels=390704`。这些列段状态尚未纳入正式 CEGUI session，也未确认是原 DLL 的通用布局机制。原始未适配模型的几何仍为列表 `636×394`、header 相对 `(2,2)`、`600×22`、两列各337px和水平条可见。没有安装 APK，也没有修改设备游戏资源。

验证加载了 `imagesets/biaoti_0.imageset`、`imagesets/OiramLook.imageset`、`gy_0`、`jingquedianji_0`、`kuang_0`、`kuang2_0` 与 `MultiList_0`。首轮缺图是隔离探针漏载 `biaoti0`/`OiramLook` 图集，并非设备包缺少资源；补齐后严格 image-property 检查通过。设备 `/sdcard/Android/data/org.pal4.port/files/PAL4/gamedata/ui.cpk` 与 `I:\Games\original\gamedata\ui.cpk.original` 长度均为 92,405,369 字节，SHA256 均为 `a81ae971616af98dd8550100bc9607fd2f4238a097f2ea1db55f24754bee7522`。

PC 原 DLL `PAL4_CEGUI_LOAD_TREE=1` 同源记录显示两个列段的尺寸为 `20×22` 和 `337×22`，相对位置均为 `(0,0)`；水平条 `DocumentSize=357`、`PageSize=602`、`Visible=False`。目前只证明了 PC 最终窗口树状态，未证明其形成原因。独立截图比较 probe 使用 `ListHeader::MinimumSegmentPixelWidth` 复现这些列宽/起点并让横条按文档/页面尺寸隐藏；该适配标记为实验结果，尚未通过原 DLL 调用链验收，不能作为正式 session 行为依据。真实游戏列表初始化还会清除 XML 样例列并加入空标题存档列。

最新可见项修改已在严格 Android probe 运行。实际 800×600 渲染输出：

- RGBA：`H:\PAL4\objs\pal4-android\emulator\cegui-load-page-android-parity-20261029.rgba`，top-down RGBA，1,920,000 字节。
- PNG：`H:\PAL4\objs\pal4-android\emulator\cegui-load-page-android-parity-20261029.png`。
- 实验性列段适配后，画面与 PC 同源静态布局相同，显示左对齐 `Column2` 标题、槽位按钮文字和右侧垂直条；`Column1` 被覆盖且水平条隐藏。此像素结果不代表正式 session 已采用该列段状态，也没有存档数据行。

PC 原 DLL 与实验性 Android 静态图片使用相同 UI CPK SHA256 `a81ae971616af98dd8550100bc9607fd2f4238a097f2ea1db55f24754bee7522`。实验适配后的逐像素比较：全帧 RGB 不同 `4,770/480,000`，最大通道差255，MAE0.664835；列表框区域 `(80,55)-(718,496)` 不同 `1,579/281,358`，列表内部 `(90,96)-(685,486)` 不同 `371/232,050`；标题 `(625,0)-(800,60)` 不同0；底部按钮区域 `(80,505)-(800,600)` 不同 `1,980/68,400`。该数值仅描述静态 probe 的实验渲染，不代表正式列表行为或存档页验收。PC 图是无存档初始化的静态布局参考。

构建与隔离运行命令：

```powershell
$adb = 'H:\PAL4\Tools\android\sdk\platform-tools\adb.exe'
$probe = 'H:\PAL4\objs\pal4-android\cegui041-x86_64\pal4_cegui041_gles_probe'
$remote = '/data/local/tmp/cegui-load-page-parity-20261029'
$deviceRgba = '/data/local/tmp/cegui-load-page-parity-20261029.rgba'
$capture = 'H:\PAL4\objs\pal4-android\emulator\cegui-load-page-android-parity-20261029.rgba'
cmake --build H:\PAL4\objs\pal4-android\cegui041-x86_64 --target pal4_cegui041_gles_probe
& $adb -s emulator-5558 push $probe $remote
& $adb -s emulator-5558 shell chmod 755 $remote
& $adb -s emulator-5558 shell "PAL4_CEGUI_LOAD_PAGE_ONLY=1 PAL4_CEGUI_LOAD_PAGE_RGBA=$deviceRgba $remote /sdcard/Android/data/org.pal4.port/files/PAL4/gamedata/ui.cpk /dev/null"
& $adb -s emulator-5558 pull $deviceRgba $capture
& $adb -s emulator-5558 shell rm -f $remote $deviceRgba
python fresh/ports/tools/cegui041_probe/compare_load_page_capture.py `
  H:/PAL4/objs/pal4-android/emulator/cegui-load-page-pc-same-source-20261029.rgba `
  $capture --region 80 55 718 496
```

当前 probe 和临时 RGBA 已从设备清理；上述命令可复现最新严格运行。

## 未完成

- OiramLook 的选择刷、列头背景/分隔线/排序图标和横向滚动条皮肤；列头排序、滚动条拖动、点击及滚轮行为仍需完成/验证。
- 存档枚举与真实行/单元格内容、选中项和 `BtnLoad0..9` 的点击/载入流程、关闭交互及手柄导航。
- 可见项修正后的 PC 原 DLL 与 Android 像素比较，以及真实存档列表对照。
- 正式菜单到读档页的接线。

当前结果证明 `StaticText` 类型缺口已补，原 `MultiColumnList` 模型/窗口树可由布局创建，并通过实际 Android GLES 绘制与资源清理；不代表读档功能或原版列表外观完成。
