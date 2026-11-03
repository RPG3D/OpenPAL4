#include "pal4/game_data_shop_goods_index.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
    try {
        const pal4::GameDataShopGoodsIndex index(
            0x8000, {{1, 2}, {1, 4}, {0x340bc6d9u, 3},
                     {0x80000000u, 5}, {0xffffffffu, 1}});
        Check(index.FindByHash(1) == 0x8000 + 2 * 56,
              "duplicate key returns the first indexed record");
        Check(index.FindByHash(0x340bc6d9u) == 0x8000 + 3 * 56 &&
                  index.FindByHash(0x80000000u) == 0x8000 + 5 * 56 &&
                  index.FindByHash(0xffffffffu) == 0x8000 + 56,
              "lookup uses unsigned key ordering and 56-byte stride");
        Check(index.FindByHash(0) == 0 &&
                  index.FindByHash(2) == 0 &&
                  index.FindByHash(0xfffffffeu) == 0,
              "lookup rejects keys between indexed hashes");
        const pal4::GameDataShopGoodsIndex empty(0x8000, {});
        Check(empty.FindByHash(1) == 0,
              "empty index returns null");

        pal4::LegacyCrc32 crc;
        constexpr std::array<std::uint8_t, 9> digits{
            '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        Check(index.FindByText(digits.data(), digits.size(), crc) ==
                  0x8000 + 3 * 56 &&
                  crc.LastResult() == 0x340bc6d9u,
              "text lookup computes the same CRC and returns its record");
        Check(index.FindByText(nullptr, 0, crc) == 0x8000 + 56 &&
                  crc.LastResult() == 0xffffffffu,
              "empty text uses the empty-buffer CRC seed");
        Check(index.FindByText(digits.data(), 0, crc) == 0x8000 + 56,
              "zero-length text hashes without reading bytes");
        std::cout << "game_data_shop_goods_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
