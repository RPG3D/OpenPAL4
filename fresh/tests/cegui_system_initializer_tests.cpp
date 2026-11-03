#include "pal4/cegui_system_initializer.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint64_t MappingHash(
    const std::vector<pal4::CeguiKeyMapping>& mappings) {
    std::uint64_t hash = 0xcbf29ce484222325ull;
    for (const auto mapping : mappings) {
        for (const auto word : {mapping.windows_code,
                                mapping.cegui_code}) {
            for (std::uint32_t byte = 0u; byte < 4u; ++byte) {
                hash ^= (word >> (byte * 8u)) & 0xffu;
                hash *= 0x100000001b3ull;
            }
        }
    }
    return hash;
}

class Runtime final : public pal4::ICeguiSystemRuntime {
public:
    pal4::CeguiSystemState* state = nullptr;
    std::uint32_t allocation = 0x9000u;
    std::vector<pal4::CeguiKeyMapping> mappings;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t renderer_seen = 0u;
    std::uint32_t provider_seen = 0u;

    void ConstructObject(std::uint32_t address) override {
        Check(address == 0x1004u && state != nullptr &&
                  state->Bytes()[0] == 0u,
              "base object follows byte reset at self plus four");
        events.emplace_back("object");
    }
    void InsertKeyMapping(std::uint32_t object,
                          pal4::CeguiKeyMapping mapping) override {
        Check(object == 0x1004u, "mapping targets embedded object");
        mappings.push_back(mapping);
    }
    std::uint32_t AllocateSystem(std::uint32_t bytes) override {
        Check(bytes == 0x1b0u && mappings.size() == 100u,
              "CEGUI allocation follows all key mappings");
        events.emplace_back("allocate");
        return allocation;
    }
    void ConstructSystemWithProvider(
        std::uint32_t address, std::uint32_t renderer,
        std::uint32_t provider, std::string_view log_filename) override {
        Check(address == 0x9000u && log_filename == "CEGUI.log",
              "explicit-provider constructor inputs");
        renderer_seen = renderer;
        provider_seen = provider;
        events.emplace_back("provider");
    }
    void ConstructSystemWithDefaultProvider(
        std::uint32_t address, std::uint32_t renderer,
        std::string_view log_filename) override {
        Check(address == 0x9000u && log_filename == "CEGUI.log",
              "default-provider constructor inputs");
        renderer_seen = renderer;
        events.emplace_back("default-provider");
    }
    std::uint32_t GetLogger() override {
        events.emplace_back("logger");
        return 0xa000u;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write");
    }
};

void OrderedMappingsAndExplicitProvider() {
    pal4::CeguiSystemState state;
    state.Bytes().fill(0xaau);
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiSystemInitializer::Initialize(
              state, 0x1000u, 0x2000u, 0x3000u,
              1280, 800, runtime) == 0x1000u,
          "initializer returns its input object");
    Check(runtime.mappings.size() == 100u &&
              MappingHash(runtime.mappings) == 0x384650d7e6e983afull,
          "all 100 IDA key mappings retain exact order and values");
    Check(runtime.mappings[0].windows_code == 8u &&
              runtime.mappings[0].cegui_code == 14u &&
              runtime.mappings[2].windows_code == 13u &&
              runtime.mappings[12].windows_code == 36u &&
              runtime.mappings[13].windows_code == 35u &&
              runtime.mappings[73].windows_code == 109u &&
              runtime.mappings[74].windows_code == 108u &&
              runtime.mappings.back().windows_code == 165u &&
              runtime.mappings.back().cegui_code == 184u,
          "special and nonascending entries keep observed positions");
    Check(runtime.events == std::vector<std::string>{
              "object", "allocate", "provider", "write",
              "logger", "write"} &&
              runtime.renderer_seen == 0x2000u &&
              runtime.provider_seen == 0x3000u &&
              runtime.words.at(0x9000u) == 0x841ea8u &&
              runtime.words.at(0xa000u) == 0u,
          "system construction, vtable and logger writes follow map setup");
    Check(state.ReadFloat(0x14u) == 0.75f &&
              state.ReadFloat(0x18u) == 0.75f &&
              state.ReadFloat(0x1cu) == -106.66669f &&
              state.Bytes()[1] == 0xaau,
          "1280 by 800 special scaling and untouched adjacent byte");
}

void DefaultProviderAndGeneralScaling() {
    pal4::CeguiSystemState state;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CeguiSystemInitializer::Initialize(
              state, 0x1000u, 0x2500u, 0u,
              1920, 1080, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "object", "allocate", "default-provider", "write",
                  "logger", "write"},
          "null resource provider selects default CEGUI constructor");
    Check(std::fabs(state.ReadFloat(0x14u) - 800.0f / 1920.0f) < 1e-6f &&
              std::fabs(state.ReadFloat(0x18u) - 600.0f / 1080.0f) < 1e-6f &&
              state.ReadFloat(0x1cu) == 0.0f,
          "general display dimensions use independent X/Y scale ratios");
}

void FailedSystemAllocationStillClearsLoggerAndScales() {
    pal4::CeguiSystemState state;
    Runtime runtime;
    runtime.state = &state;
    runtime.allocation = 0u;
    Check(pal4::CeguiSystemInitializer::Initialize(
              state, 0x1000u, 0x2000u, 0x3000u,
              800, 600, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "object", "allocate", "logger", "write"} &&
              runtime.words.size() == 1u &&
              runtime.words.at(0xa000u) == 0u,
          "null CEGUI allocation skips constructor but keeps later writes");
    Check(state.ReadFloat(0x14u) == 1.0f &&
              state.ReadFloat(0x18u) == 1.0f &&
              state.ReadFloat(0x1cu) == 0.0f,
          "scales are still initialized after system allocation failure");
    pal4::CeguiRuntimeStub stub;
    pal4::CeguiSystemState stub_state;
    Check(pal4::CeguiSystemInitializer::Initialize(
              stub_state, 0x1000u, 0u, 0u,
              800, 600, stub) == 0x1000u,
          "linkable CEGUI stub runs without the external library");
}

}  // namespace

int main() {
    OrderedMappingsAndExplicitProvider();
    DefaultProviderAndGeneralScaling();
    FailedSystemAllocationStillClearsLoggerAndScales();
    std::cout << "3 CEGUI initializer test cases passed\n";
}
