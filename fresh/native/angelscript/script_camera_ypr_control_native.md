# 活动相机增量YPR控制

## 原始分叉与静态依据

pair38在M08/4 ctrlB/func7003完成第四灯镜头收尾时，原生报callback -58 not implemented后退出。
本轮之前两场正常战斗分别6/20条玩家指令通过稳定数值门限，各掉2个3121，正常写档保留
第一灯+4份燧石；第二/第三灯分别1089052231/1174743641通过，未把第四灯失败计为完成。
日志compare-rebuilt-20261031-064433.err.log与m08-floor4-candle4-38-journal.json保留。

先过ownership gate，再现场IDA复核5D3000/5EACE0：PAL4回调取得当前活动相机对象，
三轴参数是增量，低8位option传给已有SetAllAnglesAndUpdateMatrix。不是绝对赋值，
也不固定MainCamera。公开RenderWare/现有矩阵服务仍承担底层更新，无新私有库算法。
实际SDK注册表中的-58是giCameraCtrlYPR；同脚本的BGMConfigIsInArea是已接入的-125。

## 接线与共用

NativeGameScriptCallbacks -58通过已有camera服务转交NativeScriptCameraRuntime；
该服务读取当前活动对象，再交创建它的NativeCameraManagerOwner处理。新增共享
CameraParameterSetters::AdjustAngles，原CameraSystemObject::AdjustAngles和新原生适配
共同调用，保留逐轴float存储、既有规范化与矩阵更新。旧鼠标yaw/pitch路径保留原roll
不写行为，避免凭新增YPR接口改变旧输入语义。无缓存角度或硬编码场景/镜头。

## 验证

实际AngelScript SDK脚本覆盖三个非零/负浮点增量、true/false ABI及缺失服务拒绝；
共享参数/相机对象/回调3/3通过。原生资源fixture选中ScriptCamera，再通过真实runtime
调整yaw/pitch/roll并核对MainCamera未变。SDK callback1/1、原资源package1/1及共享3/3
通过；最终应用构建成功、完整resource1退出0/GUI12812对应/静音452次/动态1600×900副屏。

新pair39从第一灯+4份燧石近档直接冷恢复，不重打战斗。第二/三/四灯请求分别
278803208/1720728518/993717637，均通过稳定七组位姿/镜头float32、party/全局/钱/
fullMT/库存以及六机关所查属性和根变换；剩余3121为1，两door uses0、两wall active0。
正常返回一层save2，双writer各483065字节归档m08-floor4-four-candles-global50600-20261031；
原hash2589fa37fdd4fcbe95429d22cbb7f288c323fb51c48ddad7815cb63aab892dbf，
原生hash9e400c20c0ca08db658337fa5b0acd96685e6832e737c64de5fd7a7e88d3b417。
冷恢复结果见该检查点provenance.json与M08入口专题。

生成产物统一H:/PAL4/objs/m08-camera-ypr-20261031-*。诊断仍为17ms固定时间输入，
不据局部验收声称默认实时、完整相机模块、完整CRT/ATB或整个游戏等价。
