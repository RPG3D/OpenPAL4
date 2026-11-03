#include "pal4/player_manager_slot_processor.h"

#include <cstdlib>
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

std::size_t IndexOf(const std::vector<std::string>& events,
                    const std::string& wanted) {
    for (std::size_t i = 0u; i < events.size(); ++i)
        if (events[i] == wanted) return i;
    Check(false, "expected process event missing");
    return events.size();
}

struct CopyRuntime final : pal4::PlayerManagerSlotCopyRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string>* events{};
    bool announced{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        if (!announced) {
            events->emplace_back("copy");
            announced = true;
        }
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
};

struct Runtime final : pal4::PlayerManagerSlotProcessRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    ::CopyRuntime copy;
    std::vector<std::uint32_t> first_values{10u};
    std::vector<std::uint32_t> second_values{5u};
    std::size_t first_calls{};
    std::size_t second_calls{};
    std::uint32_t property_result{0x9876u};

    Runtime() { copy.events = &events; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void ClearCollectionFirstFive(std::uint32_t address) override {
        Check(address == 0x5938u, "first collection cleanup address");
        events.emplace_back("clear-five");
    }
    void ClearFixedSortedValues(std::uint32_t address) override {
        Check(address == 0x5938u, "sorted values cleanup address");
        events.emplace_back("clear-sorted");
    }
    void ResetInventorySlots(std::uint32_t address) override {
        Check(address == 0x5920u, "inventory slots address");
        events.emplace_back("reset-inventory");
    }
    void CleanupScriptLists(std::uint32_t address) override {
        Check(address == 0x5654u, "script member address");
        events.emplace_back("cleanup-script");
    }
    void InitializeMember(std::uint32_t address) override {
        Check(address == 0x5658u || address == 0x57a8u,
              "record member initializer address");
        events.push_back(address == 0x5658u ? "initialize-first"
                                             : "initialize-second");
    }
    std::uint32_t GameDatabase() override {
        events.emplace_back("database");
        return 0x8000u;
    }
    std::uint32_t FindRecord(std::uint32_t table,
                             std::uint32_t key) override {
        Check(table == 0x8020u && key == 0x66u,
              "record search uses GameDB +0x20 and +0xa48 key");
        events.emplace_back("find-record");
        return 0x9000u;
    }
    pal4::IPlayerManagerSlotCopyRuntime& CopyRuntime() override {
        return copy;
    }
    void ProcessCollection(std::uint32_t object) override {
        Check(object == 0x5000u, "collection processing object");
        events.emplace_back("process-collection");
    }
    void UpdateEvent(std::uint32_t script, std::uint32_t mode,
                     std::uint32_t key) override {
        Check(script == 0x5654u && mode == 1u && key == 0x66u,
              "script event arguments");
        events.emplace_back("event");
    }
    void FinalizeScript(std::uint32_t script) override {
        Check(script == 0x5654u, "script vtable +0x114 target");
        events.emplace_back("finalize-script");
    }
    std::uint32_t GetElementByIndex(std::uint32_t data,
                                    std::uint32_t index) override {
        Check(data == 0x57a8u, "property data member address");
        events.push_back("index:" + std::to_string(index));
        if (index == 0u) {
            const auto position = first_calls++;
            return first_values[position < first_values.size()
                                    ? position : first_values.size() - 1u];
        }
        Check(index == 2u, "only indices zero and two are queried");
        const auto position = second_calls++;
        return second_values[position < second_values.size()
                                 ? position : second_values.size() - 1u];
    }
    std::uint32_t SetProperty(std::uint32_t data,
                              std::uint32_t index,
                              std::uint32_t value) override {
        Check(data == 0x57a8u && index == 8u && value == 0u,
              "final property setter operands");
        events.emplace_back("set-property");
        return property_result;
    }
};

void Setup(Runtime& runtime, std::int32_t first,
           std::int32_t second) {
    runtime.words[0x5a48u] = 0x66u;
    runtime.words[0x57acu] = static_cast<std::uint32_t>(first);
    runtime.words[0x57b4u] = static_cast<std::uint32_t>(second);
    for (std::uint32_t i = 0u; i < 0x24u; ++i)
        runtime.copy.words[0x9000u + i * 4u] = 0xa000u + i;
}

void ProcessingOrderAndRepeatedLimits() {
    Runtime runtime;
    Setup(runtime, 12, 7);
    runtime.first_values = {10u, 9u};
    runtime.second_values = {5u, 4u};
    Check(pal4::PlayerManagerSlotProcessor::Process(
              0x5000u, runtime) == 0x9876u,
          "process forwards final property setter result");
    Check(runtime.events[0] == "clear-five" &&
              runtime.events[1] == "clear-sorted" &&
              runtime.events[2] == "reset-inventory" &&
              runtime.events[3] == "cleanup-script" &&
              runtime.events[4] == "initialize-first" &&
              runtime.events[5] == "initialize-second" &&
              IndexOf(runtime.events, "database") <
                  IndexOf(runtime.events, "find-record") &&
              IndexOf(runtime.events, "find-record") <
                  IndexOf(runtime.events, "copy") &&
              IndexOf(runtime.events, "copy") <
                  IndexOf(runtime.events, "process-collection") &&
              IndexOf(runtime.events, "process-collection") <
                  IndexOf(runtime.events, "event") &&
              IndexOf(runtime.events, "event") <
                  IndexOf(runtime.events, "finalize-script"),
          "reset, GameDB copy, collection, event and script order");
    Check(runtime.copy.words[0x5658u] == 0xa000u &&
              runtime.copy.words[0x56e4u] == 0xa023u,
          "process composes the reconstructed 0x90-byte copy");
    Check(runtime.words[0x5890u] == 9u &&
              runtime.words[0x5898u] == 4u &&
              runtime.first_calls == 2u && runtime.second_calls == 2u &&
              runtime.events.back() == "set-property",
          "values above limits use a second getter result before property eight");
}

void NegativeAndInRangeValues() {
    Runtime negative;
    Setup(negative, -2, -3);
    Check(pal4::PlayerManagerSlotProcessor::Process(
              0x5000u, negative) == 0x9876u &&
              negative.words[0x5890u] == 0u &&
              negative.words[0x5898u] == 0u &&
              negative.first_calls == 1u && negative.second_calls == 1u,
          "negative values under positive limits are replaced with zero");

    Runtime inside;
    Setup(inside, 5, 3);
    Check(pal4::PlayerManagerSlotProcessor::Process(
              0x5000u, inside) == 0x9876u &&
              inside.words[0x5890u] == 5u &&
              inside.words[0x5898u] == 3u &&
              inside.first_calls == 1u && inside.second_calls == 1u,
          "in-range values remain unchanged");
}

void SignedUpperBoundPrecedesZeroFloor() {
    Runtime runtime;
    Setup(runtime, -2, -3);
    runtime.first_values = {static_cast<std::uint32_t>(-4),
                            static_cast<std::uint32_t>(-5)};
    runtime.second_values = {static_cast<std::uint32_t>(-6),
                             static_cast<std::uint32_t>(-7)};
    Check(pal4::PlayerManagerSlotProcessor::Process(
              0x5000u, runtime) == 0x9876u &&
              static_cast<std::int32_t>(runtime.words[0x5890u]) == -5 &&
              static_cast<std::int32_t>(runtime.words[0x5898u]) == -7,
          "signed value above a negative upper bound follows the upper branch");
}

void DefaultRuntimeComposesCollectionProcessor() {
    pal4::PlayerManagerSlotProcessRuntimeStub runtime;
    Check(pal4::PlayerManagerSlotProcessor::Process(
              0x5000u, runtime) == 0u,
          "default runtime links and executes collection processor path");
}

}  // namespace

int main() {
    ProcessingOrderAndRepeatedLimits();
    NegativeAndInRangeValues();
    SignedUpperBoundPrecedesZeroFloor();
    DefaultRuntimeComposesCollectionProcessor();
    std::cout << "4 player-manager slot processing groups passed\n";
}
