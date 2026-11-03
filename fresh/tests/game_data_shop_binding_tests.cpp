#include "pal4/game_data_shop_binding.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string operation;
    std::uint32_t manager;
    std::string name;
    std::uint32_t callback = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataShopBindingRuntime {
    std::vector<Call> calls;
    std::map<std::string, std::uint32_t> addresses{
        {"ShopProperty", 0x8100}, {"Good", 0x8200},
        {"ShopType", 0x8300}, {"GoodsType", 0x8400}};
    std::vector<std::string> errors;
    std::uint8_t second_registration_result = 1;

    std::uint8_t RegisterHandler(std::uint32_t manager,
                                 std::string_view name,
                                 std::uint32_t callback) override {
        calls.push_back({"register", manager, std::string(name), callback});
        return name == "Good" ? second_registration_result : 0;
    }
    std::uint32_t FindCollection(std::uint32_t manager,
                                 std::string_view name) override {
        calls.push_back({"collection", manager, std::string(name)});
        return addresses[std::string(name)];
    }
    std::uint32_t FindStringProperty(std::uint32_t manager,
                                     std::string_view name) override {
        calls.push_back({"property", manager, std::string(name)});
        return addresses[std::string(name)];
    }
    void LogError(std::string_view message) override {
        errors.emplace_back(message);
    }
};

void CheckHandlers() {
    Runtime runtime;
    runtime.second_registration_result = 7;
    Check(pal4::GameDataShopBinding::RegisterHandlers(
              0x9000, runtime) == 7 &&
              runtime.calls == std::vector<Call>{
                  {"register", 0x9000, "ShopProperty", 0x4ef1a0},
                  {"register", 0x9000, "Good", 0x4efd30}},
          "handler registration preserves names, callbacks, order and return");
}

void CheckResolution() {
    Runtime runtime;
    pal4::GameDataShopBindingState state{.manager = 0x9000};
    Check(pal4::GameDataShopBinding::ResolveTables(state, runtime) == 1 &&
              state.shop_property == 0x8100 && state.good == 0x8200 &&
              state.shop_type == 0x8300 && state.goods_type == 0x8400 &&
              runtime.errors.empty(),
          "all required shop table handles are retained");
    Check(runtime.calls == std::vector<Call>{
              {"collection", 0x9000, "ShopProperty"},
              {"collection", 0x9000, "Good"},
              {"property", 0x9000, "ShopType"},
              {"property", 0x9000, "GoodsType"}},
          "shop resolution chooses the correct lookup service and order");

    constexpr std::array<std::string_view, 4> names{
        "ShopProperty", "Good", "ShopType", "GoodsType"};
    for (std::size_t failure = 0; failure < names.size(); ++failure) {
        Runtime interrupted;
        interrupted.addresses[std::string(names[failure])] = 0;
        pal4::GameDataShopBindingState partial{
            0x9000, 0xdead0001, 0xdead0002, 0xdead0003, 0xdead0004};
        const auto expected =
            std::string("ERROR [ palDBShopManager::verifyDataTable < the table ") +
            std::string(names[failure]) + "  doesn't exist>]";
        try {
            static_cast<void>(pal4::GameDataShopBinding::ResolveTables(
                partial, interrupted));
            Check(false, "missing table must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == expected &&
                      interrupted.errors == std::vector<std::string>{expected} &&
                      interrupted.calls.size() == failure + 1,
                  "missing table logs the native message and stops lookup");
            const std::array<std::uint32_t, 4> fields{
                partial.shop_property, partial.good,
                partial.shop_type, partial.goods_type};
            const std::array<std::uint32_t, 4> found{
                0x8100, 0x8200, 0x8300, 0x8400};
            for (std::size_t i = 0; i < fields.size(); ++i)
                Check(fields[i] == (i < failure ? found[i] :
                                    i == failure ? 0u :
                                    0xdead0001u + static_cast<std::uint32_t>(i)),
                      "missing table preserves earlier and later field state");
        }
    }
}

}  // namespace

int main() {
    try {
        CheckHandlers();
        CheckResolution();
        std::cout << "game_data_shop_binding_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
