#include "pal4/string_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

class Runtime final : public pal4::IStringAssignRuntime {
public:
    void ThrowRange() override { throw std::out_of_range("range"); }
    void ThrowLength() override { throw std::length_error("length"); }
    void DetachAndReset(pal4::StringBufferState&) override {
        Check(false, "erase does not call detach helper");
    }
    bool AllocateText(pal4::StringBufferState& state,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(mode == 1u, "shared erase requests allocation mode one");
        events.emplace_back("allocate:" + std::to_string(length));
        if (!allocation_succeeds) {
            return false;
        }
        state.data = 0x5001u;
        state.capacity = length + 8u;
        bytes[0x5000u] = new_reference;
        return true;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        events.emplace_back("size:" + std::to_string(length));
        state.length = length;
        bytes[state.data + length] = 0u;
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      std::uint8_t flag) override {
        events.emplace_back("error:" + std::to_string(flag));
        state = {};
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        events.emplace_back("reallocate:" + std::to_string(length));
        std::vector<std::uint8_t> content;
        for (std::uint32_t index = 0u; index < length; ++index) {
            content.push_back(bytes.at(state.data + index));
        }
        state.data = 0x6001u;
        state.capacity = length + 8u;
        for (std::uint32_t index = 0u; index < length; ++index) {
            bytes[state.data + index] = content[index];
        }
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        events.emplace_back("copy:" + std::to_string(length));
        std::vector<std::uint8_t> content;
        for (std::uint32_t index = 0u; index < length; ++index) {
            content.push_back(bytes.at(source + index));
        }
        for (std::uint32_t index = 0u; index < length; ++index) {
            bytes[destination + index] = content[index];
        }
    }
    void Free(std::uint32_t) override {
        Check(false, "erase does not directly free old storage");
    }
    void Put(std::uint32_t address, const std::string& value) {
        for (std::size_t index = 0u; index < value.size(); ++index) {
            bytes[address + static_cast<std::uint32_t>(index)] =
                static_cast<std::uint8_t>(value[index]);
        }
        bytes[address + static_cast<std::uint32_t>(value.size())] = 0u;
    }

    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint8_t new_reference = 0u;
    bool allocation_succeeds = true;
};

void CheckRangeAndUniqueErase() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 0u;
    runtime.Put(0x2001u, "abcdef");
    pal4::StringBuffer buffer({0x2001u, 6u, 12u});
    bool threw = false;
    try {
        (void)buffer.Erase(7u, 1u, runtime);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw && buffer.State().length == 6u &&
              runtime.events.empty(),
          "erase rejects start beyond length before touching memory");

    Check(&buffer.Erase(2u, 2u, runtime) == &buffer &&
              buffer.State().data == 0x2001u &&
              buffer.State().length == 4u &&
              buffer.State().capacity == 12u &&
              runtime.bytes[0x2003u] == 'e' &&
              runtime.bytes[0x2004u] == 'f' &&
              runtime.bytes[0x2005u] == 0u,
          "unique erase shifts suffix and reuses storage");
}

void CheckSharedZeroCountStillDetaches() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 2u;
    runtime.Put(0x2001u, "abc");
    pal4::StringBuffer buffer({0x2001u, 3u, 8u});
    Check(&buffer.Erase(1u, 0u, runtime) == &buffer &&
              runtime.bytes[0x2000u] == 1u &&
              buffer.State().data == 0x5001u &&
              buffer.State().length == 3u &&
              runtime.bytes[0x5001u] == 'a' &&
              runtime.bytes[0x5003u] == 'c' &&
              runtime.events.back() == "size:3",
          "zero-count erase still detaches an ordinary shared header");
}

void CheckEraseAllBranches() {
    Runtime unique;
    unique.bytes[0x2000u] = 0xffu;
    unique.Put(0x2001u, "abc");
    pal4::StringBuffer owned({0x2001u, 3u, 8u});
    Check(&owned.Erase(0u, 99u, unique) == &owned &&
              owned.State().data == 0x2001u &&
              owned.State().length == 0u &&
              unique.bytes[0x2001u] == 0u,
          "unique full erase retains storage and writes first-byte terminator");

    Runtime shared;
    shared.new_reference = 2u;
    shared.bytes[0x2000u] = 2u;
    shared.Put(0x2001u, "abc");
    pal4::StringBuffer released({0x2001u, 3u, 8u});
    Check(&released.Erase(0u, 99u, shared) == &released &&
              released.State().data == 0u &&
              released.State().length == 0u &&
              released.State().capacity == 0u &&
              shared.bytes[0x5000u] == 1u &&
              shared.events.back() == "error:0",
          "ordinary new header decrements then clears on full erase");
}

void CheckSharedPartialReallocation() {
    Runtime runtime;
    runtime.new_reference = 2u;
    runtime.bytes[0x2000u] = 3u;
    runtime.Put(0x2001u, "abc");
    pal4::StringBuffer buffer({0x2001u, 3u, 8u});
    Check(&buffer.Erase(1u, 1u, runtime) == &buffer &&
              runtime.bytes[0x2000u] == 2u &&
              buffer.State().data == 0x6001u &&
              buffer.State().length == 2u &&
              runtime.bytes[0x6001u] == 'a' &&
              runtime.bytes[0x6002u] == 'c' &&
              runtime.bytes[0x6003u] == 0u &&
              runtime.events[runtime.events.size() - 2u] ==
                  "reallocate:2",
          "ordinary reference after detach forces reallocation for nonempty result");
}

void CheckHostStringEraseBoundary() {
    struct Case {
        std::uint32_t start;
        std::uint32_t count;
    };
    for (const Case test : {Case{0u, 0u}, Case{2u, 2u},
                            Case{1u, 0xffffffffu}, Case{6u, 4u}}) {
        Runtime runtime;
        runtime.bytes[0x2000u] = 0xffu;
        runtime.Put(0x2001u, "abcdef");
        pal4::StringBuffer buffer({0x2001u, 6u, 12u});
        std::string host = "abcdef";
        host.erase(test.start, test.count);
        (void)buffer.Erase(test.start, test.count, runtime);
        Check(buffer.State().length == host.size(),
              "normal erase length agrees with host std::string");
        for (std::size_t index = 0u; index <= host.size(); ++index) {
            const auto expected = index == host.size()
                                      ? 0u
                                      : static_cast<std::uint8_t>(host[index]);
            Check(runtime.bytes.at(buffer.State().data +
                                   static_cast<std::uint32_t>(index)) ==
                      expected,
                  "normal erase bytes agree with host std::string");
        }
    }

    Runtime runtime;
    runtime.bytes[0x2000u] = 0xffu;
    runtime.Put(0x2001u, "abc");
    pal4::StringBuffer buffer({0x2001u, 3u, 8u});
    std::string host = "abc";
    bool host_threw = false;
    bool model_threw = false;
    try {
        host.erase(4u, 1u);
    } catch (const std::out_of_range&) {
        host_threw = true;
    }
    try {
        (void)buffer.Erase(4u, 1u, runtime);
    } catch (const std::out_of_range&) {
        model_threw = true;
    }
    Check(host_threw && model_threw && buffer.State().length == 3u,
          "out-of-range erase agrees with host std::string");
}

}  // namespace

int main() {
    CheckRangeAndUniqueErase();
    CheckSharedZeroCountStillDetaches();
    CheckEraseAllBranches();
    CheckSharedPartialReallocation();
    CheckHostStringEraseBoundary();
    std::cout << "5 string buffer erase test cases passed\n";
}
