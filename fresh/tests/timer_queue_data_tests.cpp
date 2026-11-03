#include "pal4/timer_queue_data.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ITimerQueueDataRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint8_t> counts;
    bool allocate_ok = true;
    std::uint32_t allocated_length = 0xffffffffu;
    std::uint32_t copied_source = 0u;
    std::uint32_t copied_length = 0xffffffffu;

    void AssignName(pal4::TimerQueueEntryState& destination,
                    const pal4::TimerQueueEntryState& source,
                    std::uint32_t offset, std::uint32_t count) override {
        Check(offset == 0u && count == 0xffffffffu,
              "copy uses entire source name");
        calls.emplace_back("assign-name");
        destination.name = source.name;
    }
    void EraseName(pal4::TimerQueueEntryState&,
                   std::uint32_t offset, std::uint32_t count) override {
        calls.emplace_back("erase:" + std::to_string(offset) + ":" +
                           std::to_string(count));
    }
    void SetTextFlag(pal4::TimerQueueEntryState& entry,
                     std::uint8_t flag) override {
        calls.emplace_back("set-flag");
        entry.text_flag = flag;
    }
    std::uint8_t ReadReferenceCount(std::uint32_t data) override {
        calls.emplace_back("read-count");
        return counts.at(data);
    }
    void WriteReferenceCount(std::uint32_t data,
                             std::uint8_t count) override {
        calls.emplace_back("write-count");
        counts[data] = count;
    }
    bool AllocateName(pal4::TimerQueueEntryState& entry,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(mode == 1u, "allocation mode");
        calls.emplace_back("allocate");
        allocated_length = length;
        if (allocate_ok) {
            entry.name.data = 0x3001u;
            entry.name.capacity = length + 1u;
        }
        return allocate_ok;
    }
    void CopyName(std::uint32_t destination, std::uint32_t source,
                  std::uint32_t length) override {
        Check(destination == 0x3001u, "allocated destination used");
        calls.emplace_back("copy-name");
        copied_source = source;
        copied_length = length;
    }
    void SetNameSize(pal4::TimerQueueEntryState& entry,
                     std::uint32_t length) override {
        calls.emplace_back("set-size");
        entry.name.length = length;
    }
    void ReleaseCallback(std::uint32_t, std::uint32_t) override {
        Check(false, "unexpected callback release");
    }
    std::uint32_t FreeName(std::uint32_t) override {
        Check(false, "unexpected name free");
        return 0u;
    }
};

pal4::TimerQueueEntryState Source() {
    pal4::TimerQueueEntryState source{};
    source.text_flag = 4u;
    source.name = {0x2001u, 5u, 8u};
    source.initial_period_bits = 0x3f800000u;
    source.current_period_bits = 0x3e800000u;
    source.owns_callback = true;
    source.callback = 0x5000u;
    return source;
}

void CheckPayload(const pal4::TimerQueueEntryState& entry) {
    Check(entry.initial_period_bits == 0x3f800000u &&
              entry.current_period_bits == 0x3e800000u &&
              entry.callback == 0x5000u,
          "periods and callback copied without conversion");
}

void CheckCopy() {
    Runtime runtime;
    auto source = Source();
    auto destination = Source();
    destination.name = {0x9000u, 9u, 9u};
    pal4::TimerQueueData::Copy(&destination, source, runtime);
    Check(destination.text_flag == 4u &&
              destination.name.data == 0x2001u &&
              destination.name.length == 5u &&
              destination.name.capacity == 8u &&
              destination.owns_callback && !source.owns_callback,
          "copy transfers callback ownership and assigns name");
    CheckPayload(destination);
    Check(runtime.calls == std::vector<std::string>{"assign-name"},
          "copy delegates only the string assignment");

    Runtime null_runtime;
    auto untouched = Source();
    pal4::TimerQueueData::Copy(nullptr, untouched, null_runtime);
    Check(untouched.owns_callback && null_runtime.calls.empty(),
          "null destination has no side effects");

    Runtime alias_runtime;
    auto aliased = Source();
    pal4::TimerQueueData::Copy(&aliased, aliased, alias_runtime);
    Check(aliased.name.data == 0u && !aliased.owns_callback &&
              alias_runtime.calls ==
                  std::vector<std::string>{"assign-name"},
          "copying to the same address observes the cleared name");
}

void CheckAssignShared() {
    Runtime runtime;
    runtime.counts[0x2001u] = 0xfdu;
    auto source = Source();
    pal4::TimerQueueEntryState destination{};
    Check(&pal4::TimerQueueData::Assign(
              destination, source, 0x1000u, runtime) == &destination,
          "assign returns destination");
    Check(destination.text_flag == 1u &&
              destination.name.data == 0x2001u &&
              destination.name.length == 5u &&
              destination.name.capacity == 8u &&
              runtime.counts[0x2001u] == 0xfeu &&
              destination.owns_callback && !source.owns_callback,
          "shareable string increments count and transfers callback");
    CheckPayload(destination);
    Check(runtime.calls ==
              std::vector<std::string>{"read-count", "set-flag",
                                       "write-count"},
          "shared name uses one reference count read");
}

void CheckAssignAllocated() {
    Runtime runtime;
    runtime.counts[0x2001u] = 0xfeu;
    auto source = Source();
    pal4::TimerQueueEntryState destination{};
    (void)pal4::TimerQueueData::Assign(
        destination, source, 0x1000u, runtime);
    Check(destination.text_flag == 4u &&
              destination.name.data == 0x3001u &&
              destination.name.length == 5u &&
              runtime.allocated_length == 5u &&
              runtime.copied_source == 0x2001u &&
              runtime.copied_length == 5u &&
              runtime.counts[0x2001u] == 0xfeu &&
              destination.owns_callback && !source.owns_callback,
          "saturated source is copied into new storage");
    Check(runtime.calls ==
              std::vector<std::string>{"read-count", "allocate",
                                       "copy-name", "set-size"},
          "allocated name call order");
}

void CheckAssignEmptyAndFailed() {
    Runtime empty_runtime;
    auto source = Source();
    source.name = {};
    pal4::TimerQueueEntryState destination{};
    (void)pal4::TimerQueueData::Assign(
        destination, source, 0x1000u, empty_runtime);
    Check(empty_runtime.allocated_length == 0u &&
              empty_runtime.copied_source == 0x1000u &&
              empty_runtime.copied_length == 0u &&
              empty_runtime.calls ==
                  std::vector<std::string>{"allocate", "copy-name",
                                           "set-size"},
          "empty name allocates and uses global empty string");

    Runtime failed;
    failed.allocate_ok = false;
    failed.counts[0x2001u] = 0xffu;
    source = Source();
    destination = {};
    (void)pal4::TimerQueueData::Assign(
        destination, source, 0x1000u, failed);
    Check(destination.name.data == 0u &&
              destination.name.length == 0u &&
              destination.owns_callback && !source.owns_callback &&
              failed.calls ==
                  std::vector<std::string>{"read-count", "allocate"},
          "failed name allocation still transfers other fields");
    CheckPayload(destination);
}

void CheckAssignSelf() {
    Runtime runtime;
    auto entry = Source();
    Check(&pal4::TimerQueueData::Assign(
              entry, entry, 0x1000u, runtime) == &entry &&
              entry.name.data == 0u && !entry.owns_callback &&
              entry.callback == 0x5000u,
          "self assignment preserves callback pointer and clears ownership");
    Check(runtime.calls ==
              std::vector<std::string>{"erase:0:4294967295",
                                       "erase:0:0"},
          "self assignment erases twice after clearing name length");
}

}  // namespace

int main() {
    CheckCopy();
    CheckAssignShared();
    CheckAssignAllocated();
    CheckAssignEmptyAndFailed();
    CheckAssignSelf();
}
