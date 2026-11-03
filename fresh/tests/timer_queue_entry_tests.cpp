#include "pal4/timer_queue_entry.h"

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

struct Runtime final : pal4::ITimerQueueEntryDeletingRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint8_t> counts;
    std::map<std::uint32_t, pal4::TimerQueueEntryState> entries;
    std::uint32_t copy_source = 0u;
    std::uint32_t copy_destination = 0u;
    std::uint32_t copy_size = 0u;
    std::uint32_t released = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t allocation_size = 0xffffffffu;
    bool allocate_ok = true;

    void EraseName(pal4::TimerQueueEntryState& entry,
                   std::uint32_t offset, std::uint32_t count) override {
        (void)entry;
        calls.push_back("erase:" + std::to_string(offset) + ":" +
                        std::to_string(count));
    }
    void SetTextFlag(pal4::TimerQueueEntryState& entry,
                     std::uint8_t flag) override {
        entry.text_flag = flag;
        calls.emplace_back("flag");
    }
    std::uint8_t ReadReferenceCount(std::uint32_t data) override {
        calls.emplace_back("read-count");
        return counts[data];
    }
    void WriteReferenceCount(std::uint32_t data,
                             std::uint8_t count) override {
        counts[data] = count;
        calls.emplace_back("write-count");
    }
    bool AllocateName(pal4::TimerQueueEntryState& entry,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(mode == 1u, "name allocation mode");
        allocation_size = length;
        calls.emplace_back("allocate");
        if (allocate_ok) entry.name.data = 0x3001u;
        return allocate_ok;
    }
    void CopyName(std::uint32_t destination, std::uint32_t source,
                  std::uint32_t length) override {
        copy_destination = destination;
        copy_source = source;
        copy_size = length;
        calls.emplace_back("copy");
    }
    void SetNameSize(pal4::TimerQueueEntryState& entry,
                     std::uint32_t length) override {
        entry.name.length = length;
        calls.emplace_back("size");
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override {
        Check(deleting == 1u, "callback deleting destructor flag");
        released = callback;
        calls.emplace_back("release");
    }
    std::uint32_t FreeName(std::uint32_t header) override {
        freed = header;
        calls.emplace_back("free");
        return 0x77u;
    }
    pal4::TimerQueueEntryState& EntryAt(
        std::uint32_t address) override {
        return entries.at(address);
    }
    void FreeObject(std::uint32_t address) override {
        calls.push_back("free-object:" + std::to_string(address));
    }
};

pal4::TimerQueueEntryState Source() {
    pal4::TimerQueueEntryState source;
    source.text_flag = 4u;
    source.name = {0x2001u, 15u, 20u};
    return source;
}

void CheckSharedConstruction() {
    Runtime runtime;
    runtime.counts[0x2001u] = 3u;
    auto source = Source();
    pal4::TimerQueueEntryState entry;
    Check(&pal4::TimerQueueEntry::Construct(
              entry, source, 0x3e99999au, 0x4000u, 0x1000u, runtime) ==
              &entry,
          "constructor returns same entry");
    Check(entry.text_flag == 1u && entry.name.data == 0x2001u &&
              entry.name.length == 15u && entry.name.capacity == 20u &&
              runtime.counts[0x2001u] == 4u &&
              runtime.allocation_size == 0xffffffffu,
          "shareable name increments reference count without allocation");
    Check(entry.initial_period_bits == 0x3e99999au &&
              entry.current_period_bits == 0x3e99999au &&
              entry.callback == 0x4000u && entry.owns_callback,
          "two period fields and callback ownership");
}

void CheckCopiedConstruction() {
    Runtime runtime;
    runtime.counts[0x2001u] = 0xfeu;
    auto source = Source();
    pal4::TimerQueueEntryState entry;
    (void)pal4::TimerQueueEntry::Construct(
        entry, source, 0x12345678u, 0u, 0x1000u, runtime);
    Check(runtime.allocation_size == 15u &&
              runtime.copy_destination == 0x3001u &&
              runtime.copy_source == 0x2001u &&
              runtime.copy_size == 15u && entry.name.length == 15u &&
              runtime.counts[0x2001u] == 0xfeu &&
              !entry.owns_callback,
          "saturated reference count takes copy path");

    Runtime empty_runtime;
    pal4::TimerQueueEntryState empty_source;
    (void)pal4::TimerQueueEntry::Construct(
        entry, empty_source, 1u, 0u, 0x1000u, empty_runtime);
    Check(empty_runtime.allocation_size == 0u &&
              empty_runtime.copy_source == 0x1000u &&
              empty_runtime.copy_size == 0u,
          "empty name allocates and copies from global empty string");

    Runtime failed;
    failed.allocate_ok = false;
    failed.counts[0x2001u] = 0xfeu;
    (void)pal4::TimerQueueEntry::Construct(
        entry, source, 2u, 0u, 0x1000u, failed);
    Check(entry.name.data == 0u && entry.name.length == 0u &&
              failed.copy_destination == 0u,
          "failed allocation leaves empty name and still initializes timer");
}

void CheckAliasConstruction() {
    Runtime runtime;
    auto entry = Source();
    (void)pal4::TimerQueueEntry::Construct(
        entry, entry, 7u, 0u, 0x1000u, runtime);
    Check(entry.name.data == 0u && entry.name.length == 0u &&
              runtime.calls.size() == 2u &&
              runtime.calls[0] == "erase:0:4294967295" &&
              runtime.calls[1] == "erase:0:0",
          "alias path follows observed zeroed-length erase sequence");
}

void CheckDestruction() {
    Runtime runtime;
    auto entry = Source();
    entry.owns_callback = true;
    entry.callback = 0x4000u;
    runtime.counts[0x2001u] = 2u;
    Check(pal4::TimerQueueEntry::Destroy(entry, runtime) == 0x2000u &&
              runtime.released == 0x4000u &&
              runtime.counts[0x2001u] == 1u &&
              runtime.freed == 0u && entry.name.data == 0u &&
              runtime.calls[0] == "release",
          "shared name decrements after callback release and returns header");

    for (auto count : {0u, 0xffu}) {
        Runtime final_runtime;
        auto final_entry = Source();
        final_runtime.counts[0x2001u] = static_cast<std::uint8_t>(count);
        Check(pal4::TimerQueueEntry::Destroy(final_entry, final_runtime) ==
                  0x77u && final_runtime.freed == 0x2000u &&
                  final_entry.name.data == 0u,
              "zero and 0xff counts free name storage");
    }

    Runtime empty_runtime;
    pal4::TimerQueueEntryState empty;
    empty.owns_callback = true;
    Check(pal4::TimerQueueEntry::Destroy(empty, empty_runtime) == 0u &&
              empty_runtime.released == 0u &&
              empty_runtime.freed == 0u,
          "empty entry has no callback or name to release");
}

void CheckDeletingDestructor() {
    Runtime retained;
    retained.entries[0x5000u] = Source();
    retained.counts[0x2001u] = 2u;
    Check(pal4::TimerQueueEntry::DestroyAndMaybeFree(
              0x5000u, 2u, retained) == 0x5000u &&
              retained.calls == (std::vector<std::string>{
                  "read-count", "write-count"}) &&
              retained.entries.at(0x5000u).name.data == 0u,
          "clear low flag destroys without freeing object");

    Runtime deleted;
    deleted.entries[0x5100u] = Source();
    deleted.entries[0x5100u].owns_callback = true;
    deleted.entries[0x5100u].callback = 0x4000u;
    deleted.counts[0x2001u] = 0u;
    Check(pal4::TimerQueueEntry::DestroyAndMaybeFree(
              0x5100u, 3u, deleted) == 0x5100u &&
              deleted.calls == (std::vector<std::string>{
                  "release", "read-count", "free", "free-object:20736"}),
          "set low flag frees object after callback and name");
}

}  // namespace

int main() {
    CheckSharedConstruction();
    CheckCopiedConstruction();
    CheckAliasConstruction();
    CheckDestruction();
    CheckDeletingDestructor();
}
