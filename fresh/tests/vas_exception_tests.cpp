#include "pal4/vas_exception.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IVasExceptionRuntime {
    std::vector<std::string> calls;
    std::uint8_t reference = 0u;
    bool allocation_succeeds = true;
    bool throw_on_copy = false;
    std::uint32_t length_after_detach = 0u;
    std::uint32_t allocated_buffer = 0x9000u;
    unsigned detach_count = 0u;
    bool require_cleared_on_destroy = false;

    void CopyBase(pal4::VasExceptionState& destination,
                  const pal4::VasExceptionState& source) override {
        calls.emplace_back("base");
        for (unsigned i = 0u; i < 3u; ++i) {
            destination.words[i] = source.words[i];
        }
    }
    std::uint32_t EmptyTextAddress() override {
        calls.emplace_back("empty");
        return 0x7000u;
    }
    std::uint8_t ReferenceByte(std::uint32_t buffer) override {
        Check(buffer == 0x4000u || buffer == 0x7000u,
              "reference lookup buffer");
        calls.emplace_back("reference");
        return reference;
    }
    void SetSharedTextFlag(pal4::VasExceptionState&) override {
        calls.emplace_back("flag");
    }
    void IncrementReferenceByte(std::uint32_t buffer) override {
        Check(buffer == 0x4000u || buffer == 0x7000u,
              "reference increment buffer");
        calls.emplace_back("increment");
    }
    void DecrementReferenceByte(std::uint32_t buffer) override {
        Check(buffer == 0x4000u,
              "decrement uses text buffer address");
        calls.emplace_back("decrement");
    }
    void FreeText(std::uint32_t header) override {
        Check(header == 0x3fffu,
              "free uses reference-byte header address");
        calls.emplace_back("free-text");
    }
    bool AllocateText(pal4::VasExceptionState& destination,
                      std::uint32_t size, std::uint32_t mode) override {
        Check((size == 3u && mode == 1u) ||
                  (size == 0u && (mode == 0u || mode == 1u)),
              "allocation size and mode");
        calls.emplace_back("allocate");
        if (allocation_succeeds) destination.words[4u] = allocated_buffer;
        return allocation_succeeds;
    }
    void CopyText(std::uint32_t destination, std::uint32_t source,
                  std::uint32_t size) override {
        if (throw_on_copy) throw std::runtime_error("copy");
        Check(destination == allocated_buffer || destination == 0x5000u,
              "copy destination");
        Check(source == 0x4000u || source == 0x7000u ||
                  source == 0x5004u || source == 0x4fffu,
              "copy source");
        Check(size == 3u || size == 0u, "copy size");
        calls.emplace_back("copy");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == allocated_buffer + 3u ||
                  address == allocated_buffer,
              "terminator address");
        Check(value == 0u, "terminator value");
        calls.emplace_back("terminator");
    }
    void DetachAndReset(pal4::VasExceptionState& destination) override {
        calls.emplace_back("detach");
        ++detach_count;
        if (detach_count == 1u) {
            destination.words[4u] = 0x5000u;
            destination.words[5u] = length_after_detach;
        }
    }
    void SetTextSize(pal4::VasExceptionState& destination,
                     std::uint32_t size) override {
        Check(size == 0u, "self-copy set size");
        calls.emplace_back("set-size");
        destination.words[5u] = size;
    }
    void DestroyBase(pal4::VasExceptionState& state) override {
        if (require_cleared_on_destroy) {
            Check(state.words[4u] == 0u && state.words[5u] == 0u &&
                      state.words[6u] == 0u,
                  "text fields clear before base destructor");
        }
        calls.emplace_back("destroy-base");
    }
};

void CheckCalls(const Runtime& runtime,
                const std::vector<std::string>& expected) {
    Check(runtime.calls == expected, "native call order");
}

pal4::VasExceptionState Source() {
    pal4::VasExceptionState source{};
    source.words = {0x1111u, 0x2222u, 0x3333u, 0x12345678u,
                    0x4000u, 3u, 8u};
    return source;
}

void TestSharedCopy() {
    for (const auto reference : {std::uint8_t{0u}, std::uint8_t{0xfdu}}) {
        Runtime runtime;
        runtime.reference = reference;
        auto source = Source();
        pal4::VasExceptionState destination{};
        destination.words[3u] = 0xaabbcc00u;
        Check(&pal4::VasException::CopyConstruct(destination, source, runtime)
                  == &destination, "copy returns destination");
        Check(destination.words == std::array<std::uint32_t, 7u>{
                  pal4::VasException::kVtable, 0x2222u, 0x3333u,
                  0xaabbcc78u, 0x4000u, 3u, 8u}, "shared object layout");
        CheckCalls(runtime, {"base", "reference", "flag", "increment"});
    }
}

void TestDeepCopy() {
    for (const auto reference : {std::uint8_t{0xfeu}, std::uint8_t{0xffu}}) {
        Runtime runtime;
        runtime.reference = reference;
        auto source = Source();
        pal4::VasExceptionState destination{};
        (void)pal4::VasException::CopyConstruct(destination, source, runtime);
        Check(destination.words[4u] == 0x9000u &&
                  destination.words[5u] == 3u &&
                  destination.words[6u] == 0u, "deep copy fields");
        Check(source.words[4u] == 0x4000u && source.words[5u] == 3u,
              "deep copy preserves source");
        CheckCalls(runtime, {"base", "reference", "allocate", "copy",
                             "terminator"});
    }
}

void TestEmptyAndFailure() {
    auto source = Source();
    source.words[4u] = 0u;
    source.words[5u] = 0u;
    Runtime runtime;
    pal4::VasExceptionState destination{};
    (void)pal4::VasException::CopyConstruct(destination, source, runtime);
    Check(destination.words[4u] == 0x9000u && destination.words[5u] == 0u,
          "zero-length allocation");
    CheckCalls(runtime, {"base", "empty", "allocate", "copy",
                         "terminator"});

    runtime = Runtime{};
    runtime.allocation_succeeds = false;
    destination = {};
    (void)pal4::VasException::CopyConstruct(destination, source, runtime);
    Check(destination.words[4u] == 0u && destination.words[5u] == 0u &&
              destination.words[0u] == pal4::VasException::kVtable,
          "failed allocation preserves empty text state");
    CheckCalls(runtime, {"base", "empty", "allocate"});
}

void TestSelfCopy() {
    for (const auto remaining : {0u, 4u, 0xffffffffu}) {
        Runtime runtime;
        runtime.length_after_detach = remaining;
        auto state = Source();
        (void)pal4::VasException::CopyConstruct(state, state, runtime);
        Check(state.words[0u] == pal4::VasException::kVtable,
              "self-copy vtable");
        Check(runtime.detach_count == 2u, "self-copy double detach");
        if (remaining == 0u) {
            CheckCalls(runtime, {"base", "detach", "detach"});
        } else {
            CheckCalls(runtime, {"base", "detach", "copy", "allocate",
                                 "set-size", "detach"});
        }
    }
}

void TestExceptionCleanup() {
    Runtime runtime;
    runtime.reference = 0xfeu;
    runtime.throw_on_copy = true;
    auto source = Source();
    pal4::VasExceptionState destination{};
    bool caught = false;
    try {
        (void)pal4::VasException::CopyConstruct(destination, source, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught, "copy error propagates");
    CheckCalls(runtime, {"base", "reference", "allocate", "destroy-base"});
}

void TestDestructor() {
    for (const auto count : {std::uint8_t{0u}, std::uint8_t{1u},
                             std::uint8_t{2u}, std::uint8_t{0xffu}}) {
        Runtime runtime;
        runtime.reference = count;
        runtime.require_cleared_on_destroy = true;
        auto state = Source();
        pal4::VasException::Destroy(state, runtime);
        Check(state.words[4u] == 0u && state.words[5u] == 0u &&
                  state.words[6u] == 0u && state.words[3u] == 0x12345678u,
              "destructor clears only text fields before base cleanup");
        if (count == 0u || count == 0xffu) {
            CheckCalls(runtime, {"reference", "free-text", "destroy-base"});
        } else {
            CheckCalls(runtime, {"reference", "decrement", "destroy-base"});
        }
    }

    Runtime empty;
    empty.require_cleared_on_destroy = true;
    auto state = Source();
    state.words[4u] = 0u;
    pal4::VasException::Destroy(state, empty);
    CheckCalls(empty, {"destroy-base"});
}

}  // namespace

int main() {
    TestSharedCopy();
    TestDeepCopy();
    TestEmptyAndFailure();
    TestSelfCopy();
    TestExceptionCleanup();
    TestDestructor();
}
