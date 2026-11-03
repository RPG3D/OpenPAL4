#include "pal4/timed_event_queue.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITimedQueueRecordCopyRuntime {
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool allocation_succeeds = true;
    std::uint32_t allocated_length = 0xffffffffu;

    std::uint32_t EmptyString() override {
        calls.emplace_back("empty");
        return 0x5000u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read");
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write");
        bytes[address] = value;
    }
    void SetNameErrorFlag(pal4::TimedQueueRecord& record,
                          std::uint8_t flag) override {
        calls.emplace_back("flag");
        record.name_flag = (record.name_flag & 0xffffff00u) | flag;
    }
    bool AllocateName(pal4::TimedQueueRecord& record,
                      std::uint32_t length, std::uint8_t mode) override {
        calls.emplace_back("allocate");
        Check(mode == 1u, "allocation mode");
        allocated_length = length;
        if (allocation_succeeds) record.name_data = 0x6000u;
        return allocation_succeeds;
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t length) override {
        calls.emplace_back("copy");
        Check(destination == 0x6000u, "copy destination");
        for (std::uint32_t i = 0; i < length; ++i)
            bytes[destination + i] = bytes.at(source + i);
    }
    void SetNameSize(pal4::TimedQueueRecord& record,
                     std::uint32_t length) override {
        calls.emplace_back("size");
        record.name_length = length;
    }
    void EraseName(pal4::TimedQueueRecord&,
                   std::uint32_t start, std::uint32_t count) override {
        calls.push_back("erase:" + std::to_string(start) + ":" +
                        std::to_string(count));
    }
};

pal4::TimedQueueRecord Source() {
    return {0x12345642u, 0x4000u, 3u, 16u,
            27u, 0xaabbcc01u, 0x7000u};
}

void CheckShared() {
    Runtime runtime;
    runtime.bytes[0x3fffu] = 0xfdu;
    auto source = Source();
    pal4::TimedQueueRecord destination{};
    destination.name_flag = 0xabcdef00u;
    destination.owns_callback = 0x11223300u;
    Check(&pal4::TimedQueueRecordCopier::CopyAssign(
              destination, source, runtime) == &destination,
          "copy returns destination");
    Check(destination.name_data == 0x4000u &&
          destination.name_length == 3u &&
          destination.name_capacity == 16u &&
          destination.name_flag == 0xabcdef01u,
          "shared string fields and flag");
    Check(runtime.bytes.at(0x3fffu) == 0xfeu,
          "reference count increment");
    Check(destination.delay == 27u &&
          destination.owns_callback == 0x11223301u &&
          destination.callback == 0x7000u &&
          source.owns_callback == 0xaabbcc00u,
          "tail fields and owner-byte transfer");
    Check(runtime.calls == std::vector<std::string>{"read", "flag", "read",
                                                     "write"},
          "shared branch service order");
}

void CheckAllocated(std::uint8_t reference, std::uint32_t length,
                    bool allocate) {
    Runtime runtime;
    runtime.allocation_succeeds = allocate;
    runtime.bytes[0x3fffu] = reference;
    runtime.bytes[0x4000u] = 'a';
    runtime.bytes[0x4001u] = 'b';
    runtime.bytes[0x4002u] = 'c';
    auto source = Source();
    source.name_length = length;
    pal4::TimedQueueRecord destination{};
    (void)pal4::TimedQueueRecordCopier::CopyAssign(
        destination, source, runtime);
    Check(runtime.allocated_length == length, "allocation length");
    Check(destination.name_data == (allocate ? 0x6000u : 0u),
          "allocation result");
    Check(destination.name_length == (allocate ? length : 0u),
          "copy size after allocation");
    Check(runtime.bytes.at(0x3fffu) == reference,
          "allocated branch does not increment source reference");
    Check(destination.callback == 0x7000u &&
          source.owns_callback == 0xaabbcc00u,
          "tail transfers despite allocation failure");
    if (allocate && length != 0u)
        Check(runtime.bytes.at(0x6002u) == 'c', "allocated bytes copied");
    if (length == 0u)
        Check(runtime.calls.front() == "allocate",
              "zero length skips source header read");
}

void CheckEmptyFallback() {
    Runtime runtime;
    runtime.bytes[0x4fffu] = 0u;
    runtime.bytes[0x5000u] = 'x';
    auto source = Source();
    source.name_data = 0u;
    source.name_length = 1u;
    pal4::TimedQueueRecord destination{};
    (void)pal4::TimedQueueRecordCopier::CopyAssign(
        destination, source, runtime);
    Check(destination.name_data == 0x5000u &&
          runtime.bytes[0x4fffu] == 1u,
          "null data shares fallback empty buffer");
    Check(runtime.calls == std::vector<std::string>{"empty", "read",
                                                     "flag", "empty",
                                                     "read", "write"},
          "fallback pointer is resolved for each source access");
}

void CheckSelf() {
    Runtime runtime;
    auto record = Source();
    (void)pal4::TimedQueueRecordCopier::CopyAssign(record, record, runtime);
    Check(record.name_data == 0u && record.name_length == 0u &&
          record.name_capacity == 0u && record.owns_callback == 0xaabbcc00u,
          "self assignment follows destructive native write order");
    Check(runtime.calls == std::vector<std::string>{
              "erase:0:4294967295", "erase:0:0"},
          "self assignment erase order");
}

}  // namespace

int main() {
    try {
        CheckShared();
        CheckAllocated(0xfeu, 3u, true);
        CheckAllocated(0xffu, 3u, false);
        CheckAllocated(0u, 0u, true);
        CheckEmptyFallback();
        CheckSelf();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
