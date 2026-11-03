#include "pal4/game_string_reference.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameStringReferenceRuntime {
    std::uint32_t database = 0x7000;
    std::uint32_t result = 0x9000;
    std::vector<int> calls;
    std::uint32_t collection = 0;
    std::uint32_t key = 0;

    std::uint32_t GameDbInstance() override {
        calls.push_back(1);
        return database;
    }

    std::uint32_t FindGameString(std::uint32_t collection_value,
                           std::uint32_t key_value) override {
        calls.push_back(2);
        collection = collection_value;
        key = key_value;
        return result;
    }
};

void CheckLookup(std::uint32_t input, std::uint32_t returned) {
    Runtime runtime;
    runtime.result = returned;
    Check(pal4::GameStringReference(input).FindInGameDB(runtime) == returned &&
              runtime.collection == 0x7034 && runtime.key == input &&
              runtime.calls == std::vector<int>{1, 2},
          "lookup forwards GameDB+52 and the reference key in order");
}

}  // namespace

int main() {
    try {
        CheckLookup(0x12345678, 0x9000);
        CheckLookup(0, 0);
        std::cout << "game_string_reference_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
