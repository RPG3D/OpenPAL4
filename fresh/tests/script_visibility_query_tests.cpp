#include "pal4/script_visibility_query.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptVisibilityQueryRuntime {
    std::uint32_t name = 0x5000u;
    std::uint32_t entity = 0x7000u;
    std::uint32_t state = 1u;
    std::uint32_t field = 0x184u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1004u) {
            calls.emplace_back("name");
            return name;
        }
        Check(address == entity + field, "entity state offset");
        calls.emplace_back("state");
        return state;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty");
        return 0x4000u;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("object manager");
        return 0x2000u;
    }
    std::uint32_t MonsterManager() override {
        calls.emplace_back("monster manager");
        return 0x3000u;
    }
    std::uint32_t FindObject(std::uint32_t manager,
                             std::uint32_t resolved_name) override {
        Check(manager == 0x2000u && resolved_name ==
                  (name == 0u ? 0x4000u : name), "object lookup arguments");
        calls.emplace_back("find object");
        return entity;
    }
    std::uint32_t FindMonster(std::uint32_t manager,
                              std::uint32_t resolved_name) override {
        Check(manager == 0x3000u && resolved_name ==
                  (name == 0u ? 0x4000u : name), "monster lookup arguments");
        calls.emplace_back("find monster");
        return entity;
    }
};

void QueryBranches(bool monster) {
    const auto query = [monster](Runtime& runtime) {
        return monster ? pal4::ScriptVisibilityQuery::Monster(0x1000u, runtime)
                       : pal4::ScriptVisibilityQuery::Object(0x1000u, runtime);
    };
    const std::string manager = monster ? "monster manager" : "object manager";
    const std::string find = monster ? "find monster" : "find object";
    for (auto value : {0u, 1u, 0x80000000u}) {
        Runtime runtime;
        runtime.field = monster ? 0x1f0u : 0x184u;
        runtime.state = value;
        Check(query(runtime) == (value != 0u) &&
                  runtime.calls == std::vector<std::string>{
                      "name", manager, find, "state"},
              "query uses the entity's exact state field");
    }
    Runtime missing;
    missing.entity = 0u;
    Check(!query(missing) && missing.calls == std::vector<std::string>{
              "name", manager, find},
          "missing entity does not read state");
    Runtime empty;
    empty.name = 0u;
    empty.field = monster ? 0x1f0u : 0x184u;
    Check(query(empty) && empty.calls == std::vector<std::string>{
              "name", "empty", manager, find, "state"},
          "null name uses static empty string");
}

}  // namespace

int main() {
    try {
        QueryBranches(false);
        QueryBranches(true);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
