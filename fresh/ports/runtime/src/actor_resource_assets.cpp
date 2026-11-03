#include "actor_resource_assets.h"
#include <stdexcept>
namespace pal4::portable::detail {
void RequireStaticActorResources(PackageReader& package,std::string_view model){
    const auto base=std::string(model)+"\\"+std::string(model);
    if(!package.Find(base+".dff"))throw std::logic_error("original actor model is missing");
    if(package.Find(base+".uva"))throw std::logic_error("actor UVA resource update is not bound");
    if(package.Find(base+"_shadow.dff"))throw std::logic_error("actor shadow resource update is not bound");
}
}
