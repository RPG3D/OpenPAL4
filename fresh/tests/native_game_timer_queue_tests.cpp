#include "pal4/native_game_timer_queue.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <string>
#include <utility>
#include <vector>
#include <array>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t ReadWord(std::uint32_t address) {
    return *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
}

}  // namespace

int main() {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
    pal4::NativeGameTimerQueue queue(
        [&](std::uint32_t owner, std::uint32_t method) {
            calls.emplace_back(owner, method);
        });
    const auto address = queue.Construct();
    const auto timer_head = ReadWord(address + 4u);
    const auto event_head = ReadWord(address + 16u);
    Check(address != 0u && timer_head != 0u && event_head != 0u &&
              ReadWord(timer_head) == timer_head &&
              ReadWord(timer_head + 4u) == timer_head &&
              ReadWord(event_head) == event_head &&
              queue.Count() == 0u,
          "native timer collection starts with original empty list layout");

    queue.AddRepeating("PalGameTimer", 1.0f, 0x1234u, 0x5c3c10u);
    const auto timer = ReadWord(timer_head);
    const auto callback = ReadWord(timer + 0x24u);
    Check(queue.Count() == 1u && ReadWord(timer + 4u) == timer_head &&
              ReadWord(callback) == 0x8494b0u &&
              ReadWord(callback + 4u) == 0x1234u &&
              ReadWord(callback + 8u) == 0x5c3c10u,
          "one-second timer retains the original callback image");
    queue.Update(0.5f);
    queue.Update(0.5f);
    Check(calls.empty(), "equal remaining time does not fire early");
    queue.Update(0.01f);
    Check(calls == std::vector<std::pair<std::uint32_t,
                                         std::uint32_t>>{{0x1234u, 0x5c3c10u}},
          "elapsed frame dispatches PALGameIV timer callback");

    queue.AddRepeating("other", 2.0f, 0x5678u, 0x123456u);
    Check(queue.Remove("PalGameTimer") == 1u && queue.Count() == 2u,
          "name removal releases callback before frame cleanup");
    queue.Update(0.1f);
    Check(queue.Count() == 1u && calls.size() == 1u,
          "next frame unlinks and frees removed timer");
    queue.Update(1.0f);
    Check(calls.size() == 1u,
          "remaining timer waits while time remains");
    queue.Update(1.0f);
    Check(calls.size() == 2u &&
              calls.back() == std::pair{0x5678u, 0x123456u},
          "remaining timer keeps running after named removal");

    queue.Destroy();
    Check(queue.Address() == 0u && queue.Count() == 0u &&
              queue.OutstandingAllocations() == 0u,
          "native callback, names, nodes and list heads release once");
    const auto rebuilt = queue.Construct();
    Check(rebuilt != 0u && queue.Count() == 0u,
          "native timer collection can restart after shutdown");
    const auto before_combat=calls.size();
    queue.AddCombatRefreshEvent("ResetCombatIcon",0.4f,0x7654u);
    const auto combat_node=ReadWord(ReadWord(rebuilt+16u));
    const auto combat_callback=ReadWord(combat_node+32u);
    Check(queue.EventCount()==1u && !queue.Count() && ReadWord(combat_callback)==0x84553cu &&
        ReadWord(combat_callback+12u)==0u,"Combat refresh retains original16-byte callback in one-shot list");
    queue.Update(0.4f);Check(calls.size()==before_combat,"Combat refresh does not fire at equality");
    queue.Update(0.001f);Check(calls.size()==before_combat+1u && calls.back()==std::pair{0x7654u,0x54cac0u},"Combat refresh dispatches actual owner once");
    queue.Update(1.f);Check(calls.size()==before_combat+1u && !queue.EventCount(),"Combat refresh never repeats and its node retires");
    queue.AddCombatRefreshEvent("ResetCombatIcon",0.4f,0x7654u);
    Check(queue.RemoveEvent("ResetCombatIcon")==1u,"Combat hide releases one-shot callback");
    queue.Update(1.f);Check(!queue.EventCount() && calls.size()==before_combat+1u,"cancelled Combat refresh never invokes retired owner");
    queue.AddRepeatingWithOffset("FlashRoleButton", 0.3f, 0x1234u, 0x479470u,
                                  0xfffffffcu);
    const auto ui_node = ReadWord(ReadWord(rebuilt + 4u));
    const auto ui_callback = ReadWord(ui_node + 0x24u);
    Check(ReadWord(ui_callback) == 0x843b20u &&
              ReadWord(ui_callback + 12u) == 0xfffffffcu,
          "UI timer retains original 16-byte callback with signed owner adjustment");
    const auto before_ui = calls.size();
    queue.Update(0.3f);
    Check(calls.size() == before_ui, "UI timer preserves strict original threshold");
    queue.Update(0.001f);
    Check(calls.size() == before_ui + 1u &&
              calls.back() == std::pair{0x1230u, 0x479470u},
          "existing callback algorithm applies the UI owner adjustment");
    queue.Destroy();
    Check(queue.OutstandingAllocations() == 0u,
          "restarted timer collection releases its list heads");
    (void)queue.Construct();
    const auto before_event = calls.size();
    queue.AddEvent("HintWindowClock", 1.0f, 0x4321u, 0x4abe30u);
    Check(queue.EventCount() == 1u && queue.Count() == 0u,
          "one-shot events use a separate original collection");
    queue.Update(1.0f);
    Check(calls.size() == before_event, "one-shot equality preserves strict threshold");
    queue.Update(0.001f);
    Check(calls.size() == before_event + 1u &&
              calls.back() == std::pair{0x4321u, 0x4abe30u} && queue.EventCount() == 1u,
          "one-shot dispatch releases callback before deferred unlink");
    queue.Update(0.0f);
    Check(queue.EventCount() == 1u, "zero delta does not unlink expired events");
    queue.Update(0.001f);
    Check(queue.EventCount() == 0u && calls.size() == before_event + 1u,
          "expired event unlinks without a second invocation");
    queue.AddEvent("HintWindowClock", 1.0f, 1u, 0x4abe30u);
    Check(queue.RemoveEvent("HintWindow") == 0u &&
              queue.RemoveEvent("HintWindowClock") == 1u,
          "event removal compares complete names");
    queue.AddEvent("HintWindowClock", 0.0f, 2u, 0x4abe30u);
    queue.Update(0.001f);
    Check(calls.size() == before_event + 2u && calls.back().first == 2u && queue.EventCount() == 1u,
          "replacement suppresses the old close while dispatching the new event");
    queue.AddEvent("pending-at-destruction", 100.0f, 3u, 0x4abe30u);
    queue.Destroy();
    Check(queue.EventCount() == 0u && queue.OutstandingAllocations() == 0u,
          "shutdown releases both pending and expired one-shot event storage");
    (void)queue.Construct();bool missing_dispatcher{};
    const auto before_missing=queue.OutstandingAllocations();
    try{queue.AddTradeCallback("missing",0.2f,1u,0x48cd80u,7,0xffu,false);}catch(const std::invalid_argument&){missing_dispatcher=true;}
    Check(missing_dispatcher && queue.OutstandingAllocations()==before_missing,"Trade callback requires parameter dispatcher before allocation");queue.Destroy();
    std::vector<std::array<std::uint32_t,4>> trade_calls;bool fail_trade{};
    pal4::NativeGameTimerQueue trade_queue([](auto,auto){throw std::logic_error("unexpected basic callback");},
        [&](auto owner,auto method,auto row,auto flag){if(fail_trade)throw std::runtime_error("trade retry");trade_calls.push_back({owner,method,static_cast<unsigned>(row),flag});});
    (void)trade_queue.Construct();
    trade_queue.AddTradeCallback("delay",0.2f,0x1234u,0x48cd80u,-7,0xffu,false);
    trade_queue.Update(0.2f);Check(trade_calls.empty(),"Trade parameter event preserves strict threshold");
    fail_trade=true;bool retry{};try{trade_queue.Update(0.001f);}catch(const std::runtime_error&){retry=true;}fail_trade=false;
    trade_queue.Update(0.001f);Check(retry && trade_calls.size()==1u && trade_calls.back()==std::array<std::uint32_t,4>{0x1234u,0x48cd80u,static_cast<unsigned>(-7),0xffu},"Trade callback error preserves retry and exact signed-row/byte payload");
    trade_queue.AddTradeCallback("repeat",0.1f,0x5678u,0x48cd20u,2,0u,true);
    trade_queue.Update(0.11f);Check(trade_calls.size()==2u && trade_calls.back()==std::array<std::uint32_t,4>{0x5678u,0x48cd20u,2u,0u},"Trade timer dispatches original parameter record");
    Check(trade_queue.Remove("repeat")==1u,"Trade timer uses existing named cancellation");
    trade_queue.Update(1.f);Check(trade_calls.size()==2u && !trade_queue.Count() && !trade_queue.EventCount(),"released parameter callbacks cannot dispatch again");
    trade_queue.Destroy();Check(!trade_queue.OutstandingAllocations(),"parameter event/timer callback ownership balances");
    pal4::NativeGameTimerQueue value_queue([](auto,auto){throw std::logic_error("unexpected basic callback");},{},
        [&](auto owner,auto method,auto value){if(fail_trade)throw std::runtime_error("value callback boundary");
            Check(owner==0x1234u && method==0x576440u && value==2u,"exact Combat result callback arguments");calls.emplace_back(owner,value);});
    (void)value_queue.Construct();const auto value_base=value_queue.OutstandingAllocations(),value_calls=calls.size();
    value_queue.AddCombatEndEvent("EventCloseSystem",0.f,0x1234u,2u);
    value_queue.Update(0.f);Check(calls.size()==value_calls && value_queue.EventCount()==1u,"value callback cannot execute synchronously or on zero delta");
    value_queue.Update(.001f);Check(calls.size()==value_calls+1u && value_queue.EventCount()==1u,"value callback fires once and retains expired node until next frame");
    value_queue.Update(.001f);Check(value_queue.EventCount()==0u && value_queue.OutstandingAllocations()==value_base,"value callback record/name/node all retire");
    value_queue.AddCombatEndEvent("EventCloseSystem",0.f,0x1234u,2u);fail_trade=true;bool value_failed{};
    try{value_queue.Update(.001f);}catch(const std::runtime_error&){value_failed=true;}fail_trade=false;
    Check(value_failed && value_queue.EventCount()==1u && value_queue.RemoveEvent("EventCloseSystem")==1u,"failed value callback stays queue owned until cancellation");
    value_queue.Update(.001f);Check(value_queue.OutstandingAllocations()==value_base && calls.size()==value_calls+1u,"canceled failed value callback never executes again");
    value_queue.Destroy();Check(!value_queue.OutstandingAllocations(),"value callback queue shutdown releases sentinels");
    {
        std::vector<std::pair<std::uint32_t,std::uint32_t>> reset_calls;
        pal4::NativeGameTimerQueue poetry([&](auto owner,auto method){reset_calls.emplace_back(owner,method);});
        const auto owner=poetry.Construct();
        poetry.AddPoetryResetEvent(1.5f,0x1234u);
        const auto head=ReadWord(owner+16u), node=ReadWord(head+4u);
        const auto callback=ReadWord(node+8u+24u);
        Check(ReadWord(callback)==0x844244u, "Poetry reset retains original member callback vtable");
        poetry.Update(1.5f);
        Check(reset_calls.empty(), "Poetry pause keeps strict original timer threshold");
        poetry.Update(.001f);
        Check(reset_calls==std::vector<std::pair<std::uint32_t,std::uint32_t>>{{0x1234u,0x4b3760u}},
              "Poetry reset reaches actual frame and reset method once");
        poetry.Update(.001f);
        poetry.AddPoetryResetEvent(1.5f,0x1234u);
        Check(poetry.RemoveEvent("PoetryResume")==1u, "Poetry layout retirement cancels pending resume");
        poetry.Update(2.f);
        Check(reset_calls.size()==1u, "retired Poetry callback cannot fire");
        poetry.Destroy();
        Check(poetry.OutstandingAllocations()==0u, "Poetry event and callback ownership balance");
    }
    {
        pal4::NativeGameTimerQueue map([](auto,auto){throw std::logic_error("unexpected basic map timer");});
        const auto map_address=map.Construct();
        const auto baseline=map.OutstandingAllocations();
        bool missing{};
        try{(void)map.AllocateWorldMapStorage();}catch(const std::logic_error&){missing=true;}
        Check(missing && map.OutstandingAllocations()==baseline,"WorldMap allocation requires live typed dispatcher");
        unsigned arrivals{};
        map.SetWorldMapCallback([&](auto owner,auto method,std::string_view location){
            Check(owner==0x1230u && method==0x484c10u && location=="M11",
                  "WorldMap flight receives adjusted owner and exact by-value destination");
            ++arrivals;
            Check(map.Remove("WorldMapFly")==1u,"arrival cancels its own original timer");
            Check(location=="M11","arrival text survives self-cancellation");
        });
        std::string source="M11";
        const auto storage=map.AllocateWorldMapStorage();
        const auto typed=map.ConstructWorldMapCallback(storage,0x1234u,0x484c10u,source,0xfffffffcu);
        source="changed";
        Check(typed==storage && ReadWord(typed)==0x843cd4u && ReadWord(typed+12u)==0xfffffffcu &&
              ReadWord(typed+24u)==3u && *reinterpret_cast<const unsigned char*>(ReadWord(typed+20u)-1u)==0u,
              "WorldMap callback keeps original32-byte image and exclusively owned flagged string");
        map.AddOwnedCallback("WorldMapFly",.01f,typed);
        map.Update(.01f);Check(!arrivals,"WorldMap flight preserves strict timer threshold");
        map.Update(5.f);Check(arrivals==1u && map.Count()==1u,"large frame invokes WorldMap once and defers retired node unlink");
        map.Update(.001f);Check(!map.Count() && map.OutstandingAllocations()==baseline,"WorldMap self-cancel releases text callback name and node once");
        const auto pending=map.AllocateWorldMapStorage();
        (void)map.ConstructWorldMapCallback(pending,0x1234u,0x484c10u,"M12");
        map.AddOwnedCallback("WorldMapFly",.01f,pending);
        map.ReleaseCallbacks();
        Check(map.OutstandingAllocations()==baseline+2u,"release-all destroys owned WorldMap text and callback before node unlink");
        map.Update(.001f);Check(map.OutstandingAllocations()==baseline,"released WorldMap node/name are reclaimed on next nonzero frame");
        const auto discarded=map.AllocateWorldMapStorage();
        (void)map.ConstructWorldMapCallback(discarded,0x1234u,0x484c10u,"");
        map.DiscardCallback(discarded);
        Check(map.OutstandingAllocations()==baseline,"empty WorldMap callback can be discarded before queue transfer");
        const auto shutdown=map.AllocateWorldMapStorage();
        (void)map.ConstructWorldMapCallback(shutdown,0x1234u,0x484c10u,"M13");
        map.AddOwnedCallback("WorldMapFly",.01f,shutdown);
        map.Destroy();Check(map.OutstandingAllocations()==0u,"shutdown releases pending WorldMap callbacks and their location storage");
    }
    return EXIT_SUCCESS;

}
