#include "pal4/monster_behavior_manager.h"
namespace pal4 {
void RegisterMonsterBehaviorFactories(std::uint32_t registry,IMonsterBehaviorFactoryRegistrationRuntime& runtime){
    for(std::uint32_t id=0;id<MonsterBehaviorManager::kFactories.size();++id)
        runtime.InsertBehavior(registry,id,MonsterBehaviorManager::kFactories[id]);
}
}
