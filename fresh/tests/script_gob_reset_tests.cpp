#include "pal4/script_gob_reset.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptGobResetRuntime {
    std::string expected_name = "gob";
    std::uint32_t object = 0x2000u;
    std::vector<std::string> calls;

    std::uint32_t ObjectManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t FindObject(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x1000u && name == expected_name,
              "GOB lookup manager and name");
        calls.emplace_back("find");
        return object;
    }
    std::uint32_t InvokeReset(std::uint32_t found) override {
        Check(found == 0x2000u, "GOB virtual reset receiver");
        calls.emplace_back("reset");
        return 0xabcd1234u;
    }
};

void Branches() {
    Runtime found;
    Check(pal4::ScriptGobReset::ByName("gob", found) == 0xabcd1234u &&
              found.calls == std::vector<std::string>{
                  "manager", "find", "reset"},
          "found GOB calls virtual reset and forwards result");
    Runtime missing;
    missing.object = 0u;
    Check(pal4::ScriptGobReset::ByName("gob", missing) == 0u &&
              missing.calls == std::vector<std::string>{"manager", "find"},
          "missing GOB skips virtual reset");
    Runtime null_name;
    null_name.expected_name = "";
    null_name.object = 0u;
    Check(pal4::ScriptGobReset::ByName(nullptr, null_name) == 0u &&
              null_name.calls == std::vector<std::string>{
                  "manager", "find"},
          "null script string resolves to empty name");
}

}  // namespace

int main() {
    try {
        Branches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
