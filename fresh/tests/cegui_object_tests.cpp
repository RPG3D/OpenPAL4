#include "pal4/cegui_object.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiObjectRuntime {
public:
    pal4::CeguiObjectState* state = nullptr;
    int calls = 0;

    void InitializeList(std::uint32_t address) override {
        Check(address == 0x5000u && state != nullptr,
              "list initialization receives object address");
        Check(state->Bytes()[0] == 0x44u &&
                  state->Bytes()[1] == 0x33u &&
                  state->Bytes()[8] == 0x55u,
              "three field writes precede list initialization");
        ++calls;
    }
};

void ConstructorWritesObservedBytes() {
    pal4::CeguiObjectState state;
    state.Bytes().fill(0xaau);
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiObject::Construct(
              state, 0x5000u, 0x33u, 0x44u, 0x55u,
              runtime) == 0x5000u && runtime.calls == 1,
          "constructor returns object after initializing list");
    for (std::uint32_t offset = 0u;
         offset < pal4::CeguiObjectState::kObservedBytes; ++offset) {
        if (offset != 0u && offset != 1u && offset != 8u) {
            Check(state.Bytes()[offset] == 0xaau,
                  "constructor preserves every unobserved byte");
        }
    }
}

class ListRuntime final : public pal4::ICeguiObjectListRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> write_addresses;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 0x18u, "list allocates 24-byte tree nodes");
        const auto address = 0x5000u +
                             static_cast<std::uint32_t>(allocations.size()) *
                                 0x100u;
        allocations.push_back(address);
        return address;
    }
    void Free(const std::uint32_t address) override {
        Check(allocations.size() == 1u,
              "unused nil candidate is freed before header allocation");
        freed.push_back(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        write_addresses.push_back(address);
        words[address] = value;
    }
};

void FirstListInitializationCreatesNil() {
    pal4::CeguiObjectState state;
    state.Bytes().fill(0xaau);
    pal4::CeguiTreeGlobals globals;
    ListRuntime runtime;
    Check(pal4::CeguiObject::InitializeList(
              state, globals, runtime) == 0x5100u &&
              globals.nil == 0x5000u && globals.reference_count == 1u &&
              runtime.freed.empty() && runtime.allocations.size() == 2u,
          "first list initialization retains new shared nil");
    Check(runtime.words.at(0x5014u) == 1u &&
              runtime.words.at(0x5004u) == 0u &&
              runtime.words.at(0x5000u) == 0u &&
              runtime.words.at(0x5008u) == 0u,
          "nil has black color and zero links");
    Check(state.ReadDword(4u) == 0x5100u &&
              state.ReadDword(0x0cu) == 0u &&
              runtime.words.at(0x5104u) == 0x5000u &&
              runtime.words.at(0x5114u) == 0u &&
              runtime.words.at(0x5100u) == 0x5100u &&
              runtime.words.at(0x5108u) == 0x5100u,
          "object and header fields match IDA writes");
    Check(runtime.write_addresses == std::vector<std::uint32_t>{
              0x5014u, 0x5004u, 0x5000u, 0x5008u,
              0x5104u, 0x5114u, 0x5100u, 0x5108u},
          "first initialization writes nil before header");
    for (std::uint32_t index = 0u; index < 16u; ++index) {
        if ((index < 4u) || (index >= 8u && index < 12u)) {
            Check(state.Bytes()[index] == 0xaau,
                  "list initialization preserves unrelated object bytes");
        }
    }
}

void ExistingNilIsReused() {
    pal4::CeguiObjectState state;
    pal4::CeguiTreeGlobals globals{3u, 0x9000u};
    ListRuntime runtime;
    Check(pal4::CeguiObject::InitializeList(
              state, globals, runtime) == 0x5100u &&
              globals.nil == 0x9000u && globals.reference_count == 4u &&
              runtime.freed == std::vector<std::uint32_t>{0x5000u},
          "existing nil frees candidate and increments reference count");
    Check(runtime.words.at(0x5104u) == 0x9000u &&
              state.ReadDword(4u) == 0x5100u,
          "new header points to preexisting nil");
    Check(runtime.words.count(0x5000u) == 0u &&
              runtime.words.count(0x5008u) == 0u,
          "discarded candidate receives only preliminary writes");
}

class CombinedRuntime final : public pal4::ICeguiObjectRuntime {
public:
    pal4::CeguiObjectState& state;
    pal4::CeguiTreeGlobals& globals;
    ListRuntime& memory;

    CombinedRuntime(pal4::CeguiObjectState& object,
                    pal4::CeguiTreeGlobals& tree,
                    ListRuntime& list_memory)
        : state(object), globals(tree), memory(list_memory) {}

    void InitializeList(const std::uint32_t address) override {
        Check(address == 0x4000u && state.Bytes()[0] == 0x44u &&
                  state.Bytes()[1] == 0x33u && state.Bytes()[8] == 0x55u,
              "constructor writes object bytes before list initialization");
        static_cast<void>(pal4::CeguiObject::InitializeList(
            state, globals, memory));
    }
};

void ConstructorCanUseReconstructedListInitializer() {
    pal4::CeguiObjectState state;
    pal4::CeguiTreeGlobals globals;
    ListRuntime memory;
    CombinedRuntime runtime(state, globals, memory);
    Check(pal4::CeguiObject::Construct(
              state, 0x4000u, 0x33u, 0x44u, 0x55u,
              runtime) == 0x4000u &&
              state.ReadDword(4u) == 0x5100u &&
              globals.nil == 0x5000u,
          "object constructor composes with recovered list initializer");
}

}  // namespace

int main() {
    ConstructorWritesObservedBytes();
    FirstListInitializationCreatesNil();
    ExistingNilIsReused();
    ConstructorCanUseReconstructedListInitializer();
    std::cout << "4 CEGUI object test cases passed\n";
}
