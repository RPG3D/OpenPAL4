#include "pal4/combat_event_record.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool allocation_succeeds = true;
    bool detach_restores_size = false;
    std::uint32_t detach_count = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t EmptyTextAddress() override {
        calls.push_back("empty");
        return 9000u;
    }
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override {
        calls.push_back("flag " + std::to_string(record) + " " +
                        std::to_string(flag));
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      bool copy_mode) override {
        calls.push_back("allocate " + std::to_string(record) + " " +
                        std::to_string(length) + " " +
                        std::to_string(copy_mode));
        if (allocation_succeeds) words[record + 4u] = 8000u;
        return allocation_succeeds;
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t length) override {
        calls.push_back("copy " + std::to_string(destination) + " " +
                        std::to_string(source) + " " +
                        std::to_string(length));
        for (std::uint32_t i = 0; i < length; ++i)
            bytes[destination + i] = bytes[source + i];
    }
    void DetachAndReset(std::uint32_t record) override {
        ++detach_count;
        calls.push_back("detach " + std::to_string(record));
        if (detach_restores_size && detach_count == 1u) {
            words[record + 4u] = 7000u;
            words[record + 8u] = 2u;
        }
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        calls.push_back("size " + std::to_string(record) + " " +
                        std::to_string(length));
        words[record + 8u] = length;
    }
};

void Prepare(Runtime& runtime, std::uint32_t length,
             std::uint32_t text_pointer, std::uint8_t refcount) {
    runtime.bytes[5000u] = 0x81u;
    runtime.words[5004u] = text_pointer;
    runtime.words[5008u] = length;
    runtime.words[5012u] = 9u;
    runtime.words[5016u] = 45u;
    runtime.words[6000u] = 67u;
    runtime.bytes[(text_pointer ? text_pointer : 9000u) - 1u] = refcount;
    runtime.bytes[7000u] = 'a';
    runtime.bytes[7001u] = 'b';
    runtime.bytes[7002u] = 'c';
}

void ShareTextAndSeparateMetadata() {
    Runtime runtime;
    Prepare(runtime, 3u, 7000u, 1u);
    Check(pal4::CombatEventRecord::CopyData(
              4000u, 5000u, 6000u, runtime) == 4000u &&
              runtime.bytes[4000u] == 0x81u &&
              runtime.words[4004u] == 7000u &&
              runtime.words[4008u] == 3u &&
              runtime.words[4012u] == 9u &&
              runtime.words[4016u] == 67u &&
              runtime.bytes[6999u] == 2u,
          "copy shares text below refcount threshold and uses separate metadata");
    Check(runtime.calls == std::vector<std::string>{
              "word 4004 0", "byte 4000 129", "word 4008 0",
              "word 4012 0", "flag 4000 1", "word 4004 7000",
              "word 4008 3", "word 4012 9", "byte 6999 2",
              "word 4016 67"},
          "shared copy field and reference count order");
}

void DeepCopyAtSaturatedCount() {
    Runtime runtime;
    Prepare(runtime, 3u, 7000u, 0xfeu);
    pal4::CombatEventRecord::CopyExtended(4000u, 5000u, runtime);
    Check(runtime.words[4004u] == 8000u && runtime.words[4008u] == 3u &&
              runtime.words[4016u] == 45u &&
              runtime.bytes[8000u] == 'a' &&
              runtime.bytes[8001u] == 'b' &&
              runtime.bytes[8002u] == 'c' &&
              runtime.bytes[8003u] == 0u &&
              runtime.bytes[6999u] == 0xfeu,
          "saturated refcount makes a terminated deep copy");
    Check(runtime.calls == std::vector<std::string>{
              "word 4004 0", "byte 4000 129", "word 4008 0",
              "word 4012 0", "allocate 4000 3 1",
              "copy 8000 7000 3", "word 4008 3", "byte 8003 0",
              "word 4016 45"},
          "deep copy allocation and byte order");
}

void EmptyAndFailedAllocation() {
    Runtime empty;
    Prepare(empty, 0u, 0u, 0u);
    pal4::CombatEventRecord::CopyExtended(4000u, 5000u, empty);
    Check(empty.words[4004u] == 8000u && empty.words[4008u] == 0u &&
              empty.bytes[8000u] == 0u && empty.words[4016u] == 45u,
          "empty source still allocates and terminates text");
    Check(empty.calls[4] == "allocate 4000 0 1" &&
              empty.calls[5] == "empty" &&
              empty.calls[6] == "copy 8000 9000 0",
          "empty fallback is read after successful allocation");

    Runtime failed;
    Prepare(failed, 3u, 7000u, 0xfeu);
    failed.allocation_succeeds = false;
    pal4::CombatEventRecord::CopyExtended(4000u, 5000u, failed);
    Check(failed.words[4004u] == 0u && failed.words[4008u] == 0u &&
              failed.words[4016u] == 45u &&
              failed.calls.back() == "word 4016 45",
          "failed allocation still copies metadata");
}

void SelfAndNullDestination() {
    Runtime runtime;
    Prepare(runtime, 3u, 7000u, 1u);
    Check(pal4::CombatEventRecord::CopyData(
              5000u, 5000u, 6000u, runtime) == 5000u &&
              runtime.detach_count == 2u &&
              runtime.words[5004u] == 0u &&
              runtime.words[5008u] == 0u &&
              runtime.words[5016u] == 67u,
          "self-copy observes zeroed fields and detaches twice");
    const auto before = runtime.calls.size();
    pal4::CombatEventRecord::CopyExtended(0u, 5000u, runtime);
    Check(runtime.calls.size() == before,
          "extended copy ignores a null destination");

    Runtime changed;
    Prepare(changed, 0u, 7000u, 1u);
    changed.detach_restores_size = true;
    pal4::CombatEventRecord::CopyExtended(5000u, 5000u, changed);
    Check(changed.detach_count == 2u &&
              changed.words[5008u] == 0u &&
              changed.calls[changed.calls.size() - 4u] ==
                  "allocate 5000 0 0" &&
              changed.calls[changed.calls.size() - 3u] ==
                  "size 5000 0",
          "self-copy handles size restored by first detach");
}

}  // namespace

int main() {
    try {
        ShareTextAndSeparateMetadata();
        DeepCopyAtSaturatedCount();
        EmptyAndFailedAllocation();
        SelfAndNullDestination();
        std::cout << "combat event record tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
