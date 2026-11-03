#include "pal4/shop_ui_click_callback.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiClickCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
    }
};

void TestConstructor() {
    Runtime runtime;
    const auto result = pal4::ShopUiClickCallback::Construct(
        0x5000u, 0x4864f0u, 0x11223344u, 0x6000u, runtime);
    Check(result == 0x5000u &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x5008u, 0x4864f0u},
                      {0x500cu, 0x11223344u},
                      {0x5010u, 0x6000u},
                      {0x5000u, 0x843d1cu}}),
          "callback fields and vtable are written in IDA order");
    Check(runtime.words.size() == 4u,
          "constructor leaves other callback words untouched");
}

}  // namespace

int main() {
    TestConstructor();
    return EXIT_SUCCESS;
}
