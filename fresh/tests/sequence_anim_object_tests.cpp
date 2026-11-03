#include "pal4/sequence_anim_object.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::array<std::uint32_t, 8> kDerived{
    0x84a8b8u, 0x84a8acu, 0x84a8a4u, 0x84a898u,
    0x84a888u, 0x84a87cu, 0x84a870u, 0x84a864u};
constexpr std::array<std::uint32_t, 8> kBase{
    0x84a1dcu, 0x84a1d0u, 0x84a1c8u, 0x84a1bcu,
    0x84a1acu, 0x84a1a0u, 0x84a194u, 0x84a178u};

void CheckFacets(const pal4::SequenceAnimObjectState& state,
                 const std::array<std::uint32_t, 8>& expected) {
    for (std::uint32_t i = 0u; i != expected.size(); ++i)
        Check(state.words[5u + i] == expected[i],
              "all eight facet vtables and order");
}

struct Runtime final : pal4::ISequenceAnimObjectRuntime {
    pal4::SequenceAnimObjectState* state = nullptr;
    std::uint32_t count = 7u;
    std::uint32_t writes = 0u;
    bool throw_on_read = false;
    std::uint32_t SystemInstance() override { return 0x5000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50a4u && state != nullptr &&
                  state->words[0u] == 0x84a8c8u,
              "instance count read after derived vtable installation");
        CheckFacets(*state, kDerived);
        if (throw_on_read) throw std::runtime_error("count");
        return count;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x50a4u, "EffectSystem instance count offset");
        count = value;
        ++writes;
    }
};

void TestLifecycle() {
    pal4::SequenceAnimObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.state = &state;
    Check(&pal4::SequenceAnimObject::Construct(state, runtime) == &state &&
              state.words[0u] == 0x84a8c8u && runtime.count == 8u &&
              runtime.writes == 1u && state.words[13u] == 0u &&
              state.words[30u] == 0u,
          "constructor installs object vtable, zeros two resource fields and counts");
    CheckFacets(state, kDerived);
    for (std::uint32_t i = 1u; i != 5u; ++i)
        Check(state.words[i] == 0u, "base reset clears owner prefix");
    Check(state.words[14u] == 0xaaaaaaaau &&
              state.words[31u] == 0xaaaaaaaau,
          "constructor preserves unrelated object words");
    state.words[13u] = 0x1234u;
    state.words[30u] = 0x5678u;
    pal4::SequenceAnimObject::Destroy(state, runtime);
    Check(state.words[0u] == 0x84b240u && runtime.count == 7u &&
              runtime.writes == 2u && state.words[13u] == 0x1234u &&
              state.words[30u] == 0x5678u,
          "destructor restores base, decrements count and preserves resources");
    CheckFacets(state, kBase);
}

void TestConstructorUnwind() {
    pal4::SequenceAnimObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    runtime.state = &state;
    runtime.throw_on_read = true;
    bool threw = false;
    try {
        (void)pal4::SequenceAnimObject::Construct(state, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.words[0u] == 0x84b240u &&
              state.words[13u] == 0u && state.words[30u] == 0u &&
              runtime.writes == 0u,
          "constructor failure restores base and rethrows");
    CheckFacets(state, kBase);
}

struct PropertyRuntime final : pal4::ISequenceAnimObjectRuntime {
    std::uint32_t reads = 0u;
    std::uint32_t SystemInstance() override { return 0u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5014u, "resource property address");
        ++reads;
        return 0x12345678u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {
        Check(false, "property must not write");
    }
};

void TestProperties() {
    pal4::SequenceAnimObjectState state{};
    state.words[30u] = 0x5000u;
    PropertyRuntime runtime;
    Check(pal4::SequenceAnimObject::ResourceProperty(state, runtime) == 0u &&
              runtime.reads == 0u, "inactive property gate");
    state.words[1u] = 0x100u;
    Check(pal4::SequenceAnimObject::ResourceProperty(state, runtime) == 0u &&
              runtime.reads == 0u, "property uses low activity byte");
    state.words[1u] = 1u;
    Check(pal4::SequenceAnimObject::ResourceProperty(state, runtime) ==
              0x12345678u && runtime.reads == 1u,
          "active resource property");
    Check(pal4::SequenceAnimObject::ClassName() == "SequnceAnimObject",
          "native misspelled class string");
}

}  // namespace

int main() {
    TestLifecycle();
    TestConstructorUnwind();
    TestProperties();
}
