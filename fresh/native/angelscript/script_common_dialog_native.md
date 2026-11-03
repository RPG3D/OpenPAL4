# 原生 CommonDialog 脚本接线（2026-10-30）

## 前沿及证据

50300近档进入Q04/CN03，掌柜func5005选择住宿后在-317显式停止。归属门对5E8D00、5D8980、5E7A30、4AAA70返回OWNERSHIP_REVIEW_REQUIRED；现场IDA确认：-317/-306分别向同一个CommonDialog显示模式3/4，随后排入32字节、vtable849648、mode1、queue+54等待；轮询窗口隐藏后解除等待，保持Scene。-229返回最后选择是否等于1。IDA已纠正旧误名为Script_ShowCommonDialogInSelectMode、Script_CommonDialogGetLastSelect和Script_ShowCommonDialog。

## 模块与库复用

回调复用ScriptDialogs及ScriptCommonDialogSelection。NativeScriptCommonDialogRuntime只适配缓存窗口和实际SDK上下文，每个主/事件/并发/timed上下文持有独立等待记录，由opening wait聚合。SelectDialog及其状态10/XML操作明确不由此适配器提供，不伪造未实现功能。

应用借用现有NativeCommonDialogCache，与Combat/System共享完整布局和控制器，复用原始CommonDialog.xml、MessageBoxDialog和CEGUI DLL。工厂查询按原版4BF610释放旧选择回调；等待清理仅退休自己的SDK等待，不主动关闭其他消费者的共享窗口。UI最终退出时关闭/释放应用持有的窗口。原版4AAA70忽略脚本传入的位置对，实际固定在0.5/0.5居中。

## 数值观察发现的原有偏差

首对原版92296/native98664在实际支付确认处，标题文本、导航目标数2、选择初值2、金钱6826、48全局变量及完整MT一致，但CommonDialog按钮+280标签为原版4/1/2、native0/0/0（pair-common-script-confirm.json，请求312931035）。未把此轮记作完整一致。双方Esc后返回Scene，金额未变，见pair-common-script-confirm-cancel.json。

检查安装的CEGUIBase.dll导出证实：getUserData RVA193F0读取+118h，setUserData RVA194E0写+118h；getID RVA18E90读取+E4h。旧NativeCommonDialogLayout错误地用getID/setID适配共享代码的+280读写。现已改为直接调用库的getUserData/setUserData，并增加实际窗口内存标签检查，避免只凭回调结果相同掩盖差异。

双端数值快照增加只读inventory/money/CommonDialog地址，原版使用现场IDA确认的8EDFDC库存单例、8EDFB0 UI管理器缓存+107*4；不调用惰性构造、不修改库存。pal4_pair_console.py的common-dialog命令按已验证CEGUI String布局读标题、导航数、选择及标签；明确隐藏窗口会保留字段，不以字段相同证明可见性或完整游戏一致。

## 当前验证记录

- common-script-shared-test1的两个目标未构建，保留Not Run失败；补齐目标后shared-test2为3/3通过（ScriptDialogs、CommonDialogSelection、MessageBoxDialog）。
- build1/2成功，smoke1退出0、GUI12620/12620、DISPLAY1静音观测526次。实际Yes/No/OK、Scene等待、同SDK结果及取消通过；此版本尚未包含UserData修复，因此不能替代后续复测。
- build3加入数值观察字段，首对实际确认和取消证据如上。
- build4包含UserData修复、原版工厂回调资源释放及实际字段回归；smoke2在旧组件断言getID时失败，不记通过。修订CommonDialog/YesNo旧断言读取实际UserData，build5完成；完整smoke3退出0、GUI12812/12812、DISPLAY1静音观测533次。新增原生脚本回归和原有CommonDialog/YesNo、Combat/System消费者均通过。

## 实际住宿与50400验收

最终原版82124/native99400从50300近档重测，pair-common-userdata-confirm.json中的原始标题、初值2、导航数2和UserData标签4/1/2对应。双方实际Enter确认，正常PayMoney(300,false)令金钱6826→6526，进入CN03Y，未直接写钱/剧情/选择或RNG。

pair-common-userdata-inn-story-journal.json逐句记录源脚本func2003五句42048–42052及分页状态；全部对应后正常到50400/Q04CY/Scene/control1。结束快照请求1739118546，pair-common-userdata-inn-verdict.json按float32重新打包核对位置、人物方向、摄像机eye/target/angles/distance全部bits相同；48全局变量、金钱6526、完整MT624对应。旧取消路径的微小yaw残差未据此宣布普遍修复。

实际F7和Enter打开覆盖确认时，共享CommonDialog的标题、初值、导航及UserData再次对应。两个正常writer的确认输入串行发送，防止共享测试存档目录相互覆盖；分别归档到checkpoints/q04-night-global50400-20261030-common-dialog/original及native。均460829字节、global50400/arena51；SHA256分别da7f72beee950935e6810bac7a1ca5cceda1ffc4a56365e762f7b5d51cd4076c、da0c2c58298c937a64c67e997861ab895faa2a5c169c0c999b368d0b0ecd8e35。哈希不同，不声称完整字段或字节相等。

三层Esc后双方正常返回Scene/control1、金钱/48全局/完整MT对应（请求1639686941）。随后分别以native来源和original来源正常冷启动slot0，不覆盖位置，两次双端均到50400/Q04CY/Scene/control1，金钱6526、48全局/完整MT、上述位姿/镜头float32 bits对应；请求分别326179674和763007936，证据pair-inn-night-cold-native.json、pair-inn-night-cold-original.json，检查点provenance已记录。共享slot0当前为original来源。

后续主线已沿实际夜间渡口func2004及M08入场推进到50402，期间修复Machine初始显示/加载所有权，并从真实save2建立新的双writer检查点；两来源均已双端限定冷恢复。最新证据见[Machine初始显示与GOB加载所有权](../renderware/machine_initial_visibility_native.md)。当前证据仅涵盖受控17ms和有限字段冷恢复，默认实时、完整存档字段及全流程仍未验收。
