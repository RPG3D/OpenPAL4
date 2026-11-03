# Q03 大厅换场景等待诊断（2026-10-27）

脚本依据直接读取 `I:/Games/original/gamepatch/gamedata/EditData/script/Q03.cs`（GBK），运行仍使用原始 CSB。live223 实际双端进入 Q03/XN04 的 func2009，41424–41438 共15句匹配并逐句发送相同 Space。41438 后原版到41439，native装载Q03X后无下一GPU帧；尚不能把原因判定为 ArenaReady 或镜头等待。

复现入口为正常保存的31300近档：`H:/PAL4/objs/checkpoints/m05-after-return-global31300-20261027-live219`。前面146句不重跑。live223两端已正常关闭；失败请求2232523及原始截图/metadata保留，不重复投递。

比较快照消息0x8504新增可选 `scripts` 对象，由应用服务只读采集、独立snapshot模块序列化。字段包括实际world/scene、arena_ready、pending_scene、scene_depth、timed_active、淡出phase/active/elapsed/duration，以及root/event/当前scene的runner status、SDK state、line和等待位。

等待位：combined=1、flash=2、movie=4、timer=8、action=16、turn=32、walk=64、blend=128、objecteffects=256、talk=512、camera=1024。event/scene采集各自当前owner；root只分解combined/flash/timer/camera，其他原因仍包含在combined中，不能以缺少某位判定root无该等待。movie位包含CompletionPending。line由SDK读取，原始字节码若无调试行可能无有效行号。

本改动不执行或恢复脚本，不修改时钟、剧情、RNG、角色、等待或场景状态。保留原ready gate。只有同一进程/请求的新诊断才可用于根因判断。

构建build1为遗留接口/实现不一致的链接失败；build2暴露movie接口差异；build3已通过。完整resource1通过（GUI13455/13455，正常shutdown）。实际修复与41439及后续31400终点尚未验收。

## live224 实测结论

新增诊断实际输出有效JSON：2243001为Q03X/ready1、pending false、scene finished、event suspended，并已显示41439。本轮26句41424–41449全部匹配后自然control1；终点全部位姿/镜头float32 bits及完整MT对应。没有修改游戏等待逻辑，不能宣称修复了223停滞或证明它只是加载。223 GPU超时的根因仍未确定。

原版正常保存31400（header37，仅云天河），已独立归档；native在F7/Space后发生System Present失败退出，未取得其新档。新档冷读待验。两端已关闭，后续从31400候选档验证，可靠回退为31300。完整resource1通过GUI13455/13455。现场证据H:/PAL4/objs/live224-hall-story-verdict.json和live224-work-state.json。
