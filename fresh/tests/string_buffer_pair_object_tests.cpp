#include "pal4/string_buffer_pair_object.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::IStringBufferRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t size) override {
        events.push_back("alloc:" + std::to_string(size));
        ++allocation_calls;
        if (allocation_calls == throw_on_allocation)
            throw std::runtime_error("allocation failed");
        if (allocation_calls == zero_on_allocation) return 0;
        const auto result = next_allocation;
        next_allocation += size + 0x10;
        return result;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        if (address == throw_on_free) throw std::runtime_error("free failed");
        return 17;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return memory[address];
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        read_dword_addresses.push_back(address);
        return static_cast<std::uint32_t>(memory[address]) |
               (static_cast<std::uint32_t>(memory[address + 1]) << 8U) |
               (static_cast<std::uint32_t>(memory[address + 2]) << 16U) |
               (static_cast<std::uint32_t>(memory[address + 3]) << 24U);
    }
    void Copy(const std::uint32_t destination, const std::uint32_t source,
              const std::uint32_t size) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::to_string(source) + ":" + std::to_string(size));
        std::vector<std::uint8_t> bytes;
        for (std::uint32_t i = 0; i < size; ++i) bytes.push_back(memory[source + i]);
        for (std::uint32_t i = 0; i < size; ++i) memory[destination + i] = bytes[i];
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        events.push_back("dword:" + std::to_string(address) + ":" +
                         std::to_string(value));
        for (std::uint32_t i = 0; i < 4; ++i) {
            memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8U));
        }
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        events.push_back("byte:" + std::to_string(address) + ":" +
                         std::to_string(value));
        memory[address] = value;
    }
    std::uint32_t FindSubstring(std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t FindCharacter(std::uint32_t, std::int32_t) override { return 0; }
    std::uint32_t FindLastCharacter(std::uint32_t, std::int32_t) override { return 0; }
    std::int32_t CompareCaseInsensitive(std::uint32_t, std::uint32_t) override { return 0; }
    void ConvertUppercase(std::uint32_t) override {}
    void ConvertLowercase(std::uint32_t) override {}

    void PutDword(const std::uint32_t address, const std::uint32_t value) {
        for (std::uint32_t i = 0; i < 4; ++i) memory[address + i] = static_cast<std::uint8_t>(value >> (i * 8U));
    }
    void Put(const std::uint32_t address, const std::string& value) {
        for (std::size_t i = 0; i < value.size(); ++i) memory[address + static_cast<std::uint32_t>(i)] = static_cast<std::uint8_t>(value[i]);
        memory[address + static_cast<std::uint32_t>(value.size())] = 0;
    }
    [[nodiscard]] std::string ReadCString(const std::uint32_t address) const {
        std::string value;
        for (std::uint32_t i = 0; i < 256; ++i) {
            const auto found = memory.find(address + i);
            if (found == memory.end() || found->second == 0) break;
            value.push_back(static_cast<char>(found->second));
        }
        return value;
    }

    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> events;
    std::vector<std::uint32_t> read_dword_addresses;
    std::uint32_t next_allocation = 0x5000;
    std::uint32_t allocation_calls = 0;
    std::uint32_t throw_on_allocation = 0;
    std::uint32_t zero_on_allocation = 0;
    std::uint32_t throw_on_free = 0;
};

void PairInitializationUsesTwoSourceRecordsAndExistingCapacity() {
    FakeRuntime runtime;
    runtime.PutDword(0x3000, 0x1000);
    runtime.PutDword(0x3004, 3);
    runtime.PutDword(0x300c, 0x1100);
    runtime.PutDword(0x3010, 2);
    runtime.Put(0x1000, "abc");
    runtime.Put(0x1100, "xy");
    runtime.PutDword(0x4000, 0x2000);
    runtime.PutDword(0x4008, 16);
    runtime.PutDword(0x400c, 0x2100);
    runtime.PutDword(0x4014, 16);

    pal4::StringBufferPairObject::InitializeAt(0x4000, 0x3000, runtime);

    Check(runtime.read_dword_addresses == std::vector<std::uint32_t>{
              0x3004u, 0x3000u, 0x4000u, 0x4008u,
              0x3010u, 0x300cu, 0x400cu, 0x4014u},
          "pair initialization reads each source length before its pointer");
    Check(runtime.ReadDword(0x4004) == 3 && runtime.ReadDword(0x4010) == 2,
          "both embedded string lengths are updated");
    Check(runtime.ReadCString(0x2000) == "abc" &&
              runtime.ReadCString(0x2100) == "xy",
          "both embedded strings are copied to their existing buffers");
    Check(runtime.events == std::vector<std::string>{
              "dword:16388:3", "copy:8192:4096:3", "byte:8195:0",
              "dword:16400:2", "copy:8448:4352:2", "byte:8450:0"},
          "pair initialization processes first buffer then second");
}

void PairInitializationReadsAliasedSecondSourceAfterFirstCall() {
    FakeRuntime runtime;
    runtime.Put(0x1000u, "abc");
    runtime.Put(0x2000u, "z");
    runtime.Put(0x2100u, "");
    runtime.PutDword(0x3ff4u, 0x1000u);
    runtime.PutDword(0x3ff8u, 3u);
    runtime.PutDword(0x4000u, 0x2000u);
    runtime.PutDword(0x4004u, 1u);
    runtime.PutDword(0x4008u, 16u);
    runtime.PutDword(0x400cu, 0x2100u);
    runtime.PutDword(0x4014u, 16u);

    pal4::StringBufferPairObject::InitializeAt(0x4000u, 0x3ff4u, runtime);

    Check(runtime.ReadCString(0x2000u) == "abc" &&
              runtime.ReadCString(0x2100u) == "abc" &&
              runtime.ReadDword(0x4010u) == 3u,
          "second source observes first destination after the first call");
    Check(runtime.read_dword_addresses == std::vector<std::uint32_t>{
              0x3ff8u, 0x3ff4u, 0x4000u, 0x4008u,
              0x4004u, 0x4000u, 0x400cu, 0x4014u, 0x4010u},
          "aliased source fields are read only when their call is reached");
}

void BufferInitializationGrowsWithoutCopyingOldContent() {
    FakeRuntime runtime;
    runtime.PutDword(0x4000, 0x2000);
    runtime.PutDword(0x4008, 2);
    runtime.Put(0x1000, "new");

    static_cast<void>(pal4::StringBuffer::InitializeAt(0x4000, 0x1000, 3,
                                                       runtime));

    Check(runtime.ReadDword(0x4000) == 0x5000 &&
              runtime.ReadDword(0x4004) == 3 &&
              runtime.ReadDword(0x4008) == 11,
          "growth stores new pointer, length, and length plus eight capacity");
    Check(runtime.events == std::vector<std::string>{
              "alloc:11", "free:8192", "dword:16392:11",
              "dword:16384:20480", "dword:16388:3",
              "copy:20480:4096:3", "byte:20483:0"},
          "growth skips old copy, frees old data, then copies new source");
}

void AddressBackedInitializationPreservesCapacitySemantics() {
    FakeRuntime empty;
    empty.Put(0x1000, "abc");
    Check(pal4::StringBuffer::InitializeAt(0x4000, 0x1000, 3, empty) ==
              0x4000 &&
              empty.ReadDword(0x4000) == 0x5000 &&
              empty.ReadDword(0x4004) == 3 &&
              empty.ReadDword(0x4008) == 11 &&
              empty.ReadCString(0x5000) == "abc" &&
              empty.events == std::vector<std::string>{
                  "dword:16388:3", "dword:16392:11", "alloc:11",
                  "dword:16384:20480", "dword:16388:3",
                  "copy:20480:4096:3", "byte:20483:0"},
          "null record stores length and capacity before allocating once");

    FakeRuntime signed_capacity;
    signed_capacity.PutDword(0x4000, 0x2000);
    signed_capacity.PutDword(0x4008, 0x80000000u);
    signed_capacity.Put(0x1000, "abc");
    static_cast<void>(pal4::StringBuffer::InitializeAt(
        0x4000, 0x1000, 3, signed_capacity));
    Check(signed_capacity.ReadDword(0x4000) == 0x5000 &&
              signed_capacity.ReadDword(0x4008) == 11 &&
              signed_capacity.events[0] == "alloc:11" &&
              signed_capacity.events[1] == "free:8192",
          "signed capacity comparison grows a record with sign bit set");
}

void PairCleanupRunsSecondThenFirstAndSkipsNullPointers() {
    FakeRuntime runtime;
    runtime.PutDword(0x4000, 0x2000);
    runtime.PutDword(0x400c, 0x2100);
    pal4::StringBufferPairObject::CleanupAt(0x4000, runtime);
    Check(runtime.events == std::vector<std::string>{
              "free:8448", "dword:16396:0", "free:8192",
              "dword:16384:0"},
          "pair cleanup frees the second buffer first and clears each pointer");
    runtime.events.clear();
    pal4::StringBufferPairObject::CleanupAt(0x4000, runtime);
    Check(runtime.events.empty(), "repeated cleanup of null pointers has no writes");
}

void ObjectDestructionIsNullSafeAndFreesPairAfterFields() {
    FakeRuntime runtime;
    pal4::StringBufferPairObject::DestroyObject(0, runtime);
    Check(runtime.events.empty(), "null object destructor does nothing");
    runtime.PutDword(0x4000, 0x2000);
    runtime.PutDword(0x400c, 0x2100);
    pal4::StringBufferPairObject::DestroyObject(0x4000, runtime);
    Check(runtime.events == std::vector<std::string>{
              "free:8448", "dword:16396:0", "free:8192",
              "dword:16384:0", "free:16384"} &&
              runtime.ReadDword(0x4000) == 0 && runtime.ReadDword(0x400c) == 0,
          "object is freed after both embedded buffers are cleaned");
}

void CleanupExceptionStillReleasesFirstString() {
    FakeRuntime runtime;
    runtime.PutDword(0x4000, 0x2000);
    runtime.PutDword(0x400c, 0x2100);
    runtime.throw_on_free = 0x2100;
    bool caught = false;
    try {
        pal4::StringBufferPairObject::CleanupAt(0x4000, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.events == std::vector<std::string>{
              "free:8448", "free:8192", "dword:16384:0"} &&
              runtime.ReadDword(0x4000) == 0,
          "second cleanup failure still invokes first cleanup");
}

void FactoryInitializesTwoEmbeddedStrings() {
    FakeRuntime runtime;
    const auto object =
        pal4::StringBufferPairObject::AllocateAndInitializeObjects(runtime);
    Check(object == 0x5000 && runtime.ReadDword(object + 8) == 0x20 &&
              runtime.ReadDword(object) == 0x5028 &&
              runtime.ReadDword(object + 4) == 0 &&
              runtime.ReadDword(object + 0x0c) == 0x5058 &&
              runtime.ReadDword(object + 0x10) == 0 &&
              runtime.ReadDword(object + 0x14) == 0x20,
          "factory initializes both 12-byte string records");
    Check(runtime.events == std::vector<std::string>{
              "alloc:24", "alloc:32", "dword:20480:20520",
              "dword:20484:0", "dword:20488:32", "byte:20520:0",
              "alloc:32", "dword:20492:20568", "dword:20496:0",
              "dword:20500:32", "byte:20568:0"},
          "second constructor targets object plus twelve without overwriting first");
}

void FactoryFailurePathsMatchConstructionState() {
    FakeRuntime zero;
    zero.zero_on_allocation = 1;
    Check(pal4::StringBufferPairObject::AllocateAndInitializeObjects(zero) == 0 &&
              zero.events == std::vector<std::string>{"alloc:24"},
          "null object allocation skips both embedded constructors");

    for (const auto failed_allocation : {2u, 3u}) {
        FakeRuntime runtime;
        runtime.throw_on_allocation = failed_allocation;
        bool caught = false;
        try {
            static_cast<void>(
                pal4::StringBufferPairObject::AllocateAndInitializeObjects(runtime));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        const auto expected = failed_allocation == 2u
            ? std::vector<std::string>{
                  "alloc:24", "alloc:32", "free:20480"}
            : std::vector<std::string>{
                  "alloc:24", "alloc:32", "dword:20480:20520",
                  "dword:20484:0", "dword:20488:32", "byte:20520:0",
                  "alloc:32", "free:20520", "dword:20480:0",
                  "free:20480"};
        Check(caught && runtime.events == expected,
              "exception cleanup follows the number of constructed members");
    }
}

}  // namespace

int main() {
    PairInitializationUsesTwoSourceRecordsAndExistingCapacity();
    PairInitializationReadsAliasedSecondSourceAfterFirstCall();
    BufferInitializationGrowsWithoutCopyingOldContent();
    AddressBackedInitializationPreservesCapacitySemantics();
    PairCleanupRunsSecondThenFirstAndSkipsNullPointers();
    ObjectDestructionIsNullSafeAndFreesPairAfterFields();
    CleanupExceptionStillReleasesFirstString();
    FactoryInitializesTwoEmbeddedStrings();
    FactoryFailurePathsMatchConstructionState();
    std::cout << "9 string-buffer-pair test cases passed\n";
}
