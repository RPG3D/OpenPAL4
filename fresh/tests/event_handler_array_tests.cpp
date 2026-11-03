#include "pal4/event_handler_array.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IEventHandlerArrayRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t allocated_bytes = 0u;
    std::uint32_t freed = 0xffffffffu;
    std::uint32_t invoked_object = 0u;
    std::uint32_t invoked_slot = 0u;
    std::vector<std::uint32_t> visited;
    std::uint32_t predicate_match = 0u;
    std::uint32_t predicate_slot = 0u;
    bool extend_during_visit = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocated_bytes = bytes;
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    std::uint32_t InvokeVirtual(std::uint32_t object,
                                std::uint32_t slot) override {
        calls.emplace_back("invoke");
        invoked_object = object;
        invoked_slot = slot;
        if (slot == 0x14u) {
            visited.push_back(object);
            if (extend_during_visit && visited.size() == 1u) {
                words[0x7008u] = 0x8008u;
                words[0x1008u] = 0x700cu;
            }
        }
        return 0x87654321u;
    }
    std::uint8_t InvokePredicate(std::uint32_t object,
                                 std::uint32_t slot,
                                 std::uint32_t) override {
        calls.push_back("predicate:" + std::to_string(object) + ":" +
                        std::to_string(slot));
        return object == predicate_match && slot == predicate_slot;
    }
};

void CheckInsertEmpty() {
    Runtime runtime;
    Check(pal4::EventHandlerArray::Insert(0x1000u, 0x7000u, runtime) ==
              0x87654321u,
          "empty insert return");
    Check(runtime.allocated_bytes == 4u && runtime.freed == 0u,
          "empty allocation and old buffer release");
    Check(runtime.words[0x3000u] == 0x7000u &&
          runtime.words[0x1004u] == 0x3000u &&
          runtime.words[0x1008u] == 0x3004u &&
          runtime.words[0x100cu] == 0x3004u,
          "empty insert fields");
    Check(runtime.invoked_object == 0x7000u &&
          runtime.invoked_slot == 0xcu &&
          runtime.calls.back() == "invoke",
          "insert callback follows array update");
}

void CheckInsertWithSpace() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x1008u] = 0x2004u;
    runtime.words[0x100cu] = 0x200cu;
    runtime.words[0x2000u] = 0x6000u;
    Check(pal4::EventHandlerArray::Insert(0x1000u, 0x7000u, runtime) ==
              0x87654321u,
          "spare capacity insert return");
    Check(runtime.words[0x2004u] == 0x7000u &&
          runtime.words[0x1008u] == 0x2008u &&
          runtime.words[0x1004u] == 0x2000u,
          "spare capacity append");
    Check(runtime.allocated_bytes == 0u &&
          runtime.freed == 0xffffffffu,
          "spare capacity does not reallocate");
}

void CheckGrowth(std::uint32_t count) {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x1008u] = 0x2000u + count * 4u;
    runtime.words[0x100cu] = runtime.words[0x1008u];
    for (std::uint32_t i = 0; i < count; ++i)
        runtime.words[0x2000u + i * 4u] = 0x6000u + i;
    Check(pal4::EventHandlerArray::Insert(0x1000u, 0x7000u, runtime) ==
              0x87654321u,
          "growth insert return");
    const auto capacity = count + (count > 1u ? count : 1u);
    Check(runtime.allocated_bytes == 4u * capacity &&
          runtime.freed == 0x2000u &&
          runtime.words[0x1004u] == 0x3000u &&
          runtime.words[0x1008u] == 0x3000u + 4u * (count + 1u) &&
          runtime.words[0x100cu] == 0x3000u + 4u * capacity,
          "growth capacity and pointers");
    for (std::uint32_t i = 0; i < count; ++i)
        Check(runtime.words[0x3000u + i * 4u] == 0x6000u + i,
              "growth preserves old pointer order");
    Check(runtime.words[0x3000u + count * 4u] == 0x7000u,
          "growth appends new handler");
}

Runtime ThreeHandlers() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x1008u] = 0x200cu;
    runtime.words[0x100cu] = 0x2010u;
    runtime.words[0x2000u] = 0x6000u;
    runtime.words[0x2004u] = 0x6001u;
    runtime.words[0x2008u] = 0x6002u;
    return runtime;
}

void CheckRemove() {
    {
        auto runtime = ThreeHandlers();
        Check(pal4::EventHandlerArray::Remove(0x1000u, 0x6001u,
                                               runtime) == 0x87654321u,
              "middle removal return");
        Check(runtime.words[0x1008u] == 0x2004u &&
              runtime.invoked_object == 0x6000u &&
              runtime.invoked_slot == 0x10u,
              "middle match truncates tail and calls remaining last");
    }
    {
        auto runtime = ThreeHandlers();
        Check(pal4::EventHandlerArray::Remove(0x1000u, 0x6000u,
                                               runtime) == 0u,
              "first removal returns zero remaining count");
        Check(runtime.words[0x1008u] == 0x2000u &&
              runtime.invoked_object == 0u,
              "first match leaves no callback target");
    }
    {
        auto runtime = ThreeHandlers();
        Check(pal4::EventHandlerArray::Remove(0x1000u, 0x9999u,
                                               runtime) == 0x87654321u,
              "missing removal return");
        Check(runtime.words[0x1008u] == 0x200cu &&
              runtime.invoked_object == 0x6002u,
              "missing handler still invokes last element");
    }
    {
        auto runtime = ThreeHandlers();
        runtime.words[0x1008u] = runtime.words[0x1004u];
        Check(pal4::EventHandlerArray::Remove(0x1000u, 0x9999u,
                                               runtime) == 0u &&
                  runtime.invoked_object == 0u,
              "allocated empty array returns zero without callback");
    }
    {
        Runtime runtime;
        Check(pal4::EventHandlerArray::Remove(0x1000u, 0x9999u,
                                               runtime) == 0u,
              "empty null array result");
        Check(runtime.invoked_object == 0u, "empty array has no callback");
    }
}

void CheckQueries() {
    auto runtime = ThreeHandlers();
    Check(pal4::EventHandlerArray::GetLast(0x1000u, runtime) == 0x6002u,
          "last handler result");
    runtime.predicate_match = 0x6002u;
    runtime.predicate_slot = 4u;
    Check(pal4::EventHandlerArray::Contains(0x1000u, 0x9000u, runtime),
          "last handler special predicate");

    runtime.calls.clear();
    runtime.predicate_match = 0x6001u;
    runtime.predicate_slot = 8u;
    Check(pal4::EventHandlerArray::Contains(0x1000u, 0x9000u, runtime),
          "reverse scan predicate");
    std::vector<std::string> predicates;
    for (const auto& call : runtime.calls)
        if (call.find("predicate:") == 0u) predicates.push_back(call);
    Check(predicates == std::vector<std::string>{
              "predicate:24578:4", "predicate:24578:8",
              "predicate:24577:8"},
          "special last check precedes reverse inclusive scan");

    runtime.predicate_match = 0u;
    Check(!pal4::EventHandlerArray::Contains(0x1000u, 0x9000u, runtime),
          "no predicate match");
    Runtime empty;
    Check(pal4::EventHandlerArray::GetLast(0x1000u, empty) == 0u &&
          !pal4::EventHandlerArray::Contains(0x1000u, 0x9000u, empty),
          "null array query");
    empty.words[0x1004u] = 0x7000u;
    empty.words[0x1008u] = 0x7000u;
    Check(pal4::EventHandlerArray::GetLast(0x1000u, empty) == 0u,
          "allocated empty array has no last element");
}

void CheckForEach() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x7000u;
    runtime.words[0x1008u] = 0x7008u;
    runtime.words[0x7000u] = 0x8000u;
    runtime.words[0x7004u] = 0x8004u;
    runtime.extend_during_visit = true;
    Check(pal4::EventHandlerArray::ForEach(0x1000u, runtime) == 0x700cu,
          "foreach returns final reloaded end");
    Check(runtime.visited == std::vector<std::uint32_t>{
              0x8000u, 0x8004u, 0x8008u} &&
          runtime.invoked_slot == 0x14u,
          "foreach includes entry appended during first callback");
    Runtime empty;
    Check(pal4::EventHandlerArray::ForEach(0x1000u, empty) == 0u &&
          empty.visited.empty(), "empty foreach");
}

}  // namespace

int main() {
    try {
        CheckInsertEmpty();
        CheckInsertWithSpace();
        CheckGrowth(1u);
        CheckGrowth(3u);
        CheckRemove();
        CheckQueries();
        CheckForEach();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
