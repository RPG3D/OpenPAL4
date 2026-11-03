#include "pal4/script_gob_position.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptGobPositionRuntime {
    std::uint32_t name = 0x5000u;
    std::uint32_t object = 0x6000u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1004u, "GOB name payload offset");
        calls.emplace_back("name");
        return name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty");
        return 0x4000u;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("manager");
        return 0x2000u;
    }
    std::uint32_t FindObject(std::uint32_t manager,
                             std::uint32_t resolved_name) override {
        Check(manager == 0x2000u && resolved_name ==
                  (name == 0u ? 0x4000u : name), "GOB lookup operands");
        calls.emplace_back("find");
        return object;
    }
    std::uint32_t SetPosition(
        std::uint32_t found_object, const std::array<float, 3>& position,
        std::uint32_t flags) override {
        Check(found_object == object && flags == 0u &&
                  position == std::array<float, 3>{1.25f, -2.5f, 3.75f},
              "GOB XYZ and immediate update flag");
        calls.emplace_back("set position");
        return 0x81u;
    }
};

void PositionBranches() {
    Runtime normal;
    Check(pal4::ScriptGobPosition::Set(
              0x1000u, 1.25f, -2.5f, 3.75f, normal) == 0x81u &&
              normal.calls == std::vector<std::string>{
                  "name", "manager", "find", "set position"},
          "GOB position forwards ordered float coordinates");
    Runtime empty;
    empty.name = 0u;
    Check(pal4::ScriptGobPosition::Set(
              0x1000u, 1.25f, -2.5f, 3.75f, empty) == 0x81u &&
              empty.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find", "set position"},
          "null GOB name uses static empty string");
    Runtime missing;
    missing.object = 0u;
    Check(pal4::ScriptGobPosition::Set(
              0x1000u, 1.25f, -2.5f, 3.75f, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "manager", "find"},
          "missing GOB skips position update");
}

}  // namespace

int main() {
    try {
        PositionBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
