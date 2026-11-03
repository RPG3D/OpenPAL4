#include "pal4/game_data_skill_registry.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataSkillRegistryRuntime {
    std::vector<std::string> calls;
    std::vector<pal4::GameDataSkillField> appended;
    std::uint8_t process_result = 0x81;
    bool reverse_on_sort = false;

    void InitializeGlobalStorage() override { calls.emplace_back("globals"); }
    void RegisterGlobalCleanup() override { calls.emplace_back("atexit"); }
    void CleanupGlobalStorage() override { calls.emplace_back("cleanup"); }
    std::string SourceName(std::uint32_t address) override {
        calls.emplace_back("source");
        if (address >= 0x8ef9c8 && address < 0x8efa08 &&
            (address - 0x8ef9c8) % 16 == 0)
            return std::string(1, static_cast<char>(
                                      'A' + (address - 0x8ef9c8) / 16));
        if (address >= 0x8eeb90 && address < 0x8eebe0 &&
            (address - 0x8eeb90) % 16 == 0)
            return "SCP_ID_" + std::to_string(
                                   1 + (address - 0x8eeb90) / 16);
        throw std::runtime_error("unexpected source address");
    }
    std::string ErrorSuffix() override { return "!"; }
    void AppendField(std::vector<pal4::GameDataSkillField>& fields,
                     pal4::GameDataSkillField field) override {
        calls.emplace_back("append");
        appended.push_back(field);
        fields.push_back(std::move(field));
    }
    void Sort(std::vector<pal4::GameDataSkillField>& fields) override {
        calls.emplace_back("sort");
        Check(fields.size() == 31, "sort follows all 31 fields");
        if (reverse_on_sort) std::reverse(fields.begin(), fields.end());
    }
    std::uint8_t ProcessCollection(
        std::uint32_t object,
        const std::vector<pal4::GameDataSkillField>& fields) override {
        calls.emplace_back("process");
        Check(object == 0x6000 && fields.size() == 31,
              "process receives object and initialized fields");
        return process_result;
    }
};

void CheckRegistration() {
    using pal4::GameDataSkillFieldKind;
    Runtime runtime;
    pal4::GameDataSkillRegistry registry;
    const auto& fields = registry.Initialize(runtime);
    Check(fields.size() == 31 && runtime.appended.size() == 31,
          "31 fields are registered");
    Check(runtime.calls[0] == "globals" && runtime.calls[1] == "atexit" &&
              runtime.calls.back() == "sort",
          "global construction precedes fields and sort follows them");

    constexpr std::array<std::uint32_t, 31> offsets{
        0, 128, 4, 112, 116, 120, 124, 56, 60, 64, 68,
        72, 76, 80, 84, 88, 92, 96, 100, 104, 108,
        8, 24, 40, 132, 136, 140, 144, 160, 176, 192};
    constexpr std::array<std::string_view, 31> names{
        "ID!", "SKILL_DEPENDED!", "AI_CMD_TYPE!", "ATTACHED_SKILL!",
        "CONSUMED_RAGE!", "CONSUMED_MONEY!", "NEED_PROP_ID!",
        "A_LV_LMT", "B_LV_LMT", "C_LV_LMT", "D_LV_LMT",
        "SCP_ID_1!", "SCP_ID_2!", "SCP_ID_3!", "SCP_ID_4!",
        "SCP_ID_5!", "SCP_ID_1_NUMBER", "SCP_ID_2_NUMBER",
        "SCP_ID_3_NUMBER", "SCP_ID_4_NUMBER", "SCP_ID_5_NUMBER",
        "NAME!", "DESCRIPTION!", "PROP!", "ACTTYPE!", "TARGET!",
        "HIT_EXTRA!", "ANIMATION!", "EFFECT!", "TARGET_EF!",
        "TARGET_BIND!"};
    for (std::size_t i = 0; i < fields.size(); ++i) {
        Check(fields[i].offset == offsets[i] && fields[i].name == names[i],
              "field name and byte offset match IDA registration order");
        Check(fields[i].object_size == (i == 24 ? 32u : 24u),
              "descriptor allocation size");
        if (i >= 7 && i <= 10)
            Check(fields[i].source_address ==
                      0x8ef9c8 + static_cast<std::uint32_t>(i - 7) * 16,
                  "four level names use 16-byte source stride");
        if (i >= 11 && i <= 20)
            Check(fields[i].source_address ==
                      0x8eeb90 + static_cast<std::uint32_t>((i - 11) % 5) * 16,
                  "SCP_ID and NUMBER names share five source records");
    }
    for (std::size_t i = 0; i <= 20; ++i)
        Check(fields[i].kind == GameDataSkillFieldKind::numeric &&
                  fields[i].vtable == 0x843368,
              "numeric fields use the same vtable");
    for (std::size_t i : {21u, 22u, 23u, 27u, 28u, 29u, 30u})
        Check(fields[i].kind == GameDataSkillFieldKind::text &&
                  fields[i].vtable == 0x844574,
              "text fields use their own vtable");
    Check(fields[24].kind == GameDataSkillFieldKind::action_type &&
              fields[24].vtable == 0x844494 &&
              fields[24].source_address == 0x8ef5a8 &&
              fields[24].source_count == 3,
          "ACTTYPE has 32-byte descriptor and three-value source");
    Check(fields[25].kind == GameDataSkillFieldKind::numeric &&
              fields[25].vtable == 0x843368 &&
              fields[26].kind == GameDataSkillFieldKind::hit_extra &&
              fields[26].vtable == 0x8444dc,
          "TARGET and HIT_EXTRA use their distinct descriptors");

    const auto call_count = runtime.calls.size();
    Check(&registry.Initialize(runtime) == &fields &&
              runtime.calls.size() == call_count,
          "repeated initialization preserves one-time collection");
    registry.Cleanup(runtime);
    Check(runtime.calls.back() == "cleanup", "CRT cleanup delegates to collection");
}

void CheckWrapper() {
    Runtime runtime;
    runtime.reverse_on_sort = true;
    pal4::GameDataSkillRegistry registry;
    Check(registry.Process(0x6000, runtime) == 0x81 &&
              runtime.calls[runtime.calls.size() - 2] == "sort" &&
              runtime.calls.back() == "process",
          "wrapper processes the sorted collection and forwards result");
    Check(registry.Initialize(runtime).front().offset == 192,
          "wrapper forwards the runtime-sorted collection");
}

}  // namespace

int main() {
    try {
        CheckRegistration();
        CheckWrapper();
        std::cout << "game_data_skill_registry_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
