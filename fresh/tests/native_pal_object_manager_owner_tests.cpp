#include "pal4/native_pal_object_manager_owner.h"
#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void Check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
struct Object final:pal4::INativePalObject{
    pal4::NativePalObjectManagerOwner& manager;std::uint32_t id;std::vector<std::uint32_t>& log;unsigned& deleted;
    bool fail{},guard{};float elapsed{};
    Object(pal4::NativePalObjectManagerOwner& m,std::uint32_t id,std::vector<std::uint32_t>& l,unsigned& d):manager(m),id(id),log(l),deleted(d){}
    ~Object()override{++deleted;}
    std::uint32_t Address()const noexcept override{return id;}
    void Render(std::uint32_t camera)override{
        Check(camera==0x123456u,"PalObject forwards actual camera payload");log.push_back(id);
        if(guard)for(unsigned op=0;op<5u;++op){bool rejected{};try{
            if(op==0u)manager.Clear();
            if(op==1u)manager.Destroy();
            if(op==2u)(void)manager.Remove("a");
            if(op==3u)(void)manager.RenderFrame(camera);
            if(op==4u)(void)manager.Update(1.f);
        }catch(const std::logic_error&){rejected=true;}Check(rejected,"PalObject dispatch rejects mutation or reentry");}
        if(fail)throw std::runtime_error("intentional PalObject failure");
    }
    void Update(float value)override{elapsed=value;log.push_back(id+100u);}
};
}
int main(){try{
    pal4::NativePalObjectManagerOwner manager;std::vector<std::uint32_t> log;unsigned deleted{};
    Check(manager.Address() && !manager.Size() && manager.OutstandingAllocations()==2u,"PalObject real lifecycle creates image/name and empty host collection");
    Check(!manager.RenderFrame(0x123456u) && !manager.Update(.25f),"empty PalObject shared traversals require no fake callback");
    auto z=std::make_unique<Object>(manager,2u,log,deleted);auto a=std::make_unique<Object>(manager,1u,log,deleted);auto* first=a.get();
    Check(manager.Register("z",std::move(z)) && manager.Register("a",std::move(a)),"PalObject owns registered typed services");
    Check(manager.Find("A")==1u && !manager.Find("missing") && manager.Size()==2u,"PalObject uses original case insensitive key semantics");
    Check(!manager.Register("Z",std::make_unique<Object>(manager,3u,log,deleted)) && deleted==1u,"duplicate case key preserves existing owner and retires rejected owner");
    first->guard=true;first->fail=true;bool failed{};try{(void)manager.RenderFrame(0x123456u);}catch(const std::runtime_error& e){failed=std::string(e.what())=="intentional PalObject failure";}
    Check(failed && log==std::vector<std::uint32_t>{1u} && deleted==1u && manager.Size()==2u,"PalObject failure stops ordered traversal without deleting objects");
    first->fail=false;log.clear();Check(manager.RenderFrame(0x123456u)==2u && log==std::vector<std::uint32_t>{1u,2u},"PalObject same collection retries in key order without erase");
    log.clear();Check(manager.Update(.125f)==2u && std::bit_cast<std::uint32_t>(first->elapsed)==std::bit_cast<std::uint32_t>(.125f) && log==std::vector<std::uint32_t>{101u,102u},"PalObject virtual12 retains exact elapsed bits and order");
    Check(manager.Remove("A") && !manager.Remove("a") && deleted==2u && manager.Size()==1u,"PalObject case insensitive removal retires one owner");
    manager.Clear();manager.Clear();Check(deleted==3u && !manager.Size() && manager.OutstandingAllocations()==2u,"PalObject repeated clear retains only manager image/name");
    manager.Destroy();manager.Destroy();Check(!manager.Address() && !manager.OutstandingAllocations(),"PalObject destruction retires image/name storage");
    failed=false;try{(void)manager.RenderFrame(0x123456u);}catch(const std::logic_error&){failed=true;}Check(failed,"destroyed PalObject cannot fabricate a successful frame");
    std::cout<<"Native PalObject owner: shared lifecycle/traversals, host STL key order, exact virtual payloads, creator ownership, failure/reentry and zero allocations pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
