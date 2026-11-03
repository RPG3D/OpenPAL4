#include "pal4/native_global_key_down.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_pal_game_iv_state_list.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::uint32_t Address(const void* pointer) {
    return reinterpret_cast<std::uint32_t>(pointer);
}

struct Special final : pal4::IGlobalKeyDownSpecialRuntime {
    std::vector<std::string> calls;
    std::uint32_t manager;
    explicit Special(std::uint32_t value) : manager(value) {}
    void CaptureScreenshot() override { calls.emplace_back("screenshot"); }
    bool IsTimerFrameVisible(std::uint32_t value) override {
        Check(value == manager, "specialist receives native manager");
        calls.emplace_back("visible"); return true;
    }
    void DispatchTimerEvent(std::uint32_t value, std::uint32_t key,
                            std::uint32_t active, std::uint32_t frame) override {
        Check(value == manager && key == pal4::GlobalKeyDownHandler::kTimerEventKey &&
              active == 1u && frame == 777u, "native manager timer cell forwarded");
        calls.emplace_back("timer");
    }
    std::uint32_t FindFrame(std::uint32_t value, std::uint32_t key) override {
        Check(value == manager && key == pal4::GlobalKeyDownHandler::kFrameKey,
              "specialist frame lookup");
        calls.emplace_back("frame"); return 123u;
    }
    std::uint32_t CreateDocument() override { calls.emplace_back("create"); return 456u; }
    std::uint32_t FindAttribute(std::uint32_t doc, std::string_view name) override {
        Check(doc == 456u && name == "Frame", "specialist document lookup");
        calls.emplace_back("attribute"); return 0u;
    }
    void SetAttributeInt(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("unexpected existing attribute");
    }
    void AddAttributeInt(std::uint32_t doc, std::string_view name,
                         std::uint32_t value) override {
        Check(doc == 456u && name == "Frame" && value == 123u,
              "specialist frame attribute value");
        calls.emplace_back("add");
    }
    void ChangeGameState(std::uint32_t state, std::uint32_t doc) override {
        Check(state == 10u && doc == 456u, "specialist state transition");
        calls.emplace_back("change");
    }
    void DestroyDocument(std::uint32_t doc) override {
        Check(doc == 456u, "specialist document release"); calls.emplace_back("destroy");
    }
};

void Run() {
    std::array<std::uint32_t, 128> game{};
    std::array<std::uint32_t, 96> manager{};
    std::array<std::uint32_t, 5> event{};
    pal4::NativePalGameIVStateList states(Address(game.data()));
    Check(states.Register(0x1000u, 1u, [] {}) &&
          states.Register(0x2000u, 6u, [] {}), "native registered scene/loading states");
    Check(game[0x188u / 4u] != 0u, "state registry occupies +188h");
    game[0x18cu / 4u] = 0x1000u;
    std::vector<std::uint32_t> dispatched;
    pal4::NativeEventHandlerArray active(
        [](std::uint32_t, std::uint32_t) { return 1u; },
        [&](std::uint32_t object, std::uint32_t slot, std::uint32_t argument) {
            Check(slot == 0u && argument == Address(event.data()),
                  "global dispatch uses slot zero and borrowed DLL event");
            dispatched.push_back(object);
            return static_cast<std::uint8_t>(object);
        });
    bool playing = false;
    bool append_during_movie_query = false;
    unsigned closes = 0u;
    pal4::NativeGlobalKeyDownMovie movie{0x3000u,
        [&] {
            if (append_during_movie_query) {
                append_during_movie_query = false;
                (void)active.Insert(3u);
            }
            return playing;
        }, [&] { ++closes; playing = false; }};
    pal4::NativeGlobalKeyDown input(Address(game.data()), states,
        Address(manager.data()), active, movie);
    event[4] = 200u; // mapped W/up; ordinary keys require no specialist services.
    Check(input.Process(Address(event.data())) == 0u, "empty native activity");
    (void)active.Insert(1u); (void)active.Insert(2u);
    Check(input.Process(Address(event.data())) == 2u && dispatched.back() == 2u,
          "only top native activity receives down");
    event[4] = 1u;
    playing = true;
    dispatched.clear();
    Check(input.Process(Address(event.data())) == 0u && closes == 1u &&
          dispatched.empty(), "movie Escape bypasses top UI");
    append_during_movie_query = true;
    Check(input.Process(Address(event.data())) == 3u && dispatched.back() == 3u,
          "activity changed during movie query is read after callback");
    (void)active.Remove(3u);
    Check(input.Process(Address(event.data())) == 2u, "removing top restores prior activity");
    game[0x18cu / 4u] = 0u;
    event[4] = 67u;
    Check(input.Process(Address(event.data())) == 2u,
          "no current state makes timer key fall through to top UI");
    game[0x18cu / 4u] = 0x2000u;
    dispatched.clear();
    Check(input.Process(Address(event.data())) == 1u && dispatched.empty(),
          "registered loading mode consumes timer key without timer services");
    event[4] = 66u;
    bool rejected = false;
    try { (void)input.Process(Address(event.data())); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "missing screenshot binding is explicit");
    Special special(Address(manager.data()));
    pal4::NativeGlobalKeyDown complete(Address(game.data()), states,
        Address(manager.data()), active, movie, &special);
    Check(complete.Process(Address(event.data())) == 0u &&
          special.calls == std::vector<std::string>{"screenshot"},
          "bound screenshot service receives native branch");
    game[0x18cu / 4u] = 0x1000u;
    manager[0x144u / 4u] = 777u;
    special.calls.clear(); event[4] = 67u;
    Check(complete.Process(Address(event.data())) == 1u &&
          special.calls == std::vector<std::string>{"visible", "timer", "create",
              "frame", "attribute", "add", "change", "destroy"},
          "native scene state forwards full timer/XML sequence");
    game[0x18cu / 4u] = 0xdeadu;
    rejected = false;
    try { (void)input.Process(Address(event.data())); }
    catch (const std::out_of_range&) { rejected = true; }
    Check(rejected, "stale current state rejected before EXE virtual dispatch");
    game[0x18cu / 4u] = 0u;
    active.ReleaseStorage(); states.Clear();
    Check(active.OutstandingAllocations() == 0u && states.OutstandingAllocations() == 0u,
          "borrowed binding retains no array or registry allocations");
}
}  // namespace

int main() {
    try {
        Run();
        std::cout << "PASS: native global key-down state, live activity, movie and specialist dispatch\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n'; return 1;
    }
}
