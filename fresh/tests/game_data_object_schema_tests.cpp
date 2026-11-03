#include "pal4/game_data_object_schema.h"
#include "pal4/game_data_shop_goods_schema.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::uint32_t binding;
    std::string name;
    std::uint32_t offset;
    pal4::GameDataFieldKind kind;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataFieldSchemaRuntime {
    std::vector<Call> calls;
    std::size_t fail_on_call = 0;

    void RegisterField(std::uint32_t binding, std::string_view name,
                       std::uint32_t offset,
                       pal4::GameDataFieldKind kind) override {
        calls.push_back({binding, std::string(name), offset, kind});
        if (calls.size() == fail_on_call)
            throw std::runtime_error("registration failed");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::GameDataObjectSchema::RegisterFields(
                  0x9000, runtime) == 1,
              "object field registration returns success");
        Check(runtime.calls == std::vector<Call>{
                  {0x9000, "ID", 4, pal4::GameDataFieldKind::text},
                  {0x9000, "NAME", 20, pal4::GameDataFieldKind::text},
                  {0x9000, "POSITION", 36,
                   pal4::GameDataFieldKind::text},
                  {0x9000, "TYPE", 52,
                   pal4::GameDataFieldKind::integer}},
              "object fields preserve IDA names, offsets, types and order");
        for (std::size_t failure = 1; failure <= 4; ++failure) {
            Runtime interrupted;
            interrupted.fail_on_call = failure;
            try {
                static_cast<void>(
                    pal4::GameDataObjectSchema::RegisterFields(
                        0x9000, interrupted));
                Check(false, "registration failure must propagate");
            } catch (const std::runtime_error& error) {
                Check(std::string(error.what()) ==
                          "registration failed" &&
                          interrupted.calls.size() == failure,
                      "registration stops at the failing field");
            }
        }
        Runtime shop;
        Check(pal4::GameDataShopGoodsSchema::RegisterFields(
                  0xa000, shop) == 1,
              "shop goods field registration returns success");
        Check(shop.calls == std::vector<Call>{
                  {0xa000, "ID", 0, pal4::GameDataFieldKind::integer},
                  {0xa000, "SHOP_ID", 20, pal4::GameDataFieldKind::text},
                  {0xa000, "GOODS_ID", 8,
                   pal4::GameDataFieldKind::integer},
                  {0xa000, "GOODS_TYPE", 4,
                   pal4::GameDataFieldKind::integer},
                  {0xa000, "OPEN_CONDITION", 12,
                   pal4::GameDataFieldKind::integer},
                  {0xa000, "OPEN_NUM", 16,
                   pal4::GameDataFieldKind::integer}},
              "shop goods fields preserve native order, offsets and types");
        for (std::size_t failure = 1; failure <= 6; ++failure) {
            Runtime interrupted;
            interrupted.fail_on_call = failure;
            try {
                static_cast<void>(
                    pal4::GameDataShopGoodsSchema::RegisterFields(
                        0xa000, interrupted));
                Check(false, "shop field failure must propagate");
            } catch (const std::runtime_error& error) {
                Check(std::string(error.what()) ==
                          "registration failed" &&
                          interrupted.calls.size() == failure,
                      "shop registration stops at the failing field");
            }
        }
        std::cout << "game_data_object_schema_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
