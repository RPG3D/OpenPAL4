#include "pal4/world_map_fly_callback.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IWorldMapFlyCallbackRuntime {
    static constexpr std::uint32_t kSource = 0x1000u;
    static constexpr std::uint32_t kObject = 0x2000u;
    static constexpr std::uint32_t kSourceData = 0x3001u;
    static constexpr std::uint32_t kAllocatedData = 0x4001u;
    static constexpr std::uint32_t kEmptyData = 0x5001u;

    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> erases;
    std::uint32_t allocations = 0u;
    std::uint32_t copies = 0u;
    std::uint32_t empty_lookups = 0u;
    std::uint32_t flags_set = 0u;
    bool allocate_succeeds = true;
    bool copy_throws = false;

    Runtime() {
        WriteByte(kSource, 0x7au);
        WriteWord(kSource + 4u, kSourceData);
        WriteWord(kSource + 8u, 4u);
        WriteWord(kSource + 0xcu, 4u);
        WriteByte(kSourceData - 1u, 2u);
        WriteByte(kEmptyData - 1u, 0xffu);
        for (std::uint32_t i = 0; i < 4u; ++i) {
            WriteByte(kSourceData + i,
                      static_cast<std::uint8_t>('a' + i));
        }
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        const auto it = memory.find(address);
        return it == memory.end() ? 0u : it->second;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        std::uint32_t result = 0u;
        for (std::uint32_t i = 0; i < 4u; ++i) {
            result |= static_cast<std::uint32_t>(ReadByte(address + i)) <<
                      (i * 8u);
        }
        return result;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (std::uint32_t i = 0; i < 4u; ++i) {
            WriteByte(address + i,
                      static_cast<std::uint8_t>(value >> (i * 8u)));
        }
    }
    std::uint32_t EmptyText() override {
        ++empty_lookups;
        return kEmptyData;
    }
    void SetErrorFlag(const std::uint32_t text,
                      const std::uint8_t value) override {
        Check(text == kObject + 0x10u && value == 1u,
              "shared text sets error flag on embedded buffer");
        ++flags_set;
        WriteByte(text, value);
    }
    void EraseText(const std::uint32_t text,
                   const std::uint32_t start,
                   const std::uint32_t count) override {
        Check(text == kObject + 0x10u, "self-alias erase receiver");
        erases.emplace_back(start, count);
    }
    bool AllocateText(const std::uint32_t text,
                      const std::uint32_t length,
                      const std::uint8_t mode) override {
        Check(text == kObject + 0x10u && mode == 1u,
              "deep text allocation receiver and mode");
        ++allocations;
        if (!allocate_succeeds) return false;
        WriteWord(text + 4u, kAllocatedData);
        WriteWord(text + 0xcu, length);
        WriteByte(kAllocatedData - 1u, 0u);
        return true;
    }
    void Copy(const std::uint32_t destination,
              const std::uint32_t source,
              const std::uint32_t length) override {
        Check(destination == kAllocatedData &&
                  (source == kSourceData || source == kEmptyData),
              "deep copy addresses");
        ++copies;
        if (copy_throws) throw std::runtime_error("copy");
        for (std::uint32_t i = 0; i < length; ++i) {
            WriteByte(destination + i, ReadByte(source + i));
        }
    }
    std::uint32_t Free(const std::uint32_t address) override {
        freed.push_back(address);
        return 0xf00du;
    }
};

void TestSharedCopyAndDestruction() {
    Runtime runtime;
    Check(pal4::WorldMapFlyCallback::Construct(
              Runtime::kObject, 0x6000u, 0x484c10u, 0u,
              Runtime::kSource, runtime) == Runtime::kObject,
          "constructor returns callback address");
    Check(runtime.ReadWord(Runtime::kObject) == 0x843cd4u &&
              runtime.ReadWord(Runtime::kObject + 4u) == 0x6000u &&
              runtime.ReadWord(Runtime::kObject + 8u) == 0x484c10u &&
              runtime.ReadWord(Runtime::kObject + 0xcu) == 0u &&
              runtime.ReadByte(Runtime::kObject + 0x10u) == 1u &&
              runtime.ReadWord(Runtime::kObject + 0x14u) ==
                  Runtime::kSourceData &&
              runtime.ReadWord(Runtime::kObject + 0x18u) == 4u &&
              runtime.ReadWord(Runtime::kObject + 0x1cu) == 4u &&
              runtime.ReadByte(Runtime::kSourceData - 1u) == 2u &&
              runtime.flags_set == 1u && runtime.allocations == 0u &&
              runtime.freed.empty(),
          "share path increments reference then releases by-value source");
    Check(pal4::WorldMapFlyCallback::Destroy(
              Runtime::kObject, runtime) == Runtime::kSourceData - 1u &&
              runtime.ReadByte(Runtime::kSourceData - 1u) == 1u &&
              runtime.ReadWord(Runtime::kObject) == 0x843b40u &&
              runtime.ReadWord(Runtime::kObject + 0x14u) == 0u &&
              runtime.ReadWord(Runtime::kObject + 0x18u) == 0u &&
              runtime.ReadWord(Runtime::kObject + 0x1cu) == 0u,
          "ordinary destructor decrements shared data and resets fields");
}

void TestDeepCopyAndDelete() {
    Runtime runtime;
    runtime.WriteByte(Runtime::kSourceData - 1u, 0xfeu);
    static_cast<void>(pal4::WorldMapFlyCallback::Construct(
        Runtime::kObject, 1u, 2u, 3u, Runtime::kSource, runtime));
    Check(runtime.allocations == 1u && runtime.copies == 1u &&
              runtime.flags_set == 0u &&
              runtime.ReadWord(Runtime::kObject + 0x14u) ==
                  Runtime::kAllocatedData &&
              runtime.ReadWord(Runtime::kObject + 0x18u) == 4u &&
              runtime.ReadByte(Runtime::kAllocatedData + 3u) == 'd' &&
              runtime.ReadByte(Runtime::kAllocatedData + 4u) == 0u &&
              runtime.ReadByte(Runtime::kSourceData - 1u) == 0xfdu &&
              runtime.ReadWord(Runtime::kObject + 0xcu) == 3u,
          "saturated source count causes deep copy with terminator");
    Check(pal4::WorldMapFlyCallback::Delete(
              Runtime::kObject, 3u, runtime) == Runtime::kObject &&
              runtime.freed ==
                  std::vector<std::uint32_t>{
                      Runtime::kAllocatedData - 1u, Runtime::kObject} &&
              runtime.ReadWord(Runtime::kObject) == 0x843b40u,
          "deleting destructor releases text then object for bit-zero flag");
}

void TestSharedLengthDifferentFromCapacity() {
    Runtime runtime;
    runtime.WriteWord(Runtime::kSource + 8u, 2u);
    runtime.WriteWord(Runtime::kSource + 0xcu, 8u);
    static_cast<void>(pal4::WorldMapFlyCallback::Construct(
        Runtime::kObject, 1u, 2u, 3u, Runtime::kSource, runtime));
    Check(runtime.allocations == 0u && runtime.flags_set == 1u &&
              runtime.ReadWord(Runtime::kObject + 0x14u) ==
                  Runtime::kSourceData &&
              runtime.ReadWord(Runtime::kObject + 0x18u) == 2u &&
              runtime.ReadWord(Runtime::kObject + 0x1cu) == 8u,
          "nonzero length shares when count permits regardless of capacity");
}

void TestEmptyAndFailedAllocation() {
    Runtime runtime;
    runtime.WriteWord(Runtime::kSource + 4u, 0u);
    runtime.WriteWord(Runtime::kSource + 8u, 0u);
    runtime.WriteWord(Runtime::kSource + 0xcu, 0u);
    static_cast<void>(pal4::WorldMapFlyCallback::Construct(
        Runtime::kObject, 1u, 2u, 3u, Runtime::kSource, runtime));
    Check(runtime.allocations == 1u && runtime.copies == 1u &&
              runtime.empty_lookups == 1u && runtime.freed.empty() &&
              runtime.ReadWord(Runtime::kObject + 0x18u) == 0u,
          "empty source uses fallback and allocates zero-length text");

    Runtime failed;
    failed.allocate_succeeds = false;
    failed.WriteWord(Runtime::kSource + 8u, 2u);
    failed.WriteByte(Runtime::kSourceData - 1u, 0xfeu);
    static_cast<void>(pal4::WorldMapFlyCallback::Construct(
        Runtime::kObject, 1u, 2u, 3u, Runtime::kSource, failed));
    Check(failed.allocations == 1u && failed.copies == 0u &&
              failed.ReadWord(Runtime::kObject + 0x14u) == 0u &&
              failed.ReadByte(Runtime::kSourceData - 1u) == 0xfdu,
          "failed deep allocation still releases source reference");
    Check(pal4::WorldMapFlyCallback::Delete(
              Runtime::kObject, 2u, failed) == Runtime::kObject &&
              failed.freed.empty(),
          "deleting destructor ignores flags without bit zero");
}

void TestSelfAliasAndException() {
    Runtime alias;
    static_cast<void>(pal4::WorldMapFlyCallback::Construct(
        Runtime::kObject, 1u, 2u, 3u,
        Runtime::kObject + 0x10u, alias));
    Check(alias.erases ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0u, 0xffffffffu}, {0u, 0u}}) &&
              alias.allocations == 0u,
          "self-alias branch uses two erase calls");

    Runtime throwing;
    throwing.WriteByte(Runtime::kSourceData - 1u, 0xfeu);
    throwing.copy_throws = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::WorldMapFlyCallback::Construct(
            Runtime::kObject, 1u, 2u, 3u,
            Runtime::kSource, throwing));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
              throwing.ReadByte(Runtime::kSourceData - 1u) == 0xfdu &&
              throwing.freed ==
                  std::vector<std::uint32_t>{
                      Runtime::kAllocatedData - 1u} &&
              throwing.ReadWord(Runtime::kObject) == 0x843b40u,
          "copy failure releases by-value source and constructed text");
}

void TestSentinelReferenceDestruction() {
    for (const std::uint8_t reference : {0u, 0xffu}) {
        Runtime runtime;
        runtime.WriteWord(Runtime::kObject + 0x14u,
                          Runtime::kSourceData);
        runtime.WriteByte(Runtime::kSourceData - 1u, reference);
        Check(pal4::WorldMapFlyCallback::Destroy(
                  Runtime::kObject, runtime) == 0xf00du &&
                  runtime.freed ==
                      std::vector<std::uint32_t>{
                          Runtime::kSourceData - 1u},
              "zero and ff reference counts free backing allocation");
    }
}

}  // namespace

int main() {
    TestSharedCopyAndDestruction();
    TestDeepCopyAndDelete();
    TestSharedLengthDifferentFromCapacity();
    TestEmptyAndFailedAllocation();
    TestSelfAliasAndException();
    TestSentinelReferenceDestruction();
    return EXIT_SUCCESS;
}
