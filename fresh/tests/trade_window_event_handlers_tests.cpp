#include "pal4/trade_window_event_handlers.h"

#include <map>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITradeWindowEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x2004u, 0x4000u}, {0x2008u, 0u},
        {0x2010u, 0x3000u}, {0x2014u, 0u},
        {0x3004u, 0u}, {0x4004u, 0u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {0x8000u, 2u}};
    std::map<std::uint32_t, pal4::TradeEventData> events;
    std::map<std::uint32_t, pal4::TimerQueueEntryState> timers;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> released;
    std::vector<std::string> calls;
    std::uint32_t next = 0x1000u;
    unsigned allocation_count = 0;
    unsigned fail_allocation = 0;
    bool fail_assign = false;
    unsigned assign_count = 0;
    unsigned fail_assign_at = 0;
    bool fail_count_write = false;
    std::uint32_t fail_write_address = 0u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        ++allocation_count;
        if (allocation_count == fail_allocation) return 0u;
        const auto result = next;
        next += 0x100u;
        return result;
    }
    void Free(std::uint32_t address) override { frees.push_back(address); }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (fail_count_write &&
            (address == 0x2014u || address == 0x2008u))
            throw std::runtime_error("count write");
        if (address == fail_write_address)
            throw std::runtime_error("field write");
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void AssignName(pal4::TradeEventName& destination,
                    const pal4::TradeEventName& source) override {
        calls.emplace_back("event-name");
        ++assign_count;
        if (fail_assign || assign_count == fail_assign_at)
            throw std::runtime_error("name assignment");
        destination.name = source.name;
        if (source.name.data != 0u) ++bytes[source.name.data - 1u];
    }
    void AssignTimerName(pal4::TimerQueueEntryState& destination,
                         const pal4::TimerQueueEntryState& source) override {
        calls.emplace_back("timer-name");
        ++assign_count;
        if (fail_assign || assign_count == fail_assign_at)
            throw std::runtime_error("name assignment");
        destination.name = source.name;
        if (source.name.data != 0u) ++bytes[source.name.data - 1u];
    }
    pal4::TradeEventData& EventDataAt(std::uint32_t address) override {
        return events[address];
    }
    pal4::TimerQueueEntryState& TimerDataAt(
        std::uint32_t address) override {
        return timers[address];
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(deleting == 1u, "callback deleting flag");
        released.push_back(callback);
    }
};

pal4::TradeEventName Name() {
    pal4::TradeEventName result{};
    result.text_flag = 3u;
    result.name = {0x8001u, 4u, 8u};
    return result;
}

void CheckEventInsertion() {
    Runtime runtime;
    pal4::TradeWindowEventHandlers::AddEventHandler(
        0x2000u, Name(), 0x3e4ccccdu, 0x6000u, 0x48cd20u,
        0x77u, 1u, runtime);
    Check(runtime.allocations == std::vector<std::uint32_t>{0x14u, 0x24u},
          "event allocation sizes");
    Check(runtime.words.at(0x1000u) == 0x843d8cu &&
          runtime.words.at(0x1004u) == 0x6000u &&
          runtime.words.at(0x1008u) == 0x48cd20u &&
          runtime.words.at(0x100cu) == 0x77u &&
          runtime.bytes.at(0x1010u) == 1u,
          "callback object layout");
    Check(runtime.words.at(0x3004u) == 0x1100u &&
          runtime.words.at(0x1100u) == 0x1100u &&
          runtime.words.at(0x1104u) == 0x1100u &&
          runtime.words.at(0x2014u) == 1u,
          "empty event list inserts self linked node");
    const auto& entry = runtime.events.at(0x1108u);
    Check(entry.text.text_flag == 3u && entry.text.name.data == 0x8001u &&
          entry.delay_bits == 0x3e4ccccdu && entry.owns_callback &&
          entry.callback == 0x1000u && runtime.bytes.at(0x8000u) == 3u &&
          runtime.released.empty() && runtime.frees.empty(),
          "event data receives callback ownership and name reference");

    pal4::TradeWindowEventHandlers::AddEventHandler(
        0x2000u, Name(), 5u, 1u, 2u, 3u, 0u, runtime);
    Check(runtime.words.at(0x3004u) == 0x1300u &&
          runtime.words.at(0x1300u) == 0x3000u &&
          runtime.words.at(0x1304u) == 0x1100u &&
          runtime.words.at(0x1100u) == 0x1300u &&
          runtime.words.at(0x2014u) == 2u,
          "second event node is linked before first");
}

void CheckTimerInsertion() {
    Runtime runtime;
    pal4::TradeWindowEventHandlers::AddTimerHandler(
        0x2000u, Name(), 0x3dcccccdu, 0x7000u, 0x48cd20u,
        9u, 0u, runtime);
    Check(runtime.allocations == std::vector<std::uint32_t>{0x14u, 0x28u} &&
          runtime.words.at(0x4004u) == 0x1100u &&
          runtime.words.at(0x1100u) == 0x1100u &&
          runtime.words.at(0x1104u) == 0x1100u &&
          runtime.words.at(0x2008u) == 1u,
          "timer uses 40 byte node and timer list");
    const auto& entry = runtime.timers.at(0x1108u);
    Check(entry.initial_period_bits == 0x3dcccccdu &&
          entry.current_period_bits == 0x3dcccccdu &&
          entry.owns_callback && entry.callback == 0x1000u &&
          entry.name.data == 0x8001u &&
          runtime.calls == std::vector<std::string>{"timer-name", "timer-name"} &&
          runtime.bytes.at(0x8000u) == 3u,
          "timer duplicates delay and transfers callback");
}

void CheckSkippedAndFailedAllocation() {
    Runtime ignored;
    pal4::TradeWindowEventHandlers::AddEventHandler(
        0x2000u, Name(), 1u, 2u, 0u, 3u, 4u, ignored);
    Check(ignored.allocations.empty() && ignored.calls.empty(),
          "null handler skips all work");

    Runtime no_callback;
    no_callback.fail_allocation = 1u;
    pal4::TradeWindowEventHandlers::AddEventHandler(
        0x2000u, Name(), 1u, 2u, 3u, 4u, 5u, no_callback);
    Check(no_callback.allocations ==
              std::vector<std::uint32_t>{0x14u, 0x24u} &&
          !no_callback.events.at(0x1008u).owns_callback &&
          no_callback.events.at(0x1008u).callback == 0u &&
          no_callback.words.at(0x2014u) == 1u,
          "callback allocation failure still inserts event");

    Runtime no_node;
    no_node.fail_allocation = 2u;
    bool threw = false;
    try {
        pal4::TradeWindowEventHandlers::AddEventHandler(
            0x2000u, Name(), 1u, 2u, 3u, 4u, 5u, no_node);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && no_node.words.at(0x3004u) == 0u &&
          no_node.words.at(0x2014u) == 0u &&
          no_node.released == std::vector<std::uint32_t>{0x1000u} &&
          no_node.bytes.at(0x8000u) == 2u,
          "failed node allocation releases temporary ownership");

    Runtime name_error;
    name_error.fail_assign = true;
    threw = false;
    try {
        pal4::TradeWindowEventHandlers::AddTimerHandler(
            0x2000u, Name(), 1u, 2u, 3u, 4u, 5u, name_error);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && name_error.allocations.size() == 1u &&
          name_error.released == std::vector<std::uint32_t>{0x1000u} &&
          name_error.words.at(0x2008u) == 0u,
          "name assignment exception releases allocated callback");

    Runtime copy_error;
    copy_error.fail_assign_at = 2u;
    threw = false;
    try {
        pal4::TradeWindowEventHandlers::AddEventHandler(
            0x2000u, Name(), 1u, 2u, 3u, 4u, 5u, copy_error);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && copy_error.words.at(0x3004u) == 0u &&
          copy_error.words.at(0x2014u) == 0u &&
          copy_error.frees == std::vector<std::uint32_t>{0x1100u} &&
          copy_error.released == std::vector<std::uint32_t>{0x1000u} &&
          copy_error.bytes.at(0x8000u) == 2u,
          "copy exception restores list and temporary references");

    Runtime count_error;
    count_error.fail_count_write = true;
    threw = false;
    try {
        pal4::TradeWindowEventHandlers::AddEventHandler(
            0x2000u, Name(), 1u, 2u, 3u, 4u, 5u, count_error);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && count_error.words.at(0x3004u) == 0u &&
          count_error.frees == std::vector<std::uint32_t>{0x1100u} &&
          count_error.released == std::vector<std::uint32_t>{0x1000u} &&
          count_error.bytes.at(0x8000u) == 2u,
          "failure after ownership transfer cleans inserted record");

    Runtime callback_write_error;
    callback_write_error.fail_write_address = 0x1008u;
    threw = false;
    try {
        pal4::TradeWindowEventHandlers::AddEventHandler(
            0x2000u, Name(), 1u, 2u, 3u, 4u, 5u,
            callback_write_error);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && callback_write_error.allocations.size() == 1u &&
          callback_write_error.frees ==
              std::vector<std::uint32_t>{0x1000u},
          "partial callback initialization frees allocation");
}

void CheckCopyAndCleanup() {
    Runtime runtime;
    pal4::TradeEventData source{};
    source.text = Name();
    source.delay_bits = 0x3f000000u;
    source.owns_callback = true;
    source.callback = 0x5000u;
    pal4::TradeEventData destination{};
    Check(pal4::TradeWindowEventHandlers::CopyEventData(
              &destination, source, runtime) == 0x5000u &&
          destination.delay_bits == source.delay_bits &&
          destination.text.name.data == 0x8001u &&
          destination.owns_callback && !source.owns_callback &&
          runtime.bytes.at(0x8000u) == 3u,
          "event copy increments name reference and moves ownership");
    Check(pal4::TradeWindowEventHandlers::CopyEventData(
              nullptr, source, runtime) == 0u && !source.owns_callback,
          "null copy destination is inert");
    Check(pal4::TradeWindowEventHandlers::CleanupEventData(
              destination, runtime) == 0x8000u &&
          runtime.released == std::vector<std::uint32_t>{0x5000u} &&
          runtime.bytes.at(0x8000u) == 2u &&
          destination.text.name.data == 0u &&
          destination.text.name.length == 0u,
          "cleanup releases callback and decrements shared name");

    pal4::TradeEventData unique{};
    unique.text.name = {0x8001u, 2u, 3u};
    runtime.bytes[0x8000u] = 0u;
    Check(pal4::TradeWindowEventHandlers::CleanupEventData(
              unique, runtime) == 0u &&
          runtime.frees == std::vector<std::uint32_t>{0x8000u},
          "zero reference byte frees name header");
    runtime.bytes[0x8000u] = 0xffu;
    unique.text.name = {0x8001u, 2u, 3u};
    (void)pal4::TradeWindowEventHandlers::CleanupEventData(unique, runtime);
    Check(runtime.frees.size() == 2u,
          "sentinel reference byte also frees name");
}

void CheckHandlerClones() {
    Runtime runtime;
    runtime.words[0x6004u] = 0x7000u;
    runtime.words[0x6008u] = 0x8000u;
    const auto first =
        pal4::TradeWindowEventHandlers::AllocateEventHandlerOne(
            0x6000u, runtime);
    const auto second =
        pal4::TradeWindowEventHandlers::AllocateEventHandlerTwo(
            0x6000u, runtime);
    Check(first == 0x1000u && second == 0x1100u &&
          runtime.allocations == std::vector<std::uint32_t>{0xcu, 0xcu} &&
          runtime.words.at(first) == 0x843d2cu &&
          runtime.words.at(second) == 0x843d80u &&
          runtime.words.at(first + 4u) == 0x7000u &&
          runtime.words.at(second + 8u) == 0x8000u,
          "both clone vtables preserve two source words");
    runtime.fail_allocation = 3u;
    Check(pal4::TradeWindowEventHandlers::AllocateEventHandlerOne(
              0x6000u, runtime) == 0u,
          "clone allocation failure returns null");

    Runtime clone_write_error;
    clone_write_error.words[0x6004u] = 0x7000u;
    clone_write_error.words[0x6008u] = 0x8000u;
    clone_write_error.fail_write_address = 0x1004u;
    bool threw = false;
    try {
        (void)pal4::TradeWindowEventHandlers::AllocateEventHandlerTwo(
            0x6000u, clone_write_error);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && clone_write_error.frees ==
                       std::vector<std::uint32_t>{0x1000u},
          "partial clone initialization frees allocation");
}

void CheckEventRemoval() {
    Runtime single;
    single.words[0x7000u] = 0x7100u;
    single.words[0x7004u] = 0x7100u;
    single.words[0x7100u] = 0x7000u;
    single.words[0x7104u] = 0x7000u;
    single.words[0x7208u] = 1u;
    single.events[0x7008u].text = Name();
    single.events[0x7008u].owns_callback = true;
    single.events[0x7008u].callback = 0x9000u;
    Check(pal4::TradeWindowEventHandlers::RemoveEventNode(
              0x7200u, 0x7300u, 0x7000u, single) == 0x7300u &&
              single.words.at(0x7300u) == 0x7100u &&
              single.words.at(0x7100u) == 0x7100u &&
              single.words.at(0x7104u) == 0x7100u &&
              single.words.at(0x7208u) == 0u,
          "single event node removed from circular list");
    Check(single.released == std::vector<std::uint32_t>{0x9000u} &&
              single.bytes.at(0x8000u) == 1u &&
              single.frees == std::vector<std::uint32_t>{0x7000u} &&
              single.events.at(0x7008u).text.name.data == 0u,
          "payload cleaned before node free");

    Runtime first;
    first.words[0x7000u] = 0x7400u;
    first.words[0x7004u] = 0x7100u;
    first.words[0x7100u] = 0x7000u;
    first.words[0x7404u] = 0x7000u;
    first.words[0x7208u] = 2u;
    Check(pal4::TradeWindowEventHandlers::RemoveEventNode(
              0x7200u, 0x7300u, 0x7000u, first) == 0x7300u &&
              first.words.at(0x7300u) == 0x7400u &&
              first.words.at(0x7100u) == 0x7400u &&
              first.words.at(0x7404u) == 0x7100u &&
              first.words.at(0x7208u) == 1u,
          "first of two nodes returns successor");

    Runtime failing;
    failing.words[0x7000u] = 0x7100u;
    failing.words[0x7004u] = 0x7100u;
    failing.words[0x7100u] = 0x7000u;
    failing.words[0x7104u] = 0x7000u;
    failing.words[0x7208u] = 1u;
    failing.events[0x7008u].text.name = {0x9001u, 1u, 2u};
    bool threw = false;
    try {
        (void)pal4::TradeWindowEventHandlers::RemoveEventNode(
            0x7200u, 0x7300u, 0x7000u, failing);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw && failing.words.at(0x7100u) == 0x7100u &&
              failing.words.at(0x7208u) == 1u &&
              failing.frees.empty() &&
              failing.words.find(0x7300u) == failing.words.end(),
          "cleanup failure occurs after unlink but before free and count");
}

}  // namespace

int main() {
    try {
        CheckEventInsertion();
        CheckTimerInsertion();
        CheckSkippedAndFailedAllocation();
        CheckCopyAndCleanup();
        CheckHandlerClones();
        CheckEventRemoval();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
