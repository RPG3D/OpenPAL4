#include "pal4/item_use_effects.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Before(const std::vector<std::string>& calls,
            const std::string& first, const std::string& second) {
    auto first_index = calls.size();
    auto second_index = calls.size();
    for (std::size_t i = 0u; i < calls.size(); ++i) {
        if (calls[i] == first && first_index == calls.size())
            first_index = i;
        if (calls[i] == second && second_index == calls.size())
            second_index = i;
    }
    return first_index < second_index;
}

struct Runtime final : pal4::IItemUseEffectsRuntime {
    static constexpr std::uint32_t recipe_data = 0x2000u;
    static constexpr std::uint32_t player = 0x3000u;
    static constexpr std::uint32_t object = 0x4000u;

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::set<std::uint32_t> direct_items;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> removed;
    std::vector<std::string> calls;
    std::int32_t random_value = 0;
    int fees = 0;
    int float_conversions = 0;

    Runtime() {
        for (std::uint32_t i = 0u; i < 4u; ++i) SetMaterial(i, 0u, 0);
        words[player + 0x184u] = 5u;
        words[recipe_data + 0x6cu] = 4u;
        words[recipe_data + 0x144u] = 0u;
        bytes[object] = 1u;
    }
    void SetMaterial(std::uint32_t index, std::uint32_t item,
                     std::int32_t quantity) {
        words[recipe_data + 0x98u + index * 4u] = item;
        words[recipe_data + 0xa8u + index * 4u] =
            static_cast<std::uint32_t>(quantity);
    }
    void SetFloat(std::uint32_t address, float value) {
        words[address] = std::bit_cast<std::uint32_t>(value);
    }
    float GetFloat(std::uint32_t address) const {
        return std::bit_cast<float>(words.at(address));
    }
    std::uint32_t GameDb() override {
        calls.push_back("db");
        return 0x1000u;
    }
    std::uint32_t FindRecipe(std::uint32_t collection,
                             std::uint32_t recipe) override {
        Check(collection == 0x1080u && recipe == 88u,
              "recipe lookup arguments");
        calls.push_back("recipe");
        return recipe_data;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == player + 0x184u) calls.push_back("write count");
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == object, "item-use object flag");
        calls.push_back("flag");
        return bytes.at(address);
    }
    bool IsDirectItem(std::uint32_t db,
                       std::uint32_t item) override {
        Check(db == 0x1000u, "direct item database");
        calls.push_back("direct? " + std::to_string(item));
        return direct_items.contains(item);
    }
    void FindOrLogDirectItem(std::uint32_t db,
                             std::uint32_t item) override {
        Check(db == 0x1000u && direct_items.contains(item),
              "direct item log arguments");
        calls.push_back("log " + std::to_string(item));
    }
    std::uint32_t Inventory() override {
        calls.push_back("inventory");
        return 0x5000u;
    }
    void RemoveDirectItem(std::uint32_t inventory,
                          std::uint32_t item,
                          std::uint32_t quantity) override {
        Check(inventory == 0x5000u, "direct item inventory");
        calls.push_back("remove " + std::to_string(item));
        removed.emplace_back(item, quantity);
    }
    std::uint32_t FindInventoryEntry(std::uint32_t collection,
                                     std::uint32_t key,
                                     std::uint32_t category) override {
        Check(collection == 0x507cu && key == 99u && category == 7u,
              "target item lookup arguments");
        calls.push_back("target");
        return player;
    }
    std::uint32_t CurrentTime() override {
        calls.push_back("time");
        return 123456u;
    }
    void SetRandomSeed(std::uint32_t seed) override {
        Check(seed == 123456u, "random seed value");
        calls.push_back("seed");
    }
    std::int32_t Random() override {
        calls.push_back("random");
        return random_value;
    }
    std::int32_t FloatToInt(double value) override {
        ++float_conversions;
        return static_cast<std::int32_t>(value);
    }
    void PlayMusic(std::string_view name, std::uint32_t mode) override {
        Check(mode == 1u, "music mode");
        calls.push_back(std::string(name));
    }
    void UpdateAndClampCounter(std::uint32_t inventory,
                               std::int32_t delta) override {
        Check(inventory == 0x5000u && delta == -80,
              "item-use fee arguments");
        calls.push_back("charge");
        ++fees;
    }
};

void CheckMaterialGate() {
    Runtime runtime;
    runtime.SetMaterial(0, 30u, 4);
    runtime.SetMaterial(1, 10u, 2);
    runtime.SetMaterial(2, 30u, 9);
    runtime.SetMaterial(3, 20u, 3);
    runtime.direct_items = {10u, 30u};
    Check(pal4::ItemUseEffects::Apply(
              runtime.object, 88u, 99u, runtime) == -2 &&
              runtime.removed ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {10u, 2u}, {30u, 4u}}) &&
              runtime.words.at(runtime.player + 0x184u) == 5u &&
              runtime.fees == 0 &&
              runtime.calls.back() == "remove 30",
          "unresolved ingredient returns minus two after sorted direct removals");
}

void CheckChanceAndFailure() {
    for (const auto [difference, chance] :
         std::vector<std::pair<std::int32_t, std::int32_t>>{
             {-1, 10}, {-2, 9}, {-3, 8}, {-4, 7}, {-5, 6},
             {-100, 6}}) {
        Runtime runtime;
        runtime.words[runtime.player + 0x184u] = 0u;
        runtime.words[runtime.recipe_data + 0x6cu] =
            static_cast<std::uint32_t>(-difference);
        runtime.random_value = chance;
        runtime.bytes[runtime.object] = 0u;
        Check(pal4::ItemUseEffects::Apply(
                  runtime.object, 88u, 99u, runtime) == -1 &&
                  runtime.words.at(runtime.player + 0x184u) ==
                      static_cast<std::uint32_t>(difference) &&
                  runtime.calls.at(runtime.calls.size() - 4u) ==
                      "wa09a.wav" &&
                  runtime.calls.back() == "charge" &&
                  runtime.fees == 1 && runtime.float_conversions == 0,
              "chance threshold failure writes count and charges once");
        Check(Before(runtime.calls, "time", "seed") &&
                  Before(runtime.calls, "seed", "random") &&
                  Before(runtime.calls, "random", "write count") &&
                  Before(runtime.calls, "write count", "wa09a.wav"),
              "time seeding and count write precede failure sound");
    }
    Runtime overflow;
    overflow.words[overflow.player + 0x184u] = 0x7fffffffu;
    overflow.words[overflow.recipe_data + 0x6cu] = 0xffffffffu;
    overflow.random_value = 6;
    Check(pal4::ItemUseEffects::Apply(
              overflow.object, 88u, 99u, overflow) == -1 &&
              overflow.words.at(overflow.player + 0x184u) ==
                  0x80000000u,
          "chance selection uses subtraction sign bit on overflow");
}

void SetStatFields(Runtime& runtime, std::uint32_t modifier,
                   float float_modifier) {
    for (std::uint32_t i = 0u; i < 8u; ++i) {
        runtime.words[runtime.player + 4u + i * 4u] = 100u;
        runtime.words[runtime.recipe_data + 0xb8u + i * 4u] = modifier;
    }
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        runtime.words[runtime.player + 0x7cu + i * 4u] = 100u;
        runtime.words[runtime.recipe_data + 0xd8u + i * 4u] = modifier;
    }
    for (std::uint32_t i = 0u; i < 6u; ++i) {
        runtime.words[runtime.player + 0x34u + i * 4u] = 200u;
        runtime.words[runtime.recipe_data + 0xecu + i * 4u] = modifier;
        runtime.SetFloat(runtime.player + 0x64u + i * 4u, 2.0f);
        runtime.SetFloat(runtime.recipe_data + 0x104u + i * 4u,
                         float_modifier);
        runtime.SetFloat(runtime.player + 0x4cu + i * 4u, 4.0f);
        runtime.SetFloat(runtime.recipe_data + 0x11cu + i * 4u,
                         float_modifier);
    }
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        runtime.SetFloat(runtime.player + 0x24u + i * 4u, 6.0f);
        runtime.SetFloat(runtime.recipe_data + 0x134u + i * 4u,
                         float_modifier);
    }
}

void CheckFlatSuccess() {
    Runtime runtime;
    SetStatFields(runtime, 5u, 0.5f);
    runtime.random_value = 99;
    Check(pal4::ItemUseEffects::Apply(
              runtime.object, 88u, 99u, runtime) == 0 &&
              runtime.words.at(runtime.player + 4u) == 105u &&
              runtime.words.at(runtime.player + 0x7cu) == 105u &&
              runtime.words.at(runtime.player + 0x34u) == 205u &&
              runtime.GetFloat(runtime.player + 0x64u) == 2.5f &&
              runtime.GetFloat(runtime.player + 0x4cu) == 4.5f &&
              runtime.GetFloat(runtime.player + 0x24u) == 6.5f &&
              runtime.calls.back() == "flag" &&
              runtime.float_conversions == 0 && runtime.fees == 0,
          "flat success updates all integer and float stat groups");
    for (std::uint32_t i = 0u; i < 8u; ++i)
        Check(runtime.words.at(runtime.player + 4u + i * 4u) == 105u,
              "flat base stat index");
    for (std::uint32_t i = 0u; i < 5u; ++i)
        Check(runtime.words.at(runtime.player + 0x7cu + i * 4u) == 105u,
              "flat current stat index");
    for (std::uint32_t i = 0u; i < 6u; ++i) {
        Check(runtime.words.at(runtime.player + 0x34u + i * 4u) == 205u,
              "flat integer resistance index");
        Check(runtime.GetFloat(runtime.player + 0x64u + i * 4u) == 2.5f &&
                  runtime.GetFloat(runtime.player + 0x4cu + i * 4u) == 4.5f,
              "flat skill and float resistance index");
    }
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(runtime.GetFloat(runtime.player + 0x24u + i * 4u) == 6.5f,
              "flat attribute index");
}

void CheckPercentageSuccess() {
    Runtime runtime;
    SetStatFields(runtime, 50u, 50.0f);
    runtime.words[runtime.recipe_data + 0x144u] = 1u;
    runtime.bytes[runtime.object] = 0u;
    runtime.random_value = 0;
    Check(pal4::ItemUseEffects::Apply(
              runtime.object, 88u, 99u, runtime) == 0 &&
              runtime.words.at(runtime.player + 4u) == 150u &&
              runtime.words.at(runtime.player + 0x7cu) == 150u &&
              runtime.words.at(runtime.player + 0x8cu) == 150u &&
              runtime.words.at(runtime.player + 0x34u) == 300u &&
              runtime.words.at(runtime.player + 0x48u) == 300u &&
              runtime.GetFloat(runtime.player + 0x64u) == 3.0f &&
              runtime.GetFloat(runtime.player + 0x78u) == 3.0f &&
              runtime.GetFloat(runtime.player + 0x4cu) == 6.0f &&
              runtime.GetFloat(runtime.player + 0x60u) == 6.0f &&
              runtime.GetFloat(runtime.player + 0x24u) == 9.0f &&
              runtime.GetFloat(runtime.player + 0x30u) == 9.0f &&
              runtime.float_conversions == 19 && runtime.fees == 1 &&
              runtime.calls.back() == "charge",
          "percentage success scales all indexed integer and float groups");
    for (std::uint32_t i = 0u; i < 8u; ++i)
        Check(runtime.words.at(runtime.player + 4u + i * 4u) == 150u,
              "percentage base stat index");
    for (std::uint32_t i = 0u; i < 5u; ++i)
        Check(runtime.words.at(runtime.player + 0x7cu + i * 4u) == 150u,
              "percentage current stat index");
    for (std::uint32_t i = 0u; i < 6u; ++i) {
        Check(runtime.words.at(runtime.player + 0x34u + i * 4u) == 300u,
              "percentage integer resistance index");
        Check(runtime.GetFloat(runtime.player + 0x64u + i * 4u) == 3.0f &&
                  runtime.GetFloat(runtime.player + 0x4cu + i * 4u) == 6.0f,
              "percentage skill and float resistance index");
    }
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(runtime.GetFloat(runtime.player + 0x24u + i * 4u) == 9.0f,
              "percentage attribute index");
}

void CheckDistinctOffsets() {
    Runtime flat;
    SetStatFields(flat, 0u, 0.0f);
    flat.words[flat.recipe_data + 0xd4u] = 13u;
    flat.words[flat.recipe_data + 0xe4u] = 7u;
    flat.words[flat.recipe_data + 0xfcu] = 9u;
    flat.SetFloat(flat.recipe_data + 0x118u, 1.25f);
    flat.SetFloat(flat.recipe_data + 0x124u, 0.75f);
    flat.SetFloat(flat.recipe_data + 0x140u, 1.5f);
    Check(pal4::ItemUseEffects::Apply(
              flat.object, 88u, 99u, flat) == 0 &&
              flat.words.at(flat.player + 0x20u) == 113u &&
              flat.words.at(flat.player + 0x88u) == 107u &&
              flat.words.at(flat.player + 0x44u) == 209u &&
              flat.GetFloat(flat.player + 0x78u) == 3.25f &&
              flat.GetFloat(flat.player + 0x54u) == 4.75f &&
              flat.GetFloat(flat.player + 0x30u) == 7.5f,
          "flat mode reads distinct late integer and float recipe offsets");

    Runtime percent;
    SetStatFields(percent, 0u, 0.0f);
    percent.words[percent.recipe_data + 0x144u] = 1u;
    percent.words[percent.recipe_data + 0xd4u] = 25u;
    percent.words[percent.recipe_data + 0xe8u] = 10u;
    percent.words[percent.recipe_data + 0x100u] = 10u;
    percent.SetFloat(percent.recipe_data + 0x118u, 25.0f);
    percent.SetFloat(percent.recipe_data + 0x130u, 25.0f);
    percent.SetFloat(percent.recipe_data + 0x140u, 25.0f);
    Check(pal4::ItemUseEffects::Apply(
              percent.object, 88u, 99u, percent) == 0 &&
              percent.words.at(percent.player + 0x20u) == 125u &&
              percent.words.at(percent.player + 0x8cu) == 110u &&
              percent.words.at(percent.player + 0x48u) == 220u &&
              percent.GetFloat(percent.player + 0x78u) == 2.5f &&
              percent.GetFloat(percent.player + 0x60u) == 5.0f &&
              percent.GetFloat(percent.player + 0x30u) == 7.5f,
          "percentage mode reads distinct late fields and positive-scale slots");
}

}  // namespace

int main() {
    try {
        CheckMaterialGate();
        CheckChanceAndFailure();
        CheckFlatSuccess();
        CheckPercentageSuccess();
        CheckDistinctOffsets();
        Runtime rounding;
        SetStatFields(rounding,0u,0.0f);
        rounding.words[Runtime::recipe_data+0x144u]=1u;
        const auto base=0.6175009608268738f,modifier=39.851898193359375f;
        for(const auto offset:{0x64u,0x78u,0x24u,0x30u})rounding.SetFloat(Runtime::player+offset,base);
        for(const auto offset:{0x104u,0x118u,0x134u,0x140u})rounding.SetFloat(Runtime::recipe_data+offset,modifier);
        Check(pal4::ItemUseEffects::Apply(Runtime::object,88u,99u,rounding)==0 &&
            rounding.GetFloat(Runtime::player+0x64u)==0.8635868430137634f &&
            rounding.GetFloat(Runtime::player+0x24u)==0.8635868430137634f &&
            rounding.GetFloat(Runtime::player+0x78u)==0.8635867834091187f &&
            rounding.GetFloat(Runtime::player+0x30u)==0.8635867834091187f,
            "IDA last skill/attribute round their increments to float before adding");
        std::cout << "item_use_effects_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
