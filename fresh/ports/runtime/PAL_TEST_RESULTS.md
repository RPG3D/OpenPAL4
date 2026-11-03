# 测试结果读取

Android/Switch 共用 `SettingsServices` 接入服务220/`giGetPalTestResult`，复用既有 `ScriptPalTestResult::Get`。数值句柄只适配原接口，不截断64位指针。

## 原版证据

- IDA `0x5D86E0`：索引0/1/2分别读取全局对象 `+0x1c/+0x20/+0x24`；其他索引直接返回0。
- `0x5CE0A0`：重置时三槽清零。`0x5CDAA0` 先重置，再从用户 `pal4.set` 读取。
- `0x5CD570`：先写数量5，再写 `+0x1c` 起五个word。这里的5是字段数量，不能仅据此称作设置格式版本。

每槽保留完整uint32位模式；脚本接口按int32解释。Q01奖励分支比较等于1，不把其他非零值转换成1。查询不保存文件，也不修改剧情变量或背包。

## 便携设置

P4ST 当前为v4共64字节，前三个结果word仍在偏移16～27；偏移28～59追加独立帮助已读位，偏移60保存CRC32。兼容v1/v2/v3读取，缺少的结果槽和帮助位初始化0；下次保存写v4。旧运行时不能读取v4。保留已有临时文件/备份恢复策略，完整布局见[设置格式](SETTINGS.md)。

当前未实现原 `pal4.set` 导入、答题窗口和答题完成写入；不能宣称原设置兼容或答题功能完成。便携结果槽可持久化，教程开关和影片位保存时不会丢失。新游戏前后的完整原版生命周期仍待验证。

## 验证边界

`pal4_portable_settings_contract`检查独立zlib校验样本、v1/v2/v3迁移、任意word往返、无效索引、实时状态变更和查询不写盘；合成短脚本验证值为1与2的分支区别。`pal4_portable_tutorial_flow_contract`验证教程保存使用v4后仍保留设置、帮助已读位及等待语义。

短脚本只用于合同测试，不进入发布包、不替换原CSB。真实取弓奖励、石沉溪洞战斗与离峰均未验收。

本次Android双ABI和Switch交叉编译通过。安卓x86_64原生settings_contract已在emulator-5556执行通过；最新APK与读回核验记录位于 `H:/PAL4/objs/pal4-android/emulator/baselines/pal-test-9ad3d0d3`，应用只恢复入口，未开始新游戏。
