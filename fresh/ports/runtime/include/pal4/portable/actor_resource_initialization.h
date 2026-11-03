#pragma once
#include "pal4/portable/scene_monster_instances.h"
#include "pal4/portable/package.h"
#include "pal4/monster_behavior_manager.h"
namespace pal4::portable {
// 为已载入且原包确认无动态资源的单主体建立主资源视图。
// 只初始化前92个word；不代替BaseObject/MonsterBehaviorManager完整构造。
// clump/manager为调用方提供的稳定数值身份，mode/extra保留原装载参数。
// 失败不发布新图像；要求主资源视图尚未构造，且实例未绑定变换。
void InitializeActorResource(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,
    std::uint32_t manager_token,std::uint32_t clump_token,std::uint32_t mode,std::uint32_t extra,PackageReader&);
// 将借用主体绑定到已构造、尚无主体的资源视图。保留虚表、配置和成员所有权，
// 不重新构造资源，不代替原 LoadClump 的标签/纹理/动态资源装载或完整怪物构造。
// 要求单位缩放、无第二主体且实例未绑定变换；失败不发布，成功只更新主资源。
void BindConstructedActorResource(SceneMonsterInstances&,SceneMonsterHandle,MonsterBehaviorManagerState&,
    std::uint32_t manager_token,std::uint32_t clump_token,std::uint32_t mode,std::uint32_t extra,PackageReader&);
}
