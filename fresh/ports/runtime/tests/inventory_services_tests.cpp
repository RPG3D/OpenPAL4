#include "pal4/portable/inventory_services.h"
#include "pal4/portable/game_services.h"
#include "pal4/portable/tutorial_assets.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool ok){if(!ok)throw std::runtime_error("inventory service contract failed");}
int main(){try{
    const PropertyCatalog catalog{{1,{1,0,"test item"}},{2,{2,4,"second"}}};Inventory inventory(catalog);
    std::map<unsigned,std::string> text{{251,"gain %s x%d"},{252,"lose %s x%d"}};std::vector<std::string> notices,sounds;
    InventoryServices service(inventory,catalog,[&](auto key)->std::optional<std::string>{return text.at(key);},
        [&](auto value,float x,float y,float duration){Check(x==.5f && y==.15f && duration==1);notices.emplace_back(value);},
        [&](auto name,auto repeats){Check(repeats==1);sounds.emplace_back(name);});
    std::ostringstream trace;GameServices host(trace);
    host.add_property=[&](auto id,auto count,auto hint){service.Add(id,count,hint);};host.remove_property=[&](auto id,auto count,auto hint){service.Remove(id,count,hint);};
    ScriptGlobals globals{};ScriptStack stack(globals);stack.Compile("fixture","void Probe(){giAddProperty(1,3,true);giAddProperty(2,2,false);giDelProperty(1,1,true);}");
    Check(stack.Push("fixture","void Probe()",host).has_value() && stack.Tick(0)==ScriptState::finished);
    Check(inventory.Count(1)==2 && inventory.Count(2)==2 && notices==std::vector<std::string>{"gain test item x3","lose test item x1"} && sounds==std::vector<std::string>{"WA06","WA07"});
    service.Add(999,1,true);Check(notices.size()==2 && sounds.size()==2);
    service.Add(1,-2,true);Check(inventory.Count(1)==0 && notices.back()=="gain test item x-2");
    text[251]="%n";bool rejected=false;try{service.Add(1,1,true);}catch(const std::exception&){rejected=true;}Check(rejected && inventory.Count(1)==1);
    text[251]="gain %s x%d";service.Add(1,1,true);Check(inventory.Count(1)==2 && notices.back()=="gain test item x1"); // 异常清理文本buffer，不回滚原已发生的物品操作。
    std::cout<<"original item service, script routing, hints, audio and cleanup contracts passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
