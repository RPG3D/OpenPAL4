#include "pal4/game_timer_help_window_callback.h"
#include "pal4/dialog_manager_lifecycle.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "native_texture_dictionary_fixture.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <string>

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

struct TimerRuntime final : pal4::IGameTimerHelpWindowRuntime {
    std::uint32_t manager{};
    std::uint32_t frame_lookups{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        return ReadWord(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        *reinterpret_cast<std::uint32_t*>(
            static_cast<std::uintptr_t>(address)) = value;
    }
    std::uint32_t UiFrameManager() override { return manager; }
    std::uint32_t FindFrame(std::uint32_t found_manager,
                            std::string_view name) override {
        Check(found_manager == manager && name == "gameInfo",
              "timer looks up original game information frame");
        ++frame_lookups;
        return 0u;
    }
    std::uint8_t UpdateClock(std::uint32_t) override {
        Check(false, "unloaded gameInfo frame has no clock widget");
        return 0u;
    }
};

}  // namespace

int main() {
    {
        unsigned destroyed = 0u;
        pal4::NativePalGameIVTextureStorage cache([&](std::uint32_t dictionary) {
            Check(dictionary == 0x1234u, "native removal destroys the matched dictionary");
            ++destroyed;
        });
        (void)cache.Construct();
        Check(!cache.RemoveTextureDictionary("missing"), "empty native cache lookup misses");
        pal4::test::SeedSingleTextureDictionary(cache, "World_d3d9.txd", 0x1234u, 2u);
        const auto node = ReadWord(cache.Dictionary().head + 4u);
        Check(!cache.RemoveTextureDictionary("world_d3d9.txd") &&
                  !cache.RemoveTextureDictionary("World_d3d9.tx") &&
                  ReadWord(node + 32u) == 2u && destroyed == 0u,
              "native cache ordering preserves case and full key length");
        Check(cache.RemoveTextureDictionary("World_d3d9.txd") &&
                  ReadWord(node + 32u) == 1u && cache.Dictionary().count == 1u &&
                  destroyed == 0u,
              "native cache removal retains a dictionary with another reference");
        Check(cache.RemoveTextureDictionary("World_d3d9.txd") && destroyed == 1u &&
                  cache.Dictionary().count == 0u &&
                  !cache.RemoveTextureDictionary("World_d3d9.txd"),
              "last native cache reference destroys dictionary and removes its heap node");
        cache.Destroy();
        Check(destroyed == 1u, "cache teardown does not destroy a retired dictionary twice");
    }
    std::uint32_t destroyed_dictionaries{};
    const auto destroy = [&](std::uint32_t) { ++destroyed_dictionaries; };
    pal4::NativePalGameIVTextureStorage first(destroy);
    pal4::NativePalGameIVTextureStorage second(destroy);
    const auto first_address = first.Construct();
    try {
        (void)first.GameDataEntry(0);
        Check(false, "unbound global data must fail");
    } catch (const std::logic_error&) {
    }
    pal4::DialogManagerState global_data;
    *reinterpret_cast<std::uint32_t*>(global_data.ImageData() + 0x34u) =
        0x12345678u;
    first.BindGlobalData(global_data);
    Check(first.GameDataEntry(0) == 0x12345678u,
          "first game data entry reads the global data image");
    first.SetGameDataEntry(7, 0xabcdef01u);
    Check(*reinterpret_cast<std::uint32_t*>(
              global_data.ImageData() + 0x50u) == 0xabcdef01u &&
              first.GameDataEntry(7) == 0xabcdef01u,
          "eighth game data entry writes the global data image");
    const auto first_head = first.Dictionary().head;
    Check(reinterpret_cast<std::uintptr_t>(&first.Dictionary()) ==
              first_address + 0x1acu,
          "native texture tree matches object-base ADD EAX,1ACh in IDA callers");
    const auto nil = ReadWord(first_head + 4u);
    constexpr std::string_view names[] = {"ElapsedTime", "MVar", "MovieFlag"};
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto record = first_address + 0x124u + 16u * index;
        const auto text = ReadWord(record + 4u);
        Check(text != 0u && ReadWord(record + 8u) == names[index].size() &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(text))) == names[index],
              "PALGameIV image owns its three named game variables");
    }
    const auto global_begin = ReadWord(first_address + 0x158u);
    const auto global_end = ReadWord(first_address + 0x15cu);
    Check(global_begin != 0u && global_end - global_begin == 38u * 4u &&
              ReadWord(global_begin) == first.GlobalAddress(0x90dec0u) &&
              ReadWord(global_begin + 17u * 4u) ==
                  first.GlobalAddress(0x90df2cu) &&
              ReadWord(global_end - 4u) == first.GlobalAddress(0x90df7cu) &&
              ReadWord(first_address + 0x28u) == 1u &&
              ReadWord(first_address + 0x70u) == 1u,
          "PALGameIV image owns original core flags and 38 relocated globals");
    *reinterpret_cast<std::uint32_t*>(static_cast<std::uintptr_t>(
        first.GlobalAddress(0x90dec4u))) = 0x1234u;
    Check(first_address != 0u && first_head != 0u && nil != 0u &&
              first_head != nil && first.Dictionary().count == 0u &&
              ReadWord(first_head) == first_head &&
              ReadWord(first_head + 8u) == first_head &&
              first.CoreAllocationCount() == 4u,
          "PALGameIV image constructs its empty texture tree at +0x1ac");
    const auto string_data = ReadWord(first_address + 0x190u);
    Check(ReadWord(first_address + 0x178u) == 0u &&
              ReadWord(first_address + 0x17cu) == 0u &&
              ReadWord(first_address + 0x180u) == 0u &&
              ReadWord(first_address + 0x184u) == 0u &&
              ReadWord(first_address + 0x174u) == 0u &&
              ReadWord(first_address + 0x188u) == 0u &&
              ReadWord(first_address + 0x18cu) == 0u &&
              string_data != 0u &&
              ReadWord(first_address + 0x194u) == 0u &&
              ReadWord(first_address + 0x198u) == 32u &&
              *reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(string_data)) == '\0' &&
              ReadWord(first_address + 0x1a0u) == 0u &&
              ReadWord(first_address + 0x1a4u) == 0u &&
              ReadWord(first_address + 0x1a8u) == 0u,
          "PALGameIV owns original bone, empty string and state fields");
    first.AddStateMonster("MN001");
    first.AddStateMonster("MN001");
    first.AddStateMonster("MN002");
    first.AddStateMonster("MN003");
    const auto state_begin = ReadWord(first_address + 0x1a0u);
    Check(first.StateMonsterCount() == 3u && state_begin != 0u &&
              ReadWord(first_address + 0x1a4u) - state_begin == 36u &&
              ReadWord(first_address + 0x1a8u) - state_begin == 48u &&
              first.StateCollectionAllocationCount() == 4u &&
              std::string_view(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(ReadWord(state_begin)))) ==
                  "MN001",
          "PALGameIV state monster collection grows and deduplicates");
    std::vector<std::string> visited;
    const auto visit_allocations=first.StateCollectionAllocationCount();
    first.VisitStateMonsters([&](std::string_view name){visited.emplace_back(name);});
    Check(visited==std::vector<std::string>{"MN001","MN002","MN003"} &&
          first.StateMonsterCount()==3u && first.StateCollectionAllocationCount()==visit_allocations,
          "native5B6A40 visits actual retained strings in insertion order without ownership changes");
    const auto reject=[](const auto& action){bool threw=false;try{action();}catch(const std::exception&){threw=true;}Check(threw,"state-name traversal rejects invalid operation");};
    reject([&]{first.VisitStateMonsters({});});
    reject([&]{first.VisitStateMonsters([&](std::string_view){
        reject([&]{first.AddStateMonster("foreign");});
        reject([&]{first.ClearStateCollection();});
        reject([&]{first.Destroy();});
        reject([&]{first.VisitStateMonsters([](auto){});});
        throw std::runtime_error("visitor failure");
    });});
    visited.clear();first.VisitStateMonsters([&](auto name){visited.emplace_back(name);});
    Check(visited.size()==3u && first.StateMonsterCount()==3u &&
          first.StateCollectionAllocationCount()==visit_allocations,
          "visitor exception unwinds pin and preserves every original name for retry");
    first.ClearStateCollection();
    Check(first.StateMonsterCount() == 0u &&
              ReadWord(first_address + 0x1a4u) == state_begin &&
              first.StateCollectionAllocationCount() == 1u,
          "Scene entry clears state names but retains vector capacity");
    first.AddStateMonster("MN004");
    Check(first.StateMonsterCount() == 1u &&
              ReadWord(first_address + 0x1a0u) == state_begin &&
              first.StateCollectionAllocationCount() == 2u,
          "state collection reuses its retained vector storage");
    const auto second_address = second.Construct();
    Check(second_address != first_address &&
              second.Dictionary().head != first_head &&
              ReadWord(second.Dictionary().head + 4u) == nil &&
              second.GlobalAddress(0x90dec4u) ==
                  first.GlobalAddress(0x90dec4u) &&
              ReadWord(second.GlobalAddress(0x90dec4u)) == 0u &&
              ReadWord(first.GlobalAddress(0x90dec4u)) == 0u,
          "owners share process globals and second initialization resets them");
    second.Destroy();
    Check(!second.Constructed() && ReadWord(first_head + 4u) == nil,
          "destroying one tree preserves another owner's nil node");
    first.Destroy();
    first.Destroy();
    Check(!first.Constructed() && first.CoreAllocationCount() == 0u &&
              first.StateCollectionAllocationCount() == 0u &&
              destroyed_dictionaries == 0u,
          "empty tree teardown releases its nodes once");
    const auto rebuilt = second.Construct();
    Check(rebuilt != 0u && second.Dictionary().head != 0u,
          "tree storage can reconstruct after all prior owners release it");
    second.Destroy();
    Check(second.CoreAllocationCount() == 0u,
          "reconstructed core strings release on teardown");

    std::array<std::uint32_t, 0x224u / 4u> manager_image{};
    TimerRuntime timer_runtime;
    timer_runtime.manager = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(manager_image.data()));
    pal4::NativeGameTimerQueue timer([&](std::uint32_t owner,
                                           std::uint32_t method) {
        Check(method == 0x5c3c10u,
              "queue dispatches original PALGameIV timer method");
        (void)pal4::GameTimerHelpWindowCallback(owner).Tick(timer_runtime);
    });
    manager_image[0x1a0u / 4u] = timer.Construct();
    pal4::NativePalGameIVTextureStorage game_with_timer(destroy);
    const auto timer_address = manager_image[0x1a0u / 4u];
    manager_image[0x1a0u / 4u] = 0u;
    try {
        (void)game_with_timer.Construct(
            {&timer, timer_runtime.manager, {}});
        Check(false, "invalid UI manager binding must fail");
    } catch (const std::invalid_argument&) {
        Check(!game_with_timer.Constructed() &&
                  game_with_timer.CoreAllocationCount() == 0u &&
                  timer.Count() == 0u,
              "invalid UI manager binding does not allocate game state");
    }
    manager_image[0x1a0u / 4u] = timer_address;
    try {
        (void)game_with_timer.Construct(
            {&timer, timer_runtime.manager,
             [](std::uint32_t) {
                 throw std::runtime_error("timer frame setup failed");
             }});
        Check(false, "failed timer setup must abort construction");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "timer frame setup failed" &&
                  !game_with_timer.Constructed() &&
                  game_with_timer.CoreAllocationCount() == 0u &&
                  timer.Count() == 0u,
              "failed timer setup rolls back PALGameIV core storage");
    }
    std::uint32_t timer_text_updates{};
    const auto game_address = game_with_timer.Construct(
        {&timer, timer_runtime.manager,
         [&](std::uint32_t manager) {
             Check(manager == timer_runtime.manager,
                   "timer text uses the bound UI manager");
             ++timer_text_updates;
         }});
    Check(timer.Count() == 1u &&
              timer_text_updates == 1u &&
              game_with_timer.CoreAllocationCount() == 4u &&
              ReadWord(game_address) == 0u,
          "PALGameIV construction registers its one-second UI timer");
    timer.Update(1.0f);
    Check(ReadWord(game_address) == 0u,
          "timer waits when its remaining duration equals frame time");
    timer.Update(0.01f);
    Check(ReadWord(game_address) == 1u &&
              timer_runtime.frame_lookups == 1u,
          "rewritten timer callback advances PALGameIV elapsed seconds");
    const auto story_objects = game_with_timer.StoryObjects();
    const auto story_begin = ReadWord(game_address + 0x158u);
    for (unsigned reset = 0u; reset < 3u; ++reset) {
        *reinterpret_cast<std::uint32_t*>(story_objects.front()) = 999u;
        game_with_timer.InitializeCoreSystems();
        // Original removal retires the callback immediately; the next nonzero
        // frame reclaims its list node. Count includes that retired node.
        timer.Update(0.01f);
        Check(game_with_timer.StoryObjects() == story_objects &&
                  story_objects.size() == 38u &&
                  ReadWord(game_address + 0x158u) == story_begin &&
                  ReadWord(story_objects.front()) == 0u &&
                  ReadWord(game_address) == 0u && timer.Count() == 1u,
              "repeated NewGame core reset retains38 Story pointers and replaces one timer");
    }
    game_with_timer.Destroy();
    timer.Update(0.01f);
    Check(timer.Count() == 0u &&
              game_with_timer.CoreAllocationCount() == 0u,
          "PALGameIV teardown releases timer callback before frame cleanup");
    timer.Destroy();
    Check(timer.OutstandingAllocations() == 0u,
          "timer collection releases its native image after game shutdown");
    return EXIT_SUCCESS;
}
