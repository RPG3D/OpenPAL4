#include "pal4/game_system_data.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameSystemDataRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, bool>> insertions;
    std::vector<std::uint32_t> entries;
    std::map<std::uint32_t, std::uint32_t> memory{{0x71a0u, 0x9000u}};
    std::uint32_t erased_begin = 0xffffffffu;
    std::uint32_t erased_end = 0xffffffffu;
    std::uint32_t callback_result = 0x8000u;
    bool fail_add = false;
    bool fail_movie_text = false;

    void InitializeNamedText(std::uint32_t address,
                             std::string_view text) override {
        Check((address == 0x1124u && text == "ElapsedTime") ||
                  (address == 0x1134u && text == "MVar") ||
                  (address == 0x1144u && text == "MovieFlag"),
              "three native text records and names");
        calls.emplace_back(std::string("text ") + std::string(text));
        if (fail_movie_text && text == "MovieFlag")
            throw std::runtime_error("text failed");
    }
    void ReleaseNamedText(std::uint32_t address) override {
        calls.emplace_back("release text " + std::to_string(address));
    }
    void ZeroBytes(std::uint32_t address, std::uint32_t count) override {
        calls.emplace_back("zero " + std::to_string(address) + " " +
                           std::to_string(count));
        for (std::uint32_t offset = 0; offset < count; offset += 4u)
            memory[address + offset] = 0u;
    }
    void FillDwords(std::uint32_t address, std::uint32_t count,
                    std::uint32_t value) override {
        Check(address == 0x1028u && count == 13u && value == 1u,
              "13 core status words at +0x28");
        calls.emplace_back("fill core");
        for (std::uint32_t i = 0; i < count; ++i)
            memory[address + i * 4u] = value;
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        memory[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return memory[address];
    }
    void ErasePointerRange(std::uint32_t collection,
                               std::uint32_t begin,
                               std::uint32_t end) override {
        Check(collection == 0x1154u, "native pointer-vector offset");
        calls.emplace_back("erase pointers");
        erased_begin = begin;
        erased_end = end;
        entries.clear();
    }
    void InsertPointer(std::uint32_t collection,
                       std::uint32_t address,
                       bool copies) override {
        Check(collection == 0x1154u, "pointer-vector insertion address");
        calls.emplace_back(copies ? "insert copies" : "insert one");
        insertions.emplace_back(address, copies);
        entries.push_back(address);
    }
    std::vector<std::uint32_t> PointerEntries(
        std::uint32_t collection) override {
        Check(collection == 0x1154u, "pointer-vector iteration address");
        calls.emplace_back("entries");
        return entries;
    }
    void ReleasePointerCollection(std::uint32_t collection) override {
        Check(collection == 0x1154u, "pointer-vector cleanup address");
        calls.emplace_back("release pointers");
    }
    void Log(std::string_view format, std::string_view source,
             std::uint32_t line) override {
        Check(source == R"(D:\VcProject\Pal4\project\Sourcecode\PAL4\PAL4Game\palGameData.cpp)",
              "original game-data source path");
        if (line == 40u) {
            Check(format ==
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  palGameData initializing begin...\n\n\n",
                  "begin log text");
            calls.emplace_back("log begin");
        } else {
            Check(line == 122u && format ==
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  palGameData initialize ok\n\n\n",
                  "end log text and line");
            calls.emplace_back("log end");
        }
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frame");
        return 0x7000u;
    }
    void UpdateTimerFrameText(std::uint32_t manager) override {
        Check(manager == 0x7000u, "timer text manager");
        calls.emplace_back("update timer text");
    }
    void InitializeTimerName(std::string_view name) override {
        Check(name == "PalGameTimer", "timer identifier");
        calls.emplace_back("init timer name");
    }
    void RemoveTimer(std::uint32_t queue,
                     std::string_view name) override {
        Check(queue == 0x9000u && name == "PalGameTimer",
              "timer removal queue and name");
        calls.emplace_back("remove timer");
    }
    std::uint32_t AllocateTimerCallback(
        std::uint32_t size, std::uint32_t vtable,
        std::uint32_t owner, std::uint32_t method) override {
        Check(size == 12u && vtable == 0x8494b0u && owner == 0x1000u &&
                  method == 0x5c3c10u,
              "timer callback object layout and method");
        calls.emplace_back("callback");
        return callback_result;
    }
    std::uint32_t ConstructTimerEntry(
        std::string_view name, std::uint32_t duration_bits,
        std::uint32_t callback) override {
        Check(name == "PalGameTimer" && duration_bits == 0x3f800000u &&
                  callback == callback_result,
              "timer entry name, one-second duration and callback");
        calls.emplace_back("construct timer");
        return 0x8100u;
    }
    void AddTimer(std::uint32_t queue, std::uint32_t entry) override {
        Check(queue == 0x9000u && entry == 0x8100u,
              "timer insertion operands");
        calls.emplace_back("add timer");
        if (fail_add) throw std::runtime_error("add failed");
    }
    void DestroyTimerEntry(std::uint32_t entry) override {
        Check(entry == 0x8100u, "temporary timer entry cleanup");
        calls.emplace_back("destroy timer");
    }
    std::int32_t CleanupTimerName() override {
        calls.emplace_back("cleanup timer name");
        return -43;
    }
    std::uint32_t GlobalDataSubsystem() override {
        calls.emplace_back("global data");
        return 0xa000u;
    }
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected runtime call exists");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void InitializerPreservesLayoutAndOrder() {
    Runtime runtime;
    const pal4::GameSystemData game(0x1000u);
    Check(game.InitializeVariables(runtime) == 0x1000u,
          "outer initializer returns object address");
    Check(runtime.calls[0] == "text ElapsedTime" &&
              runtime.calls[1] == "text MVar" &&
              runtime.calls[2] == "text MovieFlag" &&
              runtime.calls[3] == "zero 4436 32" &&
              Position(runtime, "log begin") < Position(runtime, "erase pointers") &&
              Position(runtime, "entries") < Position(runtime, "frame") &&
              Position(runtime, "update timer text") <
                  Position(runtime, "init timer name") &&
              Position(runtime, "remove timer") <
                  Position(runtime, "callback") &&
              Position(runtime, "add timer") <
                  Position(runtime, "destroy timer") &&
              Position(runtime, "destroy timer") <
                  Position(runtime, "log end") &&
              runtime.calls.back() == "cleanup timer name",
          "text, core, globals and timer order");
    Check(runtime.memory[0x1000u] == 0u &&
              runtime.memory[0x1028u] == 1u &&
              runtime.memory[0x1058u] == 1u &&
              runtime.memory[0x105cu] == 0u &&
              runtime.memory[0x1070u] == 1u &&
              runtime.erased_begin == 0u && runtime.erased_end == 0u,
          "core zero/fill fields and empty initial vector");
    Check(runtime.insertions.size() == 38u &&
              runtime.insertions[0] == std::pair{0x90dec0u, false},
          "first pointer insertion");
    for (std::uint32_t i = 1u; i <= 16u; ++i)
        Check(runtime.insertions[i] ==
                  std::pair{0x90dee8u + 4u * i, false},
              "17 single pointer insertions");
    for (std::uint32_t i = 17u; i < 38u; ++i)
        Check(runtime.insertions[i] ==
                  std::pair{0x90df2cu + 4u * (i - 17u), true},
              "21 copied pointer insertions");
    Check(runtime.writes.size() == 53u &&
              runtime.writes[0] == std::pair{0x1070u, 1u},
          "one core write, 48 global resets and four timer words");
    for (std::size_t i = 0; i < runtime.entries.size(); ++i)
        Check(runtime.writes[i + 1u] ==
                  std::pair{runtime.entries[i], 0u},
              "pointer collection reset order");
    for (std::uint32_t i = 0u; i < 10u; ++i)
        Check(runtime.writes[39u + i] ==
                  std::pair{0x90dec4u + 4u * i, 0u},
              "ten direct global resets");
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(runtime.writes[49u + i] ==
                  std::pair{0x1164u + 4u * i, 0u},
              "four timer-state words reset");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(), "frame") == 3,
          "three UI manager lookups");
}

void CoreUsesActualCollectionAndHandlesAllocationFailure() {
    Runtime runtime;
    runtime.memory[0x1158u] = 0x5000u;
    runtime.memory[0x115cu] = 0x5100u;
    runtime.entries.push_back(0x90df80u);
    runtime.callback_result = 0u;
    const pal4::GameSystemData game(0x1000u);
    Check(game.InitializeCoreSystems(runtime) == -43 &&
              runtime.erased_begin == 0x5000u &&
              runtime.erased_end == 0x5100u &&
              runtime.entries.size() == 38u &&
              runtime.writes[1] == std::pair{0x90dec0u, 0u} &&
              Position(runtime, "construct timer") <
                  Position(runtime, "add timer"),
          "direct core init reads existing vector and still inserts zero callback");
    const auto original_entries = runtime.entries;
    (void)game.InitializeCoreSystems(runtime);
    Check(runtime.entries == original_entries,
          "repeated core initialization replaces rather than appends Story pointers");
}

void StorageCanPrecedeUiTimer() {
    Runtime runtime;
    const pal4::GameSystemData game(0x1000u);
    Check(game.InitializeVariableStorage(runtime) == 0x1000u &&
              runtime.entries.size() == 38u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "frame") == 0 &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "init timer name") == runtime.calls.end(),
          "PALGameIV storage can be constructed before its UI timer owner");
    Check(game.InitializeUiTimer(runtime) == -43 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "frame") == 3,
          "deferred UI timer preserves the original second phase");
}

void ExceptionsReleaseConstructedState() {
    const pal4::GameSystemData game(0x1000u);
    Runtime text_failure;
    text_failure.fail_movie_text = true;
    try {
        (void)game.InitializeVariables(text_failure);
        throw std::runtime_error("expected text failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "text failed",
              "named text failure propagates");
    }
    Check(text_failure.calls == std::vector<std::string>{
              "text ElapsedTime", "text MVar", "text MovieFlag",
              "release text 4404", "release text 4388"},
          "completed named texts release in reverse order");

    Runtime timer_failure;
    timer_failure.fail_add = true;
    try {
        (void)game.InitializeVariables(timer_failure);
        throw std::runtime_error("expected timer failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "add failed",
              "timer insertion failure propagates");
    }
    Check(Position(timer_failure, "add timer") <
              Position(timer_failure, "destroy timer") &&
              Position(timer_failure, "destroy timer") <
              Position(timer_failure, "cleanup timer name") &&
              Position(timer_failure, "cleanup timer name") <
              Position(timer_failure, "release pointers") &&
              timer_failure.calls.back() == "release text 4388",
          "timer, pointer collection and named texts release on exception");
}

void EntryAccessSplitsAtSeven() {
    const pal4::GameSystemData game(0x1000u);
    Runtime runtime;
    runtime.memory[0xa034u] = 11u;
    runtime.memory[0xa050u] = 18u;
    runtime.memory[0xa030u] = 19u;
    runtime.memory[0x1028u] = 22u;
    Check(game.GetEntry(0, runtime) == 11u &&
              game.GetEntry(7, runtime) == 18u &&
              game.GetEntry(-1, runtime) == 19u &&
              game.GetEntry(8, runtime) == 22u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "global data") == 3,
          "signed boundary and negative index use global-data offsets");
    runtime.calls.clear();
    game.SetEntry(7, 101u, runtime);
    game.SetEntry(8, 102u, runtime);
    game.SetEntry(-1, 103u, runtime);
    Check(runtime.memory[0xa050u] == 101u &&
              runtime.memory[0x1028u] == 102u &&
              runtime.memory[0xa030u] == 103u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "global data") == 2,
          "setter forwards values to exact global or local word");
}

void PointerEntryValueChecksBoundsAndDereferences() {
    const pal4::GameSystemData game(0x1000u);
    Runtime runtime;
    Check(game.PointerEntryValue(-1, runtime) == -1 && runtime.calls.empty(),
          "negative index returns before memory access");
    Check(game.PointerEntryValue(0, runtime) == -1,
          "null vector begin returns minus one");
    runtime.memory[0x1158u] = 0x4000u;
    runtime.memory[0x115cu] = 0x4008u;
    runtime.memory[0x4000u] = 0x5000u;
    runtime.memory[0x4004u] = 0x5010u;
    runtime.memory[0x5000u] = 123u;
    runtime.memory[0x5010u] = 0xfffffffdu;
    Check(game.PointerEntryValue(0, runtime) == 123 &&
              game.PointerEntryValue(1, runtime) == -3 &&
              game.PointerEntryValue(2, runtime) == -1,
          "valid slots dereference twice and end index is excluded");
}

}  // namespace

int main() {
    try {
        InitializerPreservesLayoutAndOrder();
        CoreUsesActualCollectionAndHandlesAllocationFailure();
        StorageCanPrecedeUiTimer();
        ExceptionsReleaseConstructedState();
        EntryAccessSplitsAtSeven();
        PointerEntryValueChecksBoundsAndDereferences();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
