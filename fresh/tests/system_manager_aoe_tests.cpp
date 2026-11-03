#include "pal4/system_manager_aoe.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

bool Near(const float value, const float expected) {
    return std::fabs(value - expected) < 0.002f;
}

struct Runtime final : pal4::ISystemManagerAoERuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> rates;
    std::map<std::uint32_t, std::uint32_t> groups;
    std::map<std::uint32_t, float> properties;
    std::vector<std::uint32_t> randoms;
    std::vector<std::pair<std::uint32_t, float>> property_sets;
    std::vector<std::uint32_t> released_order;
    std::vector<std::string> calls;
    float first = 0.2f;
    float second = 0.8f;
    std::uint32_t random_index = 0;
    std::uint32_t finalize_count = 0;
    std::uint32_t group_queries = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    float ObjectRate(const std::uint32_t object) override {
        calls.emplace_back("rate");
        return rates[object];
    }
    void SetObjectProperty(const std::uint32_t object,
                           const float value) override {
        calls.emplace_back("set");
        property_sets.emplace_back(object, value);
        properties[object] = value;
    }
    float GameDbFloat(const std::uint32_t index) override {
        Check(index == 24u || index == 25u, "GameDB float index");
        calls.emplace_back(index == 24u ? "db24" : "db25");
        return index == 24u ? first : second;
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        Check(random_index < randoms.size(), "random draw count");
        return randoms[random_index++];
    }
    std::uint32_t ObjectGroup(const std::uint32_t object) override {
        calls.emplace_back("group");
        ++group_queries;
        return groups[object];
    }
    void FinalizeManager(const std::uint32_t owner) override {
        Check(owner == 0x1000u, "finalize owner");
        calls.emplace_back("finalize");
        ++finalize_count;
    }
    std::uint32_t ReleaseOrdered(
        const std::vector<std::uint32_t>& objects) override {
        calls.emplace_back("release");
        released_order = objects;
        return 0xabcdu;
    }
    std::uint32_t InvokeManagerSlot4(
        const std::uint32_t owner) override {
        Check(owner == 0x1000u, "slot-four owner");
        calls.emplace_back("virtual-four");
        return 0x9876u;
    }
};

constexpr auto manager = 0x1000u;
constexpr auto group_argument = 0x9000u;

void WeightedDistributionAndGroupOverride() {
    Runtime runtime;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x200cu;
    runtime.words[0x2000u] = 0x5000u;
    runtime.words[0x2004u] = 0x5010u;
    runtime.words[0x2008u] = 0x5020u;
    runtime.words[group_argument] = 1u;
    runtime.rates[0x5000u] = 1.0f;
    runtime.rates[0x5010u] = 3.0f;
    runtime.rates[0x5020u] = 2.0f;
    runtime.groups[0x5000u] = 1u;
    runtime.randoms = {2500u, 1000u, 1500u, 1999u};
    Check(pal4::SystemManagerAoE::Distribute(
              manager, group_argument, runtime) == 0xabcdu &&
          runtime.released_order ==
              std::vector<std::uint32_t>{0x5010u, 0x5020u, 0x5000u} &&
          runtime.random_index == 4 && runtime.finalize_count == 1,
          "descending rate order and modulo-1000 random count");
    Check(runtime.property_sets.size() == 4 &&
          runtime.property_sets[0].first == 0x5010u &&
          Near(runtime.property_sets[0].second, 16.0f) &&
          runtime.property_sets[1].first == 0x5020u &&
          Near(runtime.property_sets[1].second, 10.6667f) &&
          runtime.property_sets[2].first == 0x5000u &&
          Near(runtime.property_sets[2].second, 10.6613f) &&
          runtime.property_sets[3] ==
              std::pair<std::uint32_t, float>{0x5000u, 64.0f},
          "weighted ranges and matching group override");
    Check(runtime.group_queries == 3 &&
          runtime.calls[runtime.calls.size() - 2] == "finalize" &&
          runtime.calls.back() == "release",
          "group sweep precedes manager finalize and buffer release");
}

void ReversedBoundsAndEqualRates() {
    Runtime runtime;
    runtime.first = 0.9f;
    runtime.second = 0.1f;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x2008u;
    runtime.words[0x2000u] = 0x5000u;
    runtime.words[0x2004u] = 0x5010u;
    runtime.rates[0x5000u] = 1.0f;
    runtime.rates[0x5010u] = 1.0f;
    runtime.words[group_argument] = 0u;
    runtime.randoms = {0u, 500u, 500u};
    (void)pal4::SystemManagerAoE::Distribute(
        manager, group_argument, runtime);
    Check(runtime.released_order ==
              std::vector<std::uint32_t>{0x5000u, 0x5010u} &&
          runtime.property_sets.size() == 2 &&
          Near(runtime.property_sets[0].second, 4.8f) &&
          Near(runtime.property_sets[1].second, 3.2f) &&
          runtime.group_queries == 0,
          "reversed bounds are swapped and equal-rate order is stable");
}

void EmptyAndSecondGroup() {
    Runtime empty;
    empty.words[group_argument] = 2;
    empty.randoms = {7u};
    Check(pal4::SystemManagerAoE::Distribute(
              manager, group_argument, empty) == 0xabcdu &&
          empty.random_index == 1 && empty.released_order.empty() &&
          empty.property_sets.empty() && empty.finalize_count == 1,
          "empty list still draws base number and finalizes");

    Runtime grouped;
    grouped.first = 0.9f;
    grouped.second = 0.1f;
    grouped.words[manager + 112u] = 0x2000u;
    grouped.words[manager + 116u] = 0x2004u;
    grouped.words[0x2000u] = 0x5000u;
    grouped.words[group_argument] = 2u;
    grouped.rates[0x5000u] = 4.0f;
    grouped.groups[0x5000u] = 2u;
    grouped.randoms = {0u, 500u};
    (void)pal4::SystemManagerAoE::Distribute(
        manager, group_argument, grouped);
    Check(grouped.property_sets.size() == 2 &&
          Near(grouped.property_sets.front().second, 3.2f) &&
          grouped.properties[0x5000u] == 64.0f,
          "second group forces matching object to threshold");
    Check(pal4::SystemManagerAoE::InvokeManager(manager, grouped) ==
              0x9876u && grouped.calls.back() == "virtual-four",
          "adjacent callback forwards vtable slot four");
}

}  // namespace

int main() {
    try {
        WeightedDistributionAndGroupOverride();
        ReversedBoundsAndEqualRates();
        EmptyAndSecondGroup();
        std::cout << "system_manager_aoe_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
