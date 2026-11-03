#include "pal4/rw_texture_read_hook.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IRwTextureReadHookRuntime {
public:
    std::vector<char> calls;
    std::uint32_t current = 0x1234u;
    std::uint32_t set_result = 0x5678u;
    std::uint32_t dds_result = 0;
    std::uint32_t original_result = 0xabcd;
    std::uint32_t installed = 0;
    std::uint32_t previous = 0;
    std::uint32_t last_context = 0;
    std::string dds_name;
    std::string original_name;

    std::uint32_t GetReadCallback() override {
        calls.push_back('G');
        return current;
    }
    std::uint32_t SetReadCallback(std::uint32_t callback) override {
        calls.push_back('S');
        installed = callback;
        return set_result;
    }
    std::uint32_t ReadDds(const char* name,
                          std::uint32_t context) override {
        calls.push_back('D');
        dds_name = name;
        last_context = context;
        return dds_result;
    }
    std::uint32_t ReadOriginal(std::uint32_t callback,
                               const char* name,
                               std::uint32_t context) override {
        calls.push_back('O');
        previous = callback;
        original_name = name;
        last_context = context;
        return original_result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::RwTextureReadHookState state{};
    Check(pal4::RwTextureReadHook::Install(state, runtime) == 0x5678u &&
              state.previous_callback == 0x1234u &&
              runtime.installed == pal4::RwTextureReadHook::kCustomCallbackAddress &&
              runtime.calls == std::vector<char>{'G', 'S'},
          "install saves previous callback before setting custom callback");

    runtime.calls.clear();
    runtime.dds_result = 0x9000u;
    Check(pal4::RwTextureReadHook::Read(
              state, "textures/stone.dds", 0x42u, runtime) == 0x9000u &&
              runtime.dds_name == "textures/stone" &&
              runtime.last_context == 0x42u &&
              runtime.calls == std::vector<char>{'D'},
          "DDS hit strips final extension and skips original callback");

    runtime.calls.clear();
    runtime.dds_result = 0;
    Check(pal4::RwTextureReadHook::Read(
              state, "ab.b.c", 0x55u, runtime) == 0xabcdu &&
              runtime.dds_name == "ab.b" &&
              runtime.previous == 0x1234u &&
              runtime.original_name == "ab.b.c" &&
              runtime.last_context == 0x55u &&
              runtime.calls == std::vector<char>{'D', 'O'},
          "DDS miss invokes saved callback with original arguments");

    runtime.calls.clear();
    state.previous_callback = 0;
    Check(pal4::RwTextureReadHook::Read(
              state, "stone", 0x31u, runtime) == 0 &&
              runtime.dds_name == "stone" &&
              runtime.calls == std::vector<char>{'D'},
          "missing original callback leaves DDS miss unchanged");

    runtime.calls.clear();
    Check(pal4::RwTextureReadHook::Read(
              state, ".dds", 1u, runtime) == 0 &&
              runtime.dds_name.empty(),
          "short dotted input passes an empty base to DDS loader");
    runtime.calls.clear();
    const std::string long_name(300, 'x');
    Check(pal4::RwTextureReadHook::Read(
              state, long_name.c_str(), 1u, runtime) == 0 &&
              runtime.dds_name == std::string(255, 'x'),
          "long extensionless input is bounded to the stack buffer");
    runtime.calls.clear();
    Check(pal4::RwTextureReadHook::Read(
              state, nullptr, 1u, runtime) == 0 &&
              runtime.calls.empty(),
          "null name is rejected before native string operations");

    std::cout << "rw_texture_read_hook_tests passed\n";
}
