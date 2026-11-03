#include "pal4/combat_global_strings.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatGlobalStringRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint32_t> allocations;
    std::vector<std::string> calls;
    std::uint32_t next_data = 0x9001u;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(flag == 1u, "nonempty global string flag");
        calls.emplace_back("flag:" + std::to_string(record));
    }
    void Reallocate(std::uint32_t record,
                    std::uint32_t length) override {
        Check(length > 0u, "nonempty global literal length");
        calls.emplace_back("reallocate:" + std::to_string(record));
        if (fail_allocation) return;
        allocations[record] = next_data;
        words[record + 4u] = next_data;
        next_data += 0x100u;
    }
    void CopyLiteral(std::uint32_t destination,
                     std::string_view literal) override {
        Check(!literal.empty(), "nonempty global literal");
        calls.emplace_back("copy:" + std::string(literal) + ":" +
                           std::to_string(destination));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return 0xabcdu;
    }
    int RegisterCleanup(std::uint32_t callback) override {
        calls.emplace_back("register:" + std::to_string(callback));
        return -7;
    }
};

using Register = int (*)(std::uint8_t,
                          pal4::ICombatGlobalStringRuntime&);
using Initialize = std::uint8_t (*)(
    std::uint8_t, pal4::ICombatGlobalStringRuntime&);
using Cleanup = void (*)(pal4::ICombatGlobalStringRuntime&);

struct Entry {
    std::uint32_t record;
    std::uint32_t callback;
    std::uint32_t guard;
    std::uint8_t bit;
    const char* literal;
    Register registration;
    Initialize initialize;
    Cleanup cleanup;
};

constexpr std::array<Entry, 30> kEntries{{
    {0x8fa3a0u, 0x576de0u, 0x8fa39cu, 1u, "z01",
     &pal4::CombatGlobalStrings::RegisterZ01,
     &pal4::CombatGlobalStrings::InitializeZ01,
     &pal4::CombatGlobalStrings::CleanupZ01},
    {0x8fa38cu, 0x576f40u, 0x8fa39cu, 2u, "z03",
     &pal4::CombatGlobalStrings::RegisterZ03,
     &pal4::CombatGlobalStrings::InitializeZ03,
     &pal4::CombatGlobalStrings::CleanupZ03},
    {0x8fa37cu, 0x5770a0u, 0x8fa39cu, 4u, "z02",
     &pal4::CombatGlobalStrings::RegisterZ02,
     &pal4::CombatGlobalStrings::InitializeZ02,
     &pal4::CombatGlobalStrings::CleanupZ02},
    {0x8fa36cu, 0x577200u, 0x8fa39cu, 8u, "z07",
     &pal4::CombatGlobalStrings::RegisterZ07,
     &pal4::CombatGlobalStrings::InitializeZ07,
     &pal4::CombatGlobalStrings::CleanupZ07},
    {0x8fa35cu, 0x577360u, 0x8fa39cu, 16u, "z12",
     &pal4::CombatGlobalStrings::RegisterZ12,
     &pal4::CombatGlobalStrings::InitializeZ12,
     &pal4::CombatGlobalStrings::CleanupZ12},
    {0x8fa34cu, 0x5774c0u, 0x8fa39cu, 32u, "z10",
     &pal4::CombatGlobalStrings::RegisterZ10,
     &pal4::CombatGlobalStrings::InitializeZ10,
     &pal4::CombatGlobalStrings::CleanupZ10},
    {0x8fa33cu, 0x577620u, 0x8fa39cu, 64u, "z11",
     &pal4::CombatGlobalStrings::RegisterZ11,
     &pal4::CombatGlobalStrings::InitializeZ11,
     &pal4::CombatGlobalStrings::CleanupZ11},
    {0x8fa32cu, 0x577780u, 0x8fa39cu, 128u, "z05",
     &pal4::CombatGlobalStrings::RegisterZ05,
     &pal4::CombatGlobalStrings::InitializeZ05,
     &pal4::CombatGlobalStrings::CleanupZ05},
    {0x8fa31cu, 0x5778e0u, 0x8fa318u, 1u, "z04",
     &pal4::CombatGlobalStrings::RegisterZ04,
     &pal4::CombatGlobalStrings::InitializeZ04,
     &pal4::CombatGlobalStrings::CleanupZ04},
    {0x8fa308u, 0x577a40u, 0x8fa318u, 2u, "z06",
     &pal4::CombatGlobalStrings::RegisterZ06,
     &pal4::CombatGlobalStrings::InitializeZ06,
     &pal4::CombatGlobalStrings::CleanupZ06},
    {0x8fa2f8u, 0x577ba0u, 0x8fa318u, 4u, "z08",
     &pal4::CombatGlobalStrings::RegisterZ08,
     &pal4::CombatGlobalStrings::InitializeZ08,
     &pal4::CombatGlobalStrings::CleanupZ08},
    {0x8fa2e8u, 0x577d00u, 0x8fa318u, 8u, "z09",
     &pal4::CombatGlobalStrings::RegisterZ09,
     &pal4::CombatGlobalStrings::InitializeZ09,
     &pal4::CombatGlobalStrings::CleanupZ09},
    {0x8fa2d8u, 0x577e60u, 0x8fa318u, 16u, "z14",
     &pal4::CombatGlobalStrings::RegisterZ14,
     &pal4::CombatGlobalStrings::InitializeZ14,
     &pal4::CombatGlobalStrings::CleanupZ14},
    {0x8fa2c8u, 0x577fc0u, 0x8fa318u, 32u, "z15",
     &pal4::CombatGlobalStrings::RegisterZ15,
     &pal4::CombatGlobalStrings::InitializeZ15,
     &pal4::CombatGlobalStrings::CleanupZ15},
    {0x8fa2b8u, 0x578120u, 0x8fa318u, 64u, "z16",
     &pal4::CombatGlobalStrings::RegisterZ16,
     &pal4::CombatGlobalStrings::InitializeZ16,
     &pal4::CombatGlobalStrings::CleanupZ16},
    {0x8fa2a8u, 0x578280u, 0x8fa318u, 128u, "z17",
     &pal4::CombatGlobalStrings::RegisterZ17,
     &pal4::CombatGlobalStrings::InitializeZ17,
     &pal4::CombatGlobalStrings::CleanupZ17},
    {0x8fa298u, 0x5783e0u, 0x8fa294u, 1u, "z18",
     &pal4::CombatGlobalStrings::RegisterZ18,
     &pal4::CombatGlobalStrings::InitializeZ18,
     &pal4::CombatGlobalStrings::CleanupZ18},
    {0x8fa284u, 0x578540u, 0x8fa294u, 2u, "hook_area",
     &pal4::CombatGlobalStrings::RegisterHookArea,
     &pal4::CombatGlobalStrings::InitializeHookArea,
     &pal4::CombatGlobalStrings::CleanupHookArea},
    {0x8fa274u, 0x5786a0u, 0x8fa294u, 4u, "hook_ef_pos",
     &pal4::CombatGlobalStrings::RegisterHookEfPos,
     &pal4::CombatGlobalStrings::InitializeHookEfPos,
     &pal4::CombatGlobalStrings::CleanupHookEfPos},
    {0x8fa264u, 0x578800u, 0x8fa294u, 8u, "CombatScene",
     &pal4::CombatGlobalStrings::RegisterCombatScene,
     &pal4::CombatGlobalStrings::InitializeCombatScene,
     &pal4::CombatGlobalStrings::CleanupCombatScene},
    {0x8fa254u, 0x578960u, 0x8fa294u, 16u, "CombatArea",
     &pal4::CombatGlobalStrings::RegisterCombatArea,
     &pal4::CombatGlobalStrings::InitializeCombatArea,
     &pal4::CombatGlobalStrings::CleanupCombatArea},
    {0x8fa244u, 0x578ac0u, 0x8fa294u, 32u, "StartType",
     &pal4::CombatGlobalStrings::RegisterStartType,
     &pal4::CombatGlobalStrings::InitializeStartType,
     &pal4::CombatGlobalStrings::CleanupStartType},
    {0x8fa234u, 0x578c20u, 0x8fa294u, 64u, "CombatResult",
     &pal4::CombatGlobalStrings::RegisterCombatResult,
     &pal4::CombatGlobalStrings::InitializeCombatResult,
     &pal4::CombatGlobalStrings::CleanupCombatResult},
    {0x8fa224u, 0x578d80u, 0x8fa294u, 128u, "CbMonster",
     &pal4::CombatGlobalStrings::RegisterCbMonster,
     &pal4::CombatGlobalStrings::InitializeCbMonster,
     &pal4::CombatGlobalStrings::CleanupCbMonster},
    {0x8fa214u, 0x578ee0u, 0x8fa210u, 1u, "MonsterId",
     &pal4::CombatGlobalStrings::RegisterMonsterId,
     &pal4::CombatGlobalStrings::InitializeMonsterId,
     &pal4::CombatGlobalStrings::CleanupMonsterId},
    {0x8fa200u, 0x579040u, 0x8fa210u, 2u, "CombatPosition",
     &pal4::CombatGlobalStrings::RegisterCombatPosition,
     &pal4::CombatGlobalStrings::InitializeCombatPosition,
     &pal4::CombatGlobalStrings::CleanupCombatPosition},
    {0x8fa1f0u, 0x5791a0u, 0x8fa210u, 4u, "MonsterAbState",
     &pal4::CombatGlobalStrings::RegisterMonsterAbState,
     &pal4::CombatGlobalStrings::InitializeMonsterAbState,
     &pal4::CombatGlobalStrings::CleanupMonsterAbState},
    {0x8fa1e0u, 0x579300u, 0x8fa210u, 8u, "MonsterGroup",
     &pal4::CombatGlobalStrings::RegisterMonsterGroup,
     &pal4::CombatGlobalStrings::InitializeMonsterGroup,
     &pal4::CombatGlobalStrings::CleanupMonsterGroup},
    {0x8fa3c4u, 0x57a2a0u, 0x8fa3c0u, 1u, "EventShowFight",
     &pal4::CombatGlobalStrings::RegisterEventShowFight,
     &pal4::CombatGlobalStrings::InitializeEventShowFight,
     &pal4::CombatGlobalStrings::CleanupEventShowFight},
    {0x8fa3b0u, 0x57a400u, 0x8fa3c0u, 2u, "EventCloseSystem",
     &pal4::CombatGlobalStrings::RegisterEventCloseSystem,
     &pal4::CombatGlobalStrings::InitializeEventCloseSystem,
     &pal4::CombatGlobalStrings::CleanupEventCloseSystem},
}};

void RegistrationAndInitialization() {
    Runtime runtime;
    for (const auto& entry : kEntries) {
        runtime.calls.clear();
        const auto length =
            static_cast<std::uint32_t>(std::string_view(entry.literal).size());
        Check(entry.registration(0x5au, runtime) == -7 &&
                  runtime.bytes[entry.record] == 0x5au &&
                  runtime.words[entry.record + 4u] ==
                      runtime.allocations[entry.record] &&
                  runtime.words[entry.record + 8u] == length &&
                  runtime.bytes[runtime.allocations[entry.record] + length] ==
                      0u,
              "registration initializes record and returns atexit result");
        Check(runtime.calls.size() == 10u &&
                  runtime.calls[0] == "byte:" +
                      std::to_string(entry.record) + ":90" &&
                  runtime.calls[1] == "word:" +
                      std::to_string(entry.record + 4u) + ":0" &&
                  runtime.calls[4] == "flag:" +
                      std::to_string(entry.record) &&
                  runtime.calls[5] == "reallocate:" +
                      std::to_string(entry.record) &&
                  runtime.calls[6] == std::string("copy:") + entry.literal + ":" +
                      std::to_string(runtime.allocations[entry.record]) &&
                  runtime.calls.back() == "register:" +
                      std::to_string(entry.callback),
              "record initialization and callback registration order");
    }

    Runtime direct;
    Check(kEntries[0].initialize(0u, direct) == 1u,
          "direct initializer returns low byte of allocated text pointer");
    for (const auto& entry : kEntries) {
        Runtime failed;
        failed.fail_allocation = true;
        bool raised = false;
        try {
            static_cast<void>(entry.registration(0u, failed));
        } catch (const std::bad_alloc&) {
            raised = true;
        }
        Check(raised && failed.calls.back() ==
                            "reallocate:" + std::to_string(entry.record),
              "failed allocation stops before atexit for every record");
    }
}

void CleanupGuardsAndReferences() {
    Runtime runtime;
    for (const auto& entry : kEntries)
        static_cast<void>(entry.registration(0u, runtime));
    const auto first_header = runtime.allocations[kEntries[0].record] - 1u;
    const auto second_header = runtime.allocations[kEntries[1].record] - 1u;
    const auto third_header = runtime.allocations[kEntries[2].record] - 1u;
    runtime.bytes[first_header] = 0u;
    runtime.bytes[second_header] = 3u;
    runtime.bytes[third_header] = 0xffu;
    for (std::size_t index = 3u; index < kEntries.size(); ++index)
        runtime.bytes[runtime.allocations[kEntries[index].record] - 1u] =
            index == 4u ? 2u : 0u;
    runtime.calls.clear();
    for (const auto& entry : kEntries) {
        const auto prior = runtime.bytes[entry.guard];
        entry.cleanup(runtime);
        Check(runtime.bytes[entry.guard] == (prior | entry.bit),
              "each cleanup sets its own guard bit");
    }
    Check(runtime.bytes[pal4::CombatGlobalStrings::kCleanupGuard] == 255u &&
              runtime.bytes[0x8fa318u] == 255u &&
              runtime.bytes[0x8fa294u] == 255u &&
              runtime.bytes[0x8fa210u] == 15u &&
              runtime.bytes[0x8fa3c0u] == 3u &&
              runtime.bytes[second_header] == 2u &&
              runtime.words[kEntries[0].record + 4u] == 0u &&
              runtime.words[kEntries[1].record + 4u] == 0u &&
              runtime.words[kEntries[2].record + 4u] == 0u &&
              runtime.bytes[runtime.allocations[kEntries[4].record] - 1u] ==
                  1u &&
              runtime.words[kEntries[5].record + 4u] == 0u &&
              runtime.words[kEntries[8].record + 4u] == 0u &&
              runtime.words[kEntries[11].record + 4u] == 0u &&
              runtime.words[kEntries[27].record + 4u] == 0u &&
              runtime.words[kEntries[29].record + 4u] == 0u,
          "two cleanup guards and reference branches");
    Check(runtime.calls[0] == "byte:9413532:1" &&
              runtime.calls[1] == "free:" +
                  std::to_string(first_header) &&
              runtime.calls[5] == "byte:9413532:3" &&
              runtime.calls[6] == "byte:" +
                  std::to_string(second_header) + ":2" &&
              runtime.calls[10] == "byte:9413532:7" &&
              runtime.calls[11] == "free:" +
                  std::to_string(third_header),
          "each cleanup sets guard before releasing its buffer");
    runtime.calls.clear();
    for (const auto& entry : kEntries) entry.cleanup(runtime);
    Check(runtime.calls.empty(), "guarded cleanup is idempotent");
}

}  // namespace

int main() {
    try {
        RegistrationAndInitialization();
        CleanupGuardsAndReferences();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
