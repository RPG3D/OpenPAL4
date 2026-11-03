#include "pal4/timed_callback_collection.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ITimedCallbackCollectionRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> deletes;
    std::vector<std::uint32_t> writes;
    std::vector<std::uint32_t> compared_lengths;
    std::vector<std::string> events;
    std::vector<std::uint32_t> freed_nodes;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        writes.push_back(address);
        bytes[address] = value;
    }
    void DeleteOwned(std::uint32_t address, std::uint32_t flags) override {
        deletes.emplace_back(address, flags);
        events.emplace_back("delete");
    }
    void InvokeEvent(std::uint32_t callback) override {
        Check(callback != 0u, "event callback is nonnull");
        events.emplace_back("event");
    }
    void InvokeTimer(std::uint32_t callback) override {
        Check(callback != 0u, "timer callback is nonnull");
        events.emplace_back("timer");
    }
    void CleanupEventData(std::uint32_t) override {
        events.emplace_back("event cleanup");
    }
    void CleanupTimerData(std::uint32_t) override {
        events.emplace_back("timer cleanup");
    }
    void FreeNode(std::uint32_t address) override {
        freed_nodes.push_back(address);
        events.emplace_back("free");
    }
    int CompareBytes(std::uint32_t first, std::uint32_t second,
                     std::uint32_t count) override {
        compared_lengths.push_back(count);
        if (count == 0u) return 0;
        const auto& left = texts.at(first);
        const auto& right = texts.at(second);
        Check(left.size() >= count && right.size() >= count,
              "byte comparison stays within stored text");
        return std::memcmp(left.data(), right.data(), count);
    }
};

float FloatAt(Runtime& runtime, std::uint32_t address) {
    return std::bit_cast<float>(runtime.words.at(address));
}

void SetFloat(Runtime& runtime, std::uint32_t address, float value) {
    runtime.words[address] = std::bit_cast<std::uint32_t>(value);
}

void SlotBranches() {
    Runtime runtime;
    pal4::TimedCallbackCollection cleanup(runtime);
    constexpr std::uint32_t payload = 0x2008u;
    constexpr std::uint32_t primary = payload + 0x14u;
    constexpr std::uint32_t secondary_payload = 0x3008u;
    constexpr std::uint32_t secondary = secondary_payload + 0x18u;
    runtime.bytes[primary] = 1u;
    runtime.words[primary + 4u] = 0u;
    cleanup.ReleaseEvent(payload);
    Check(runtime.bytes[primary] == 1u && runtime.writes.empty() &&
              runtime.deletes.empty(),
          "null primary pointer preserves ownership byte");

    runtime.bytes[primary] = 0u;
    runtime.words[primary + 4u] = 0xa000u;
    cleanup.ReleaseEvent(payload);
    Check(runtime.bytes[primary] == 0u &&
              runtime.words[primary + 4u] == 0u &&
              runtime.deletes.empty() &&
              runtime.writes ==
                  std::vector<std::uint32_t>{primary, primary + 4u},
          "unowned primary pointer is cleared without deletion");

    runtime.bytes[secondary] = 1u;
    runtime.words[secondary + 4u] = 0xb000u;
    cleanup.ReleaseTimer(secondary_payload);
    Check(runtime.deletes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0xb000u, 1u}} &&
              runtime.bytes[secondary] == 0u &&
              runtime.words[secondary + 4u] == 0u,
          "owned secondary pointer receives deleting virtual flag one");
}

void CollectionOrder() {
    Runtime runtime;
    pal4::TimedCallbackCollection cleanup(runtime);
    pal4::TimedCallbackCollectionState state{};
    state.event_head = 0x1000u;
    state.timer_head = 0x4000u;
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x2000u] = 0x3000u;
    runtime.words[0x3000u] = 0x1000u;
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x5000u] = 0x4000u;
    runtime.bytes[0x201cu] = 1u;
    runtime.words[0x2020u] = 0xa000u;
    runtime.bytes[0x301cu] = 0u;
    runtime.words[0x3020u] = 0xb000u;
    runtime.bytes[0x5020u] = 1u;
    runtime.words[0x5024u] = 0xc000u;
    cleanup.ReleaseAll(state);
    Check(runtime.deletes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0xa000u, 1u}, {0xc000u, 1u}} &&
              runtime.words[0x2020u] == 0u &&
              runtime.words[0x3020u] == 0u &&
              runtime.words[0x5024u] == 0u &&
              runtime.words[0x1000u] == 0x2000u &&
              runtime.words[0x4000u] == 0x5000u &&
              runtime.reads.front() == 0x1000u &&
              std::find(runtime.reads.begin(), runtime.reads.end(),
                        0x4000u) != runtime.reads.end(),
          "primary list precedes secondary and links remain unchanged");
}

void EmptyCollections() {
    Runtime runtime;
    pal4::TimedCallbackCollection cleanup(runtime);
    pal4::TimedCallbackCollectionState state{};
    state.event_head = 0x1000u;
    state.timer_head = 0x4000u;
    runtime.words[0x1000u] = 0x1000u;
    runtime.words[0x4000u] = 0x4000u;
    cleanup.ReleaseAll(state);
    Check(runtime.reads ==
              std::vector<std::uint32_t>{0x1000u, 0x4000u} &&
              runtime.writes.empty() && runtime.deletes.empty(),
          "empty lists require only their sentinel reads");
}

void NameMatches() {
    Runtime runtime;
    pal4::TimedCallbackCollection cleanup(runtime);
    pal4::TimedCallbackCollectionState state{};
    state.timer_head = 0x4000u;
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x5000u] = 0x6000u;
    runtime.words[0x6000u] = 0x7000u;
    runtime.words[0x7000u] = 0x4000u;
    runtime.words[0x500cu] = 0x9100u;
    runtime.words[0x5010u] = 5u;
    runtime.words[0x600cu] = 0x9200u;
    runtime.words[0x6010u] = 6u;
    runtime.words[0x700cu] = 0x9300u;
    runtime.words[0x7010u] = 5u;
    runtime.texts[0x9000u] = "spell";
    runtime.texts[0x9100u] = "spell";
    runtime.texts[0x9200u] = "spells";
    runtime.texts[0x9300u] = "spall";
    runtime.bytes[0x5020u] = 1u;
    runtime.words[0x5024u] = 0xa000u;
    runtime.bytes[0x6020u] = 1u;
    runtime.words[0x6024u] = 0xb000u;
    runtime.bytes[0x7020u] = 1u;
    runtime.words[0x7024u] = 0xc000u;
    Check(cleanup.ReleaseTimerByName(
              state, pal4::TimedCallbackName{0u, 0x9000u, 5u}) == 1u &&
              runtime.deletes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0xa000u, 1u}} &&
              runtime.words[0x5024u] == 0u &&
              runtime.words[0x6024u] == 0xb000u &&
              runtime.words[0x7024u] == 0xc000u &&
              runtime.compared_lengths ==
                  std::vector<std::uint32_t>{5u, 5u, 5u},
          "secondary name matching requires equal bytes and total length");

    state.event_head = 0x1000u;
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x2000u] = 0x1000u;
    runtime.words[0x200cu] = 0x9400u;
    runtime.words[0x2010u] = 0u;
    runtime.bytes[0x201cu] = 1u;
    runtime.words[0x2020u] = 0xd000u;
    Check(cleanup.ReleaseEventByName(
              state, pal4::TimedCallbackName{0u, 0u, 0u}) == 1u &&
              runtime.deletes.back() ==
                  std::pair<std::uint32_t, std::uint32_t>{0xd000u, 1u} &&
              runtime.words[0x2020u] == 0u &&
              runtime.compared_lengths.back() == 0u,
          "null query data uses empty-string fallback for zero length");
}

void TimedUpdate() {
    Runtime runtime;
    pal4::TimedCallbackCollection cleanup(runtime);
    pal4::TimedCallbackCollectionState state{};
    state.event_head = 0x1000u;
    state.event_count = 1u;
    state.timer_head = 0x4000u;
    state.timer_count = 1u;
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2000u] = 0x1000u;
    runtime.words[0x2004u] = 0x1000u;
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x4004u] = 0x5000u;
    runtime.words[0x5000u] = 0x4000u;
    runtime.words[0x5004u] = 0x4000u;
    runtime.bytes[0x201cu] = 1u;
    runtime.words[0x2020u] = 0xa000u;
    runtime.words[0x5024u] = 0xb000u;
    SetFloat(runtime, 0x2018u, 1.5f);
    SetFloat(runtime, 0x5018u, 0.5f);
    SetFloat(runtime, 0x501cu, 2.0f);

    cleanup.Update(state, 0.0f);
    Check(runtime.reads.empty() && runtime.events.empty() &&
              FloatAt(runtime, 0x2018u) == 1.5f,
          "zero delta skips both lists before any memory read");
    cleanup.Update(state, 0.5f);
    Check(FloatAt(runtime, 0x2018u) == 1.0f &&
              FloatAt(runtime, 0x5018u) == 0.0f &&
              runtime.events.empty() && state.event_count == 1u &&
              state.timer_count == 1u,
          "remaining time equal to delta subtracts without callbacks");
    cleanup.Update(state, 0.25f);
    Check(FloatAt(runtime, 0x2018u) == 0.75f &&
              FloatAt(runtime, 0x5018u) == 1.75f &&
              runtime.events == std::vector<std::string>{"timer"},
          "timer callback uses initial period minus frame overrun");
    runtime.events.clear();
    cleanup.Update(state, 1.0f);
    Check(runtime.events ==
              std::vector<std::string>{"event", "delete"} &&
              runtime.words.at(0x2020u) == 0u &&
              FloatAt(runtime, 0x2018u) == 0.75f &&
              FloatAt(runtime, 0x5018u) == 0.75f &&
              state.event_count == 1u,
          "expired event calls slot zero then releases owned callback");
    runtime.events.clear();
    cleanup.Update(state, 0.5f);
    Check(runtime.events ==
              std::vector<std::string>{"event cleanup", "free"} &&
              runtime.freed_nodes ==
                  std::vector<std::uint32_t>{0x2000u} &&
              runtime.words.at(0x1000u) == 0x1000u &&
              runtime.words.at(0x1004u) == 0x1000u &&
              state.event_count == 0u &&
              FloatAt(runtime, 0x5018u) == 0.25f,
          "event node is unlinked and freed on the following update");
    runtime.events.clear();
    SetFloat(runtime, 0x501cu, 0.25f);
    cleanup.Update(state, 1.0f);
    Check(runtime.events == std::vector<std::string>{"timer"} &&
              FloatAt(runtime, 0x5018u) == 0.0f &&
              state.timer_count == 1u,
          "timer overrun beyond initial period clamps remaining time to zero");
    runtime.words[0x5024u] = 0u;
    runtime.events.clear();
    cleanup.Update(state, 1.0f);
    Check(runtime.events ==
              std::vector<std::string>{"timer cleanup", "free"} &&
              runtime.freed_nodes ==
                  std::vector<std::uint32_t>{0x2000u, 0x5000u} &&
              runtime.words.at(0x4000u) == 0x4000u &&
              runtime.words.at(0x4004u) == 0x4000u &&
              state.timer_count == 0u,
          "null timer callback unlinks and frees its node");
}

void FloatAndDestructorEdges() {
    Runtime runtime;
    pal4::TimedCallbackCollection cleanup(runtime);
    pal4::TimedCallbackCollectionState state{};
    state.event_head = 0x1000u;
    state.timer_head = 0x4000u;
    runtime.words[0x1000u] = 0x1000u;
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x5000u] = 0x4000u;
    runtime.words[0x5024u] = 0xb000u;
    SetFloat(runtime, 0x5018u, 1.0f);
    SetFloat(runtime, 0x501cu, 2.0f);
    cleanup.Update(state, -0.5f);
    Check(FloatAt(runtime, 0x5018u) == 1.5f &&
              runtime.events.empty(),
          "negative delta still updates and increases remaining time");
    cleanup.Update(state, std::numeric_limits<float>::quiet_NaN());
    Check(std::isnan(FloatAt(runtime, 0x5018u)) && runtime.events.empty(),
          "unordered delta takes subtraction path without callback");
    runtime.events.clear();
    Check(cleanup.DestroyEventPayload(0x6008u, 0u) == 0x6008u &&
              runtime.events ==
                  std::vector<std::string>{"event cleanup"},
          "deleting wrapper with zero flag only cleans payload");
    runtime.events.clear();
    Check(cleanup.DestroyEventPayload(0x6008u, 3u) == 0x6008u &&
              runtime.events ==
                  std::vector<std::string>{"event cleanup", "free"} &&
              runtime.freed_nodes.back() == 0x6008u,
          "deleting wrapper frees payload when low flag bit is set");
}

}  // namespace

int main() {
    try {
        SlotBranches();
        CollectionOrder();
        EmptyCollections();
        NameMatches();
        TimedUpdate();
        FloatAndDestructorEdges();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

