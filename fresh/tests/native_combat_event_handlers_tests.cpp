#include "pal4/native_combat_event_handlers.h"
#include "pal4/native_script_event_router.h"
#include <iostream>
#include <stdexcept>
#include <vector>
int main(){try{
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto fn){bool failed{};try{fn();}catch(const std::logic_error&){failed=true;}check(failed,"expected handler rejection");};
    pal4::NativeScriptEventRouter router;std::uint32_t all{},extended{};
    reject([&]{pal4::NativeCombatEventHandlers missing(router,all,extended,{});});
    check(!all && !extended && !router.SubscriptionCount(),"missing domain leaves no publication/subscriptions");
    std::vector<unsigned> order;unsigned calls{};bool fail{};
    const auto before=router.Subscribe(router.Key("me_can_take_action"),1u,[&](unsigned){order.push_back(1u);});
    const auto after=router.Subscribe(router.Key("me_can_take_action"),3u,[&](unsigned){order.push_back(3u);});
    pal4::NativeCombatEventHandlers* current{};
    {
        pal4::NativeCombatEventHandlers handlers(router,all,extended,[&](unsigned owner,unsigned entry,unsigned payload){
            check(owner==extended && entry==0x57c370u && payload==123u,"dispatch retains owner/entry/payload identity");
            reject([&]{current->Close();});
            if(fail)throw std::logic_error("intentional domain failure");
            ++calls;order.push_back(2u);
        });current=&handlers;
        check(all==handlers.AllAddress() && extended==handlers.ExtendedAddress() && handlers.SubscriptionCount()==25u,
              "both actual facades publish25 subscriptions");
        reject([&]{pal4::NativeCombatEventHandlers duplicate(router,all,extended,[](auto,auto,auto){});});
        check(router.SubscriptionCount()==27u,"duplicate rejects without registering callbacks");
        (void)router.Dispatch("close_system",0u);check(!calls,"verified57B8D0 RET has no required domain call");
        (void)router.Dispatch("me_can_take_action",123u);
        check(calls==1u && order==std::vector<unsigned>{1u,2u,3u},"original mode2 dispatch ordering");
        fail=true;reject([&]{(void)router.Dispatch("me_can_take_action",123u);});fail=false;order.clear();
        (void)router.Dispatch("me_can_take_action",123u);check(calls==2u,"callback failure releases scope for retry");
        const auto saved=extended;extended=1u;reject([&]{handlers.Close();});extended=saved;
        check(handlers.SubscriptionCount()==25u,"foreign slot rejection preserves subscriptions");
        handlers.Close();handlers.Close();check(!all && !extended && !handlers.SubscriptionCount(),"Close clears both members");
    }
    check(router.SubscriptionCount()==2u,"other router consumers survive handler retirement");
    (void)router.Unsubscribe(before);(void)router.Unsubscribe(after);
    {pal4::NativeCombatEventHandlers handlers(router,all,extended,[](auto,auto,auto){});}
    check(!all && !extended && !router.SubscriptionCount(),"destructor drains both facades");
    std::cout<<"PASS: native Combat handlers404/408,25 bindings, domain/order/exception guards and teardown\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
