#include "pal4/clump_object.h"

#include <array>
#include <cstdint>
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

constexpr std::array<std::uint32_t, 12> kBase{
    0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1acu, 0x84a1dcu, 0x84a430u, 0x84a420u,
    0x84a1bcu, 0x84a408u, 0x84a184u, 0x84a178u};
constexpr std::array<std::uint32_t, 12> kDerived{
    0x84a3a4u, 0x84a398u, 0x84a38cu, 0x84a384u,
    0x84a374u, 0x84a368u, 0x84a358u, 0x84a348u,
    0x84a33cu, 0x84a324u, 0x84a314u, 0x84a308u};

void CheckFacets(const pal4::ClumpObjectState& state,
                 const std::array<std::uint32_t, 12>& facets) {
    for (std::size_t i = 0u; i < facets.size(); ++i)
        Check(state.words[5u + i] == facets[i], "facet offset and vtable");
}

struct Runtime final : pal4::IClumpObjectRuntime {
    std::vector<std::string> events;
    std::uint32_t count = 6u;
    bool fail_xml = false;
    bool fail_instance = false;
    bool saw_base_xml = false;
    bool saw_derived_destroy = false;

    void InitializeXml(pal4::ClumpObjectState& state,
                       std::uint32_t offset) override {
        events.push_back("xml_init:" + std::to_string(offset));
        saw_base_xml = state.words[0u] == 0x84b240u &&
                       state.words[5u] == kBase[0u] &&
                       state.words[16u] == kBase[11u] &&
                       state.words[18u] == 0u && state.words[19u] == 0u;
        if (fail_xml)
            throw std::runtime_error("xml");
        state.words[27u] = 0x11223344u;
    }
    void DestroyXml(pal4::ClumpObjectState& state,
                    std::uint32_t offset) override {
        events.push_back("xml_destroy:" + std::to_string(offset));
        saw_derived_destroy = state.words[0u] == 0x84a3b0u &&
                              state.words[5u] == kDerived[0u];
    }
    void ReleaseText(pal4::ClumpObjectState&,
                     std::uint32_t offset) override {
        events.push_back("text:" + std::to_string(offset));
    }
    void ReleaseBuffer(std::uint32_t address) override {
        events.push_back("buffer:" + std::to_string(address));
    }
    void SetErrorFlag(pal4::ClumpObjectState&,
                      std::uint32_t offset, std::uint8_t flag) override {
        Check(flag == 1u, "text finalization flag");
        events.push_back("flag:" + std::to_string(offset));
    }
    std::uint32_t SystemInstance() override {
        events.push_back("instance");
        if (fail_instance)
            throw std::runtime_error("instance");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x50a4u, "system count address");
        events.push_back("read");
        return count;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x50a4u, "system count write address");
        events.push_back("write");
        count = value;
    }
};

void TestConstructionAndDestruction() {
    pal4::ClumpObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    Check(&pal4::ClumpObject::Construct(state, 0x5au, runtime) ==
              &state && runtime.saw_base_xml && runtime.count == 7u &&
              runtime.events == std::vector<std::string>{
                  "xml_init:108", "instance", "read", "write"},
          "constructor orders XML, derived facets and count increment");
    Check(state.words[0u] == 0x84a3b0u, "derived vtable");
    CheckFacets(state, kDerived);
    for (std::size_t i = 1u; i <= 4u; ++i)
        Check(state.words[i] == 0u, "base reset");
    for (std::size_t i = 44u; i <= 51u; ++i)
        Check(state.words[i] == 0u, "eight loop-cleared words");
    for (const auto offset : {72u, 76u, 224u, 228u, 232u, 240u,
                              244u, 248u, 256u, 260u, 264u,
                              312u, 316u, 320u})
        Check(state.words[offset / 4u] == 0u, "exact zero fields");
    for (const auto offset : {220u, 236u, 252u, 308u})
        Check(state.words[offset / 4u] == 0xa5a5a55au,
              "indeterminate stack byte is explicit");
    Check(state.words[42u] == 0xa5a5a500u &&
              state.words[27u] == 0x11223344u &&
              state.words[17u] == 0xa5a5a5a5u,
          "byte zero and untouched words are preserved");

    state.words[78u] = 0x3000u;
    runtime.events.clear();
    pal4::ClumpObject::Destroy(state, runtime);
    Check(runtime.events == std::vector<std::string>{
              "instance", "read", "write", "buffer:12288",
              "flag:252", "flag:236", "flag:220", "xml_destroy:108"} &&
              runtime.count == 6u && runtime.saw_derived_destroy,
          "destructor releases buffer, texts and XML after count decrement");
    Check(state.words[78u] == 0u && state.words[79u] == 0u &&
              state.words[80u] == 0u && state.words[0u] == 0x84b240u,
          "destructor clears buffer and restores base vtable");
    CheckFacets(state, kBase);
}

void TestConstructionUnwind() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    runtime.fail_xml = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::ClumpObject::Construct(
            state, 0u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "xml_init:108"} && state.words[0u] == 0x84b240u,
          "failed XML construction leaves base state without XML destruction");
    CheckFacets(state, kBase);

    runtime = Runtime{};
    runtime.fail_instance = true;
    runtime.events.clear();
    threw = false;
    try {
        static_cast<void>(pal4::ClumpObject::Construct(
            state, 0x22u, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<std::string>{
              "xml_init:108", "instance", "buffer:0", "text:252",
              "text:236", "text:220", "xml_destroy:108"} &&
              runtime.saw_derived_destroy && runtime.count == 6u &&
              state.words[0u] == 0x84b240u,
          "failed singleton access unwinds owned fields in reverse order");
    CheckFacets(state, kBase);
}

}  // namespace

int main() {
    TestConstructionAndDestruction();
    TestConstructionUnwind();
}
