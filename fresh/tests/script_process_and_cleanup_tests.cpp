#include "pal4/script_process_and_cleanup.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ScriptProcessRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::int32_t comparison{1};
    bool second_name_null{};

    Runtime() {
        words[0x5a1cu] = 0x7000u;
        words[0x500cu] = 0x7100u;
        words[0x5004u] = 0x7200u;
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            words[0x50acu + index * 4u] = 20u + index;
            words[0x50a0u + index * 4u] = 40u + index;
        }
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t EmptyStringAddress() override {
        events.push_back("empty-name");
        return 0x8000u;
    }
    std::int32_t CompareCaseInsensitive(
        std::uint32_t left, std::uint32_t right) override {
        Check(left == 0x7100u &&
                  right == (words[0x5a1cu] == 0u ? 0x8000u : 0x7000u),
              "comparison uses current script name or empty fallback");
        events.push_back("compare");
        return comparison;
    }
    void CopyName(char* destination, std::uint32_t source,
                  std::uint32_t count) override {
        Check(source == 0x7200u && count == 0x100u &&
                  std::all_of(destination, destination + count,
                              [](char byte) { return byte == 0; }),
              "256-byte destination is zeroed before copy");
        std::strncpy(destination, "Tianhe", count);
        events.push_back("copy-name");
    }
    void ResetObject(std::uint32_t object) override {
        Check(object == 0x5000u, "reset target");
        events.push_back("reset");
        words[0x5a1cu] = second_name_null ? 0u : 0x7300u;
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            words[0x50acu + index * 4u] = 100u;
            words[0x50a0u + index * 4u] = 200u;
        }
    }
    void AddAndProcessScript() override {
        events.push_back("add-script");
    }
    void InitializeObject(std::uint32_t object, const char* name,
                          std::uint32_t script_name,
                          std::uint32_t mode) override {
        Check(object == 0x5000u && std::strcmp(name, "Tianhe") == 0 &&
                  script_name == (second_name_null ? 0x8000u : 0x7300u) &&
                  mode == 1u,
              "object is initialized with saved name and refreshed script name");
        events.push_back("initialize");
    }
    void CleanupAllScripts() override {
        events.push_back("cleanup-scripts");
    }
    void ApplyFirst(std::uint32_t object,
                    const std::array<std::uint32_t, 3>& values,
                    std::uint32_t mode) override {
        Check(object == 0x5000u && mode == 0u &&
                  values == std::array<std::uint32_t, 3>{20u, 21u, 22u},
              "first virtual restores second saved triple");
        events.push_back("apply-first");
    }
    std::int32_t ApplySecond(
        std::uint32_t object,
        const std::array<std::uint32_t, 3>& values,
        std::uint32_t mode) override {
        Check(object == 0x5000u && mode == 0u &&
                  values == std::array<std::uint32_t, 3>{40u, 41u, 42u},
              "second virtual restores first saved triple");
        events.push_back("apply-second");
        return -13;
    }
};

std::size_t Position(const std::vector<std::string>& events,
                     const std::string& event) {
    const auto it = std::find(events.begin(), events.end(), event);
    Check(it != events.end(), "expected event exists");
    return static_cast<std::size_t>(it - events.begin());
}

void TestEqualNameSkipsReload() {
    Runtime runtime;
    runtime.comparison = 0;
    Check(!pal4::ScriptProcessAndCleanup::NamesDiffer(0x5000u, runtime),
          "comparison helper returns false for matching names");
    runtime.events.clear();
    Check(pal4::ScriptProcessAndCleanup::Process(0x5000u, runtime) == 0,
          "matching name returns comparison zero");
    Check(runtime.events.size() == 3u &&
              runtime.events.back() == "compare",
          "matching name performs no reset or script work");
}

void TestReloadAndRestore() {
    Runtime runtime;
    Check(pal4::ScriptProcessAndCleanup::Process(0x5000u, runtime) == -13,
          "second virtual result is forwarded");
    Check(Position(runtime.events, "read:20660") <
              Position(runtime.events, "read:20640") &&
              Position(runtime.events, "read:20648") <
                  Position(runtime.events, "reset") &&
              Position(runtime.events, "reset") <
                  Position(runtime.events, "add-script") &&
              Position(runtime.events, "add-script") <
                  Position(runtime.events, "initialize") &&
              Position(runtime.events, "initialize") <
                  Position(runtime.events, "cleanup-scripts") &&
              Position(runtime.events, "cleanup-scripts") <
                  Position(runtime.events, "apply-first") &&
              Position(runtime.events, "apply-first") <
                  Position(runtime.events, "apply-second"),
          "saved triples and reload call order");
}

void TestEmptyNameFallbackIsReread() {
    Runtime runtime;
    runtime.words[0x5a1cu] = 0u;
    runtime.second_name_null = true;
    static_cast<void>(pal4::ScriptProcessAndCleanup::Process(
        0x5000u, runtime));
    Check(std::count(runtime.events.begin(), runtime.events.end(),
                     "empty-name") == 2,
          "null script name uses fallback before compare and after reset");
}

void TestNameDifferenceHelper() {
    Runtime runtime;
    runtime.comparison = -9;
    Check(pal4::ScriptProcessAndCleanup::NamesDiffer(0x5000u, runtime),
          "negative comparison returns true");
    runtime.comparison = 7;
    runtime.words[0x5a1cu] = 0u;
    Check(pal4::ScriptProcessAndCleanup::NamesDiffer(0x5000u, runtime),
          "positive comparison with null-name fallback returns true");
}

}  // namespace

int main() {
    TestEqualNameSkipsReload();
    TestReloadAndRestore();
    TestEmptyNameFallbackIsReread();
    TestNameDifferenceHelper();
}
