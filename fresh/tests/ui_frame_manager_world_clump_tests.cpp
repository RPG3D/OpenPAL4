#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerWorldClumpRuntime {
    std::int32_t result = 0;
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x519cu, "manager collection field");
        calls.emplace_back("read collection");
        return 0x6000u;
    }
    std::int32_t AddClump(std::uint32_t collection,
                          std::uint32_t clump) override {
        Check(collection == 0x6018u && clump == 0xabcdef01u,
              "embedded collection and full clump argument");
        calls.emplace_back("add clump");
        return result;
    }
};

}  // namespace

int main() {
    try {
        for (const auto result : {-1, 0, 1, 42}) {
            Runtime runtime;
            runtime.result = result;
            Check(pal4::UiFrameManagerSingleton::AddWorldClump(
                      0x5000u, 0xabcdef01u, runtime) == (result > 0),
                  "signed greater-than-zero return");
            Check(runtime.calls == (std::vector<std::string>{
                      "read collection", "add clump"}),
                  "collection read precedes add");
        }
        std::cout << "ui_frame_manager_world_clump_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
