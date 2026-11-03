#include "pal4/script_inventory_query.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptInventoryQueryRuntime {
    std::uint32_t element = 0x5000u;
    std::int32_t quantity = 3;
    std::int32_t money = 0;
    std::vector<std::string> calls;

    std::uint32_t Inventory() override {
        calls.emplace_back("inventory");
        return 0x4000u;
    }
    std::uint32_t FindElement(std::uint32_t inventory,
                              std::int32_t id,
                              std::uint32_t category) override {
        Check(inventory == 0x4000u && id == -7 && category == 5u,
              "inventory lookup operands");
        calls.emplace_back("find");
        return element;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x4134u) {
            calls.emplace_back("money");
            return static_cast<std::uint32_t>(money);
        }
        Check(address == element + 8u, "inventory quantity offset");
        calls.emplace_back("quantity");
        return static_cast<std::uint32_t>(quantity);
    }
};

void CheckQuantityBranches() {
    for (const auto [quantity, required] : {
             std::pair{3, 2}, std::pair{3, 3}, std::pair{3, 4},
             std::pair{-1, -2}, std::pair{-1, 0}}) {
        Runtime runtime;
        runtime.quantity = quantity;
        Check(pal4::ScriptInventoryQuery::HasAtLeast(
                  -7, required, runtime) == (quantity >= required) &&
                  runtime.calls == std::vector<std::string>{
                      "inventory", "find", "quantity"},
              "signed inventory quantity comparison");
    }
    Runtime missing;
    missing.element = 0u;
    Check(!pal4::ScriptInventoryQuery::HasAtLeast(-7, -1, missing) &&
              missing.calls == std::vector<std::string>{"inventory", "find"},
          "missing item returns false without reading quantity");
    for (auto quantity : {0, 3, -1}) {
        Runtime count;
        count.quantity = quantity;
        Check(pal4::ScriptInventoryQuery::Count(-7, count) == quantity &&
                  count.calls == std::vector<std::string>{
                      "inventory", "find", "quantity"},
              "item-count callback forwards stored signed word");
    }
    Runtime absent;
    absent.element = 0u;
    Check(pal4::ScriptInventoryQuery::Count(-7, absent) == 0 &&
              absent.calls == std::vector<std::string>{
                  "inventory", "find"},
          "missing item count is zero");
    for (auto amount : {0, 42, -1}) {
        Runtime money;
        money.money = amount;
        Check(pal4::ScriptInventoryQuery::Money(money) == amount &&
                  money.calls == std::vector<std::string>{
                      "inventory", "money"},
              "money query reads signed inventory counter at +0x134");
    }
}

}  // namespace

int main() {
    try {
        CheckQuantityBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
