# giCameraGetPitch 实际脚本查询

2026-10-31，M08/2 ctrl02的func7003在giPlayerLock后调用giCameraGetPitch，
native以缺失回调-63退出。此前第一处烛台func7002不调用此查询，因此通过第一处
不能覆盖第二处。失败输入687231718、触发前1805186177、原版后续只读快照
m08-candle2-original-after-failure13.json保留，失败不计双端机关验收。

库归属门5D30C0为OWNERSHIP_REVIEW_REQUIRED。现场IDA确认：初始化/取得相机系统，
取得当前活动相机内部数据，再读取其+64的float并作为浮点返回。没有按名固定
MainCamera、零值替代、角度归一化、单位换算或写入。已将泛名Camera_GetProperty_3
改为giCameraGetPitch_ReadActiveCameraAngle64。

复用既有ScriptCameraHandler::Pitch，抽出只读IScriptCameraQueryRuntime三方法接口，
既有IScriptCameraRuntime继承查询接口，IScriptCameraCallbackRuntime只转接已有
CameraSubsystem/ActiveCameraInternalId/ReadFloat，不重复相机查询算法。原生端继续借用
NativeScriptCameraRuntime及NativeCameraManagerOwner。SDK的-63使用SetReturnFloat，
缺失camera服务仍显式报告system ID，不加入静默stub或另一套相机状态。

共享测试检查当前相机三步调用顺序、精确+64、负角/正零/负零/接近360不归一化。
真实AngelScript编译执行float返回函数，验证负数/负零/359.5的float32返回位值及
缺失服务异常。core1/1、SDK1/1通过，app-build2成功；build1因新测试缺少stdexcept
头编译失败，已补齐并保留日志。证据H:/PAL4/objs/camera-get-pitch-20261031-*。
最终只读facet版本core2/2、SDK1/1及app-build3成功。实际重放从一份燧石的
native近档冷读开始1393818519，经真实出口进入M08/2，再Space触发ctrl02/func7003。
终点463742346：完整库存3121从1到0，48全局/钱/完整MT、七组位姿镜头float32、
两烛台及door01/02已查位姿/任务/根LTM对应，wall01/02均inactive。
初版接线的重放14也通过256404104；最终17确认复用共享查询后行为保留。

两处完成近档`m08-floor2-candles12-global50600-20261031`两writer均472808字节，
每来源均双端冷读/实际重入验证机关持久化（native15/52020531、original16/1729443379）。
最终17继续真实func1005进入M08/3，m08-floor3-final17-verdict.json核对人物/全部已查
镜头float32、48全局、钱7451、完整背包、完整MT和游戏提示“千佛塔四、五层”均对应。
最终两游戏正常关闭，共享slot0恢复上述original来源。新近档实际在一层save2，
后续只需真实出口跨到待测场景，不重打掉落或烛台。

以上是限定字段/稳定终点数值验收，受控17ms时钟诊断不等于真实调度等价；
完整帧/CRT/像素、全部存档字段和RNG持久化仍开放。
