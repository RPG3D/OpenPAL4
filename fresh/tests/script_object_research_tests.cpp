#include "pal4/script_object_research.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptObjectResearchRuntime {
    std::uint32_t name = 0x3000u;
    std::uint32_t object = 0x5000u;
    std::uint32_t written = 0xffffffffu;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1004u, "string payload offset");
        calls.emplace_back("name"); return name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0x4000u;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("manager"); return 0x2000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::uint32_t resolved_name) override {
        Check(manager == 0x2000u && resolved_name ==
                  (name == 0u ? 0x4000u : name), "object lookup operands");
        calls.emplace_back("find"); return object;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == object + 0x18cu, "research gate offset");
        calls.emplace_back("write"); written = value;
    }
};

void ResearchFlag() {
    for (const auto input : {0u, 1u, 0x1234u}) {
        Runtime runtime;
        Check(pal4::ScriptObjectResearch::Set(0x1000u, input, runtime) ==
                  runtime.object && runtime.written == (input & 0xffu) &&
              runtime.calls == std::vector<std::string>{
                  "name", "manager", "find", "write"},
              "research writes low byte as dword and returns object");
    }
    Runtime empty;
    empty.name = 0u;
    empty.object = 0u;
    Check(pal4::ScriptObjectResearch::Set(0x1000u, 1u, empty) == 0u &&
              empty.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find"},
          "missing object with empty name does not write");
}

}  // namespace

int main() {
    try {
        ResearchFlag();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
