#include "pal4/player_manager_loader.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerLoadRuntimeStub {
    std::vector<std::string> events;
    bool roles_present{true};
    std::uint32_t game_db_calls{};

    std::uint32_t Log(std::uint32_t line) override {
        events.push_back(line == 0x2bu ? "log-start" : "log-end");
        Check(line == 0x2bu || line == 0x49u,
              "loader uses observed log lines");
        return line == 0x49u ? 0xfeedu : 0u;
    }
    void ProcessScripts() override { events.emplace_back("scripts"); }
    std::uint32_t GameDB() override {
        ++game_db_calls;
        events.emplace_back("db");
        return 0x9000u;
    }
    std::uint32_t FindRole(std::uint32_t table,
                           std::uint32_t index) override {
        Check(table == 0x9020u && index < 4u,
              "each lookup uses GameDB role table and current index");
        events.push_back("find" + std::to_string(index));
        return roles_present && (index == 0u || index == 2u)
            ? 0xa000u + index : 0u;
    }
    void CopySlotData(std::uint32_t slot,
                      std::uint32_t record) override {
        const auto index = (slot - 0x5000u) / 0xb14u;
        Check(slot == 0x5000u + index * 0xb14u &&
                  record == 0xa000u + index,
              "record copies into matching player slot");
        events.push_back("copy" + std::to_string(index));
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        const auto index = (address - 0x5000u - 0xa1cu) / 0xb14u;
        Check(address == 0x5000u + index * 0xb14u + 0xa1cu,
              "model address comes from slot +0xa1c");
        events.push_back("model" + std::to_string(index));
        return index == 0u ? 0u : 0xb000u;
    }
    void CallLoadActor(std::uint32_t slot,
        std::uint32_t virtual_slot, std::uint32_t name,
        std::uint32_t model, std::uint32_t enabled) override {
        const auto index = (slot - 0x5000u) / 0xb14u;
        Check(virtual_slot == 0x30u &&
                  name == pal4::PlayerManagerLoader::kNames[index] &&
                  model == (index == 0u ? 0x841964u : 0xb000u) &&
                  enabled == 1u,
              "actor loader receives player name, model fallback and flag");
        events.push_back("load" + std::to_string(index));
    }
    void CallVectorMethod(std::uint32_t slot,
        std::uint32_t virtual_slot, pal4::PlayerManagerVector& vector,
        std::uint32_t option) override {
        const auto index = (slot - 0x5000u) / 0xb14u;
        Check(option == 0u &&
                  (virtual_slot == 0x4u || virtual_slot == 0x8u) &&
                  vector[0] == (index == 0u ? 0u : virtual_slot),
              "vectors start zero and persist between populated slots");
        events.push_back(std::string(virtual_slot == 0x4u ? "vec4" : "vec8") +
                         std::to_string(index));
        vector[0] = virtual_slot;
    }
    void SetStateFlag(std::uint32_t slot,
                      std::uint32_t value) override {
        Check(value == 0u, "loader clears slot state flag");
        events.push_back("flag" +
            std::to_string((slot - 0x5000u) / 0xb14u));
    }
    void CallScriptIndex(std::uint32_t script,
        std::uint32_t virtual_slot, std::uint32_t index) override {
        Check(script == 0x5000u + index * 0xb14u + 0x654u &&
                  virtual_slot == 0x18u,
              "script subobject receives matching player index");
        events.push_back("script" + std::to_string(index));
    }
    void ProcessLoadedData(std::uint32_t manager) override {
        Check(manager == 0x5000u, "postprocessor receives manager object");
        events.emplace_back("process");
    }
    void CleanupScripts() override { events.emplace_back("cleanup"); }
};

void TestPopulatedAndMissingRoles() {
    Runtime runtime;
    Check(pal4::PlayerManagerLoader::Load(0x5000u, runtime) == 0xfeedu &&
              runtime.game_db_calls == 4u,
          "loader visits all four role indexes and forwards final log result");
    const std::vector<std::string> expected{
        "log-start", "scripts",
        "db", "find0", "copy0", "model0", "load0", "vec40", "vec80",
        "flag0", "script0",
        "db", "find1",
        "db", "find2", "copy2", "model2", "load2", "vec42", "vec82",
        "flag2", "script2",
        "db", "find3", "process", "cleanup", "log-end"};
    Check(runtime.events == expected,
          "loader preserves branch and call order across all four slots");
}

void TestAllRolesMissing() {
    Runtime runtime;
    runtime.roles_present = false;
    Check(pal4::PlayerManagerLoader::Load(0x5000u, runtime) == 0xfeedu &&
              runtime.game_db_calls == 4u,
          "missing roles still advance all four indexes");
    const std::vector<std::string> expected{
        "log-start", "scripts", "db", "find0", "db", "find1",
        "db", "find2", "db", "find3", "process", "cleanup", "log-end"};
    Check(runtime.events == expected,
          "missing roles skip slot work but preserve postprocessing");
}

}  // namespace

int main() {
    TestPopulatedAndMissingRoles();
    TestAllRolesMissing();
}
