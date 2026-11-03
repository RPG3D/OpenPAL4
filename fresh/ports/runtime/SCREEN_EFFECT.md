# 屏幕淡入淡出

`ScreenEffectServices` 已实现六个原脚本接口：黑、白、红的淡入与淡出。它处理颜色、保持遮罩、阻塞等待和完成通知；GLES 负责最终全屏混合。两层均可供 Switch 复用。

## 共享规则

`fresh/include/pal4/screen_fade_state.h` 提取原 `PalScreenEffect::StartFade` 和 `EffectRenderControl::AdvanceFade` 的值操作。PC 和跨平台运行时调用同一份代码，原 PC 对象布局及纹理门控不变。

- 启动时保留当前透明度；按原始起止范围插值。
- 正在进行的效果不被新命令覆盖。
- 结束后是否保留遮罩由 hold 参数决定；淡入结束移除遮罩。
- 红色使用 (229,10,10)，淡入红的中间布尔参数不参与效果。
- 非阻塞命令立即继续；阻塞命令等效果结束后完成对应请求。
- 零/负时长按已有 PC 适配器规则立即设置终值，到下一次更新再释放阻塞等待，避免除零。

安卓预览仍将活动帧间隔限制为50ms；这是当前主机时钟策略，不宣称原游戏时间完全等价。切后台期间不推进，EGL 重建保留 CPU 效果和脚本位置。

## GLES 绘制

`GlesOverlay` 在场景和角色绘制后提交全屏三角形。0～255透明度先按原绘制路径截为整数，再使用源透明度混合。绘制不写深度，并恢复程序、VAO、深度、混合、裁剪及剔除状态。

每次创建上下文时，GPU 检查验证半透明红色覆盖蓝色、纯黑遮罩及状态恢复。上下文丢失调用 `Abandon`，随后重新创建程序和 VAO。

## 实际新游戏边界

开发命令会执行资源中的真实 `NewGame`，没有跳过其调用：

```powershell
pwsh -File fresh/ports/android/android-dev.ps1 run -Device emulator-5556 -Scene level_N01 -Animation new_game
pwsh -File fresh/ports/android/verify-screen-effects.ps1 -Device emulator-5556
```

已验证：首个 `giFlashOutBlack(0.1,true,true)` 挂起脚本；中间 GPU 像素逐渐变黑；结束像素为黑色后恢复同一上下文。目前随后会确认空闲音乐暂停，并进入真实视频等待。

`game-frames.jsonl` 记录帧号、上下文代数、脚本状态、调用次数、透明度和覆盖前后中心像素；报告为 `emulator/screen-effect-verification.json`。验证还覆盖 Home/恢复后保持黑屏且不重复调用、切回资源预览后取消执行并移除遮罩。

本项只验证淡出与取消/恢复。两段视频的完整自然播放另见[媒体报告说明](MEDIA.md)；NewGame 数据重置、真实场景加载及完整章节流程仍未完成。
