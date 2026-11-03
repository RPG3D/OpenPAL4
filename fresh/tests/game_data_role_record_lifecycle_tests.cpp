#include "pal4/game_data_role_record_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataRoleRecordTextRuntime {
    std::vector<std::uint32_t> destroyed;
    void DestroyText(std::uint32_t address) override {
        destroyed.push_back(address);
    }
};

void CheckDestroyOrder() {
    Runtime runtime;
    pal4::GameDataRoleRecordLifecycle::Destroy(0x2000, runtime);
    Check(runtime.destroyed == std::vector<std::uint32_t>{
              0x2130, 0x2120, 0x2110, 0x2100, 0x20f0,
              0x20c0, 0x20b0, 0x20a0, 0x2090},
          "Role destructor visits five tail and four middle text objects in reverse order");
}

}  // namespace

int main() {
    try {
        CheckDestroyOrder();
        std::cout << "game_data_role_record_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
