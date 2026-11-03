#include "pal4/portable/monster_scene_resources.h"
#include <stdexcept>
#include <string>

namespace pal4::portable {
namespace {
std::shared_ptr<ArenaSource> RequireSource(std::shared_ptr<ArenaSource> source){
    if(!source)throw std::invalid_argument("monster scene requires original package source");
    return source;
}
}
MonsterSceneResources::MonsterSceneResources(std::uint64_t generation,std::shared_ptr<ArenaSource> source,
    std::span<const SceneMonster> monsters,std::span<const ScenePath> paths)
    :source_(RequireSource(std::move(source))),paths_(std::make_shared<ScenePathways>(generation,paths)),
     monsters_(generation,monsters,[source=source_](std::string_view model){
         const std::string name(model);
         return source->ReadOriginalActorResource("gamedata\\PALActor\\"+name+"\\",name,{},true);
     },[source=source_](std::string_view member)->std::optional<std::vector<std::uint8_t>>{
         const auto separator=member.find_last_of("/\\");
         if(separator==member.npos || separator==0 || separator+1==member.size())throw std::invalid_argument("invalid monster actor member");
         return source->ReadObjectMember("gamedata\\PALActor\\"+std::string(member.substr(0,separator))+"\\",member.substr(separator+1));
     }){}
}
