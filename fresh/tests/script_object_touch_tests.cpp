#include "pal4/script_object_touch.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptObjectTouchRuntime {
    std::vector<std::uint32_t> managers{0x1000u, 0x2000u};
    std::uint32_t object = 0x9000u;
    std::size_t next_manager = 0u;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return managers.at(next_manager++);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(next_manager != 0u &&
              address == managers.at(next_manager - 1u) + 0x2c50u,
              "current player index at manager +0x2c50");
        calls.emplace_back("active-index");
        return 1u;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("object-manager"); return 0x8000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x8000u, "object lookup manager");
        calls.emplace_back("find:" + std::string(name));
        return object;
    }
    std::uint32_t Touch(std::uint32_t found,
                        std::uint32_t direction) override {
        Check(found == object &&
              direction == managers[1] + 0xb14u + 0xacu,
              "virtual +0x50 receives current player +0xac");
        calls.emplace_back("touch");
        return 0xfeedu;
    }
};

void NormalAndMissingObject() {
    Runtime normal;
    Check(pal4::ScriptObjectTouch::Run("stone", normal) == 0xfeedu &&
          normal.calls == std::vector<std::string>{
              "player-manager", "active-index", "object-manager",
              "find:stone", "player-manager", "active-index", "touch"},
          "object touch re-reads active player after lookup");

    Runtime missing;
    missing.object = 0u;
    Check(pal4::ScriptObjectTouch::Run(nullptr, missing) == 0u &&
          missing.calls == std::vector<std::string>{
              "player-manager", "active-index", "object-manager",
              "find:"},
          "null name falls back to empty and missing object stops");
}

void ZeroPlayerAddress() {
    Runtime first;
    first.managers[0] = 0u - 0xb14u;
    Check(pal4::ScriptObjectTouch::Run("stone", first) == 0u &&
          first.calls == std::vector<std::string>{
              "player-manager", "active-index"},
          "zero first player address bypasses object lookup");

    Runtime second;
    second.managers[1] = 0u - 0xb14u;
    Check(pal4::ScriptObjectTouch::Run("stone", second) == 0u &&
          second.calls == std::vector<std::string>{
              "player-manager", "active-index", "object-manager",
              "find:stone", "player-manager", "active-index"},
          "zero second player address bypasses virtual touch");
}

}  // namespace

int main() {
    try {
        NormalAndMissingObject();
        ZeroPlayerAddress();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
