# 王陵音乐初始化的区域判断

live245从双writer双端限定冷读的40400近档推进。真实八公山EVF func2004有高度条件：18次41704–41721对白/18次共同Space全部对应，推石和入口动作已越过；随后M07切图native明确因Music.csb回调-125未实现退出。原版保持响应，随后正常WM_CLOSE关闭，native已自行退出。尚未完成M07开场或40500。失败日志H:/PAL4/objs/compare-rebuilt-20261027-152539.err.log、live245-tomb-entry-failure-verdict.json。

原版实时EVF和16次有界W证据live245-func2004-vertices.json、live245-tomb-entry-crossing-input.json：地面/镜头投影移动有位置差，未计移动一致性验收。授权同位定位(2750,950,-1800)、共同W后双方自然触发func2004；不写故事/HP/RNG/时钟/完成字段。

所有权门禁5D57C0为游戏函数需核对，73D110明确VC6 libc __stricmp，复用宿主CRT。当前IDA5D57C0确认giBGMConfigIsInArea读取GameArenaManager+E0的场景名，参数+4空指针回退空串，忽略大小写相等即true；Music.cs M07_MUSIC的区域9选P04.mp3，其他选P09.mp3。+E0是既有SceneName()，不是LoadedAreaName()的+0C。

复用已有ScriptBgmCallbacks::IsInArea，抽出只包含该查询所需四方法的IScriptBgmAreaRuntime，旧IScriptBgmRuntime继承它，原共享算法保持不变。NativeGameScriptCallbacks的-125通过局部字符串token投影适配AngelScript string与共享legacy查询，不假设SDK字符串内存布局；比较直接调用_stricmp，不重写CRT。原生直接借用现有arena owner的IScriptBgmAreaName接口，每次读取其SceneName()，不复制/缓存过时场景名。Music初始化、主脚本、事件及定时上下文均绑定同一owner；Music上下文结束/异常仍清除services，音乐player和stream owner不变。用户脚本检索证实M07/M06等场景脚本也调用该查询，因此不能只在Music.csb临时实现。

实际SDK回归增加缺服务异常、大小写匹配、区域变化不匹配及空串；focused1 1/1(.03秒)通过。完整应用资源夹具增加真实原始M07_MUSIC两分支(P04/P09)，只在夹具末尾恢复原saved track。build1及完整resource1通过GUI13462/13462；扩展到场景/事件/定时上下文后build2通过，focused2 1/1(.04秒)、完整resource2通过GUI13214/13214正常shutdown；实际切图live246复验进行中，未据focused通过宣称流程完成。

可靠近档仍H:/PAL4/objs/checkpoints/m05-guard-global40400-20261027-live243；不重跑func2003的69句。M07开场38次41722–41759源脚本直接取用户指定GBK目录。

## live246 修复后实际切图

build2、focused2及完整resource2后，从相同original40400近档重新启动，静音800×600副屏DISPLAY1。授权同位定位(2750,950,-1800)和共同W进入func2004；18句41704–41721重新自然通过，实际越过原Music.csb -125退出点，双方已进入M07/1并显示41722及后续对白，仍按自然打字/分页边界共同Space。完整入口/开场已自然到40500，见下方；全游戏仍未完成。journal为H:/PAL4/objs/live246-tomb-entry-dialogue-journal.json，合并源函数为live246-tomb-entry-sources.txt。

## live246 入口和开场自然完成

M05 func2004共18句41704–41721，M07 func2001共38句41722–41759，56次源调用按用户GBK源序/文字/编号全部通过，共57次相同Space；41729有一次两端实际自然分页。live246-tomb-entry-story-verdict.json确认自然global40500/Scene/control1、位姿/全部镜头float32 bits/fullMT624对应，原-125实际失败点已越过。未写故事/HP/RNG/时钟/打字完成字段。

将两端授权定位到原版只读save1附近(-390,.8972,1710)，共同Space打开真实存档点，slot0和覆盖确认分别串行写两writer并立即归档。原版340801字节/native340797字节，均global40500/header248/队伍1110，4字节长度差未解释。归档H:/PAL4/objs/checkpoints/m07-entry-global40500-20261027-live246；不能称全归档一致。246正常WM_CLOSE结束。

live247加载native来源档：位置/方向/eye/target/angles/requestedDistance/fullMT相同，但currentDistance原115.29093170166016(42e694f5)、native115.29093933105469(42e694f6)，稳定重采仍差1ULP，严格cold verifier失败保留，没有放宽容差。暂停System只读MainCamera数据（原manager+34，排除System+30）证实全部四条segment相同，选中collision point Y先差1ULP，relative Y随之不同，再产生distance差；具体覆盖/计算来源未证明，不能归因为平方根或SDK。原5EC270→5EBC80→661A50及5FEA10已重查；真实RenderWare三角遍历继续调用SDK，未复制库算法。证据live247-paired-camera-raw.json、live247-tomb-cold-partial-verdict.json。247正常关闭；248换original来源档，复现相同115.2909317/115.2909393差1ULP，其他位姿/镜头字段/fullMT对应。两种来源均可双端加载，但严格cameraDistance验收均失败，未修改检查。不能称两writer完整cold相等；可靠已完整限定字段验证近档仍40400，40500为可加载但精度验收未闭合候选。

下一主线为M07/1向北及后续区域/机关；不走迷宫，按原EVF/脚本授权定位至真实触发区。镜头碰撞点差、4字节存档长度差、完整时序/像素、完整存档字段/RNG持久化及全游戏十二模块仍开放。

## 后续缓存证据澄清

live248/249后续诊断未得到有效的同输入命中记录。四条相同segment只是事后缓存，不证明计算所选碰撞点时输入相同；原缓存point无法由当前缓存segment的严格single插值重现。因此撤回任何把缓存point Y差当作已定位数值计算根因的解释，实际残差仍开放；原冷读失败结果保持。调试实验有进程被中断/强制清理，均不计正常shutdown或产品故障，详见[m07_camera_cached_probe_native.md](../renderware/m07_camera_cached_probe_native.md)。live250正常从40500近档继续原主线，无需重跑已完成入口/开场。
