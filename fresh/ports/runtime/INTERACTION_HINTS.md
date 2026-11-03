# 交互限制提示

原425D60在GOB validfor为1/3且与当前角色索引不匹配时显示GameString265/266，阻止此次物件调用，并播放原WA05。相符角色可正常继续；不修改角色索引、使用次数或剧情变量。

## 文字与窗口

章节宿主从用户 database.cpk 的 Pal4db.db 在内存中读取 GameString 并解码原 CP936；P4GS 转换文件仍供旧版和独立测试使用。声音为 PAL4/gamedata/PALSound/WA05.wav。缺失输入明确报错，不使用写死的中文替代。

布局、九宫格图框、systemBold字体和颜色直接读取原ui.cpk中的hintWindow.xml及图集。425D60传入的1.0是时长，4ABD90将它交给定时关闭队列；旧opacity命名已纠正。InteractionHints按1秒运行时计时关闭，提示不阻断自由移动或要求确认。

54A550按文字高度+1、最大行宽+7调整编辑区及外框，再保持原中心锚点。原重建函数已抽到fresh/src/hint_window_resize.cpp，由旧重建与便携层共用。安卓用字体测量回调，Switch用SDL_ttf测量，均调用同一计算；不使用教程的大窗口。平台字体栅格与原CEGUI逐像素等价尚未证明。

## 输入与验证

教程和对白继续使用原模态门控；提示仅拦截不匹配角色的物件回调。短按保留、释放清理、切场重置与关闭模态窗口防误触已接章节循环。

主机四项定向合同覆盖提示时长、过期关闭、错误角色门控、音频队列不污染脚本音效句柄、教程回归。转换工具四项测试通过。文字输入与原database.cpk SHA256相同；用户缓存和WA05已部署安卓并读回核验。

安卓独立预览只检验原文字与窗口显示，game_calls=0，不执行取弓。真实剧情、实际提示音播放、Switch实机交互仍待验收。生成的文字、字体、图集、声音不进入公开运行时包；启动时自动生成/失效检查尚未完成。

转换命令：

```
py -3 fresh/ports/tools/prepare_android_tutorial.py 用户解包目录/database/Pal4db.db 用户缓存/data/interaction-hints.p4t --table GameString --keys 265 266
```

## 获得物品提示

已扩展原HintWindow显示路径，供98/99物品服务使用原GameString251/252；物品名颜色与数量来自同一格式化结果，字符颜色范围由两平台共同消费。普通限制提示265/266行为保留，详见[背包](INVENTORY.md)。
