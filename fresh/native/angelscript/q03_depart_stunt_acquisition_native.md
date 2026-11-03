# 柳府门口会合：学习落星式

live238从original来源40100客房档双端冷读。只读EVF确认func1044客房出口及Q03X/func1038触发面，授权同位定位后共同输入进入Q03S/func2011。最初S只沿X移动、A只转镜头；第二次A输入日志覆盖同名第一次日志，第一结果仅工具输出，未作移动一致性验收；后续同W触发实际出口。请求2380311在场景替换期间延迟完成，两端都存活，未因超时重启。

41547–41555九句按用户目录GBK Q03.cs的源序、原版编号/文字hash及双方自然打字翻页边界共同Space通过并逐句GPU采集。下一步giAddProperty(3106,1,true)、giWait(2)后调用giPlayerAddSkill(0,5502,true)，native明确以未实现-145退出；原版继续，当前角色只读已学Stunt为5500/5501/5502。证据H:/PAL4/objs/live238-depart-dialogue-journal.json、live238-original-learned-stunts.json、compare-rebuilt-20261027-134510.err.log。

所有权分类后核对IDA5D5F00及反汇编：先找Player、创建HintWindow、查Stunt，再通过42FC30向player+938h的固定有序数组插入；bool只控制后面的文本/提示/WA17.wav。技能名来自Stunt+Ch，角色名来自Player+9FCh，GameString118按角色名、技能名两个%s顺序格式化。已有共享ScriptPlayerSkillAcquisition::Add完全保留；只补回调及原生资源适配器。

NativeScriptPlayerSkillAcquisition复用NativeMagicWindowRecords（原Stunt字段投影）、FixedSortedValueArray::InsertUnique和角色现有分配器、NativeScriptInventoryChange提供的真实Hint/UI/audio服务。文本沿用GameTextFormat的校验和宿主CRT路径，新增两个字符串参数入口。适配器借用角色/内存/提示服务，先于这些owner释放；记录和临时文本独立拥有。原版磁盘EXE和脚本未改，native不注入。

新增实际SDK+原GameDB回归，检查5502入真实角色集合、重复唯一但提示仍显示、静默分支、未知Stunt、缺Player、Hint创建失败的原早退及提示异常后保留已插入值/清理临时文本。build1及实际SDK+原DB学习/回调两项(.14秒)通过；完整resource1通过GUI13022/13022正常shutdown；live239实际学习及全部会合流程已通过，详见下方终点和冷读证据。

live238原版正常关闭、native此前自行退出。后续live239已从40100完成本段；当前可靠近档改为下述40300，无需重跑休息和会合。

## live239 学习节点实际复验

从同40100近档、真实两处出口共同输入重试，已越过41555，继续41556之后的会合对白。只读双方当前Player+938h固定数组均5500/5501/5502；未写技能完成字段。配对GPU `H:/PAL4/objs/live239-depart-step9-boundary3-2391923.png` 双方均显示“云天河学会落星式”，文字、位置与金色技能名对应。实际学习验收H:/PAL4/objs/live239-learned-stunts-23950001.json；全部会合对白及40300终点随后完成，见下方。

## live239–241 完整会合及40300存档

live239自然完成func2011全部89次对白调用41547–41635，共91次共同Space；41584和41605各有一次实际自然分页，未强写打字完成/故事/HP/RNG/时钟。用户指定目录GBK Q03.cs源序与文字逐项对应。终点双方Scene/control1、位置/方向/全部镜头float32 bits/fullMT624及index对应，落星式5502、队伍[1,1,1,0]和四人好感矩阵对应。证据H:/PAL4/objs/live239-depart-story-verdict.json、live239-depart-dialogue-journal.json。

两端正常菜单分别写出327010字节、global40300、scene header20(Q03S)存档，归档H:/PAL4/objs/checkpoints/q03-depart-global40300-20261027-live239。native来源在live240、original来源在live241分别双端冷读，Scene/control1、位姿/全部镜头bits/fullMT、技能[5500,5501,5502]、队伍及好感对应。完整存档字段/RNG持久化仍未验收。下一主线直接从40300经func1060去M05/1。
