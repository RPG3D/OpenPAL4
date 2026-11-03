#include "pal4/crafting_extended_application.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICraftingExtendedRuntime {
    static constexpr std::uint32_t recipe_data = 0x2000u;
    static constexpr std::uint32_t entry = 0x4000u;
    static constexpr std::uint32_t object = 0x5000u;
    static constexpr std::uint32_t temporary = 0x6000u;

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> lengths{
        {0x8000u, 3u}, {0x8100u, 5u}, {0x9000u, 0u}};
    std::set<std::uint32_t> direct_items{10u, 30u};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> removed;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> c_strings;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> text_assignments;
    std::uint8_t temporary_mode = 0xffu;
    bool temporary_alive = false;
    bool throw_on_destination = false;
    int fees = 0;

    Runtime() {
        Set(0, 30u, 4);
        Set(1, 10u, 2);
        Set(2, 30u, 9);
        Set(3, 40u, 0);
        words[recipe_data + 0x74u] = 0x1234u;
        words[recipe_data + 0x28u] = 0x8000u;
        words[recipe_data + 0x08u] = 0x8100u;
        words[entry + 0x94u] = 0u;
        bytes[recipe_data + 0x34u] = 3u;
        bytes[recipe_data + 0x44u] = 5u;
        bytes[recipe_data + 0x54u] = 7u;
        bytes[object] = 0u;
    }

    void Set(std::uint32_t index, std::uint32_t item, std::int32_t count) {
        words[recipe_data + 0x98u + index * 4u] = item;
        words[recipe_data + 0xa8u + index * 4u] =
            static_cast<std::uint32_t>(count);
    }
    void PlayMusic(std::string_view name, std::uint32_t mode) override {
        Check(name == "wa09.wav" && mode == 1u, "music arguments");
        calls.emplace_back("music");
    }
    std::uint32_t GameDb() override {
        calls.emplace_back("db");
        return 0x1000u;
    }
    std::uint32_t FindRecipe(std::uint32_t collection,
                             std::uint32_t recipe) override {
        Check(collection == 0x1080u && recipe == 88u,
              "recipe lookup arguments");
        calls.emplace_back("recipe");
        return recipe_data;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == entry + 0x180u, "property destination");
        words[address] = value;
        calls.emplace_back("property");
    }
    bool IsDirectItem(std::uint32_t database,
                       std::uint32_t item) override {
        Check(database == 0x1000u, "direct item database");
        calls.push_back("direct? " + std::to_string(item));
        return direct_items.contains(item);
    }
    void FindOrLogDirectItem(std::uint32_t database,
                             std::uint32_t item) override {
        Check(database == 0x1000u && direct_items.contains(item),
              "direct item logging");
        calls.push_back("log " + std::to_string(item));
    }
    std::uint32_t Inventory() override {
        calls.emplace_back("inventory");
        return 0x7000u;
    }
    void RemoveDirectItem(std::uint32_t inventory, std::uint32_t item,
                          std::uint32_t quantity) override {
        Check(inventory == 0x7000u, "direct item inventory");
        removed.emplace_back(item, quantity);
        calls.push_back("remove " + std::to_string(item));
    }
    std::uint32_t FindInventoryEntry(std::uint32_t collection,
                                     std::uint32_t key,
                                     std::uint32_t category) override {
        Check(collection == 0x707cu && key == 99u && category == 7u,
              "target entry lookup");
        calls.emplace_back("entry");
        return entry;
    }
    std::uint32_t EmptyCString() override {
        calls.emplace_back("empty");
        return 0x9000u;
    }
    std::uint32_t CStringLength(std::uint32_t source) override {
        return lengths.at(source);
    }
    void AssignCString(std::uint32_t destination, std::uint32_t source,
                       std::uint32_t length) override {
        c_strings.emplace_back(destination, source, length);
        calls.emplace_back("c string");
    }
    std::uint32_t CreateTemporaryText(std::uint8_t mode) override {
        Check(!temporary_alive, "one temporary at a time");
        temporary_alive = true;
        temporary_mode = mode;
        calls.emplace_back("temp create");
        return temporary;
    }
    void AssignText(std::uint32_t destination, std::uint32_t source,
                    std::uint32_t start, std::uint32_t count) override {
        Check(start == 0u && count == 0xffffffffu,
              "full text assignment");
        text_assignments.emplace_back(destination, source);
        calls.emplace_back("text assign");
        if (throw_on_destination && destination == entry + 0xf8u)
            throw std::runtime_error("destination assignment failed");
    }
    void ReleaseTemporaryText(std::uint32_t address) override {
        Check(address == temporary && temporary_alive,
              "temporary release address");
        temporary_alive = false;
        calls.emplace_back("temp release");
    }
    void UpdateAndClampCounter(std::uint32_t inventory,
                               std::int32_t delta) override {
        Check(inventory == 0x7000u && delta == -80,
              "crafting cost");
        ++fees;
        calls.emplace_back("fee");
    }
};

std::size_t IndexOf(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "missing call");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckSuccessBranches() {
    for (std::uint32_t state = 0u; state < 4u; ++state) {
        Runtime runtime;
        runtime.words[Runtime::entry + 0x94u] = state;
        runtime.bytes[Runtime::object] = 1u;
        const auto result = pal4::CraftingExtendedApplication::Apply(
            Runtime::object, 88u, 99u, runtime);
        Check(result == 0 && runtime.fees == 0,
              "success without fee for nonzero object flag");
        Check(runtime.removed ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {10u, 2u}, {30u, 4u}}),
              "sorted unique direct ingredients");
        Check(runtime.words.at(Runtime::entry + 0x180u) == 0x1234u,
              "recipe property copied");
        Check(runtime.c_strings ==
                  (std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                          std::uint32_t>>{
                      {Runtime::entry + 0xb8u, 0x8000u, 3u},
                      {Runtime::entry + 0x108u, 0x8100u, 5u}}),
              "both C strings assigned with exact lengths");
        Check(IndexOf(runtime, "music") < IndexOf(runtime, "recipe") &&
                  IndexOf(runtime, "remove 30") <
                      IndexOf(runtime, "entry") &&
                  IndexOf(runtime, "property") <
                      IndexOf(runtime, "c string"),
              "transaction call order");
        if (state < 3u) {
            const auto source = Runtime::recipe_data + 0x34u + state * 0x10u;
            Check(runtime.temporary_mode == 3u + state * 2u &&
                      runtime.text_assignments ==
                          (std::vector<std::pair<std::uint32_t,
                                                 std::uint32_t>>{
                              {Runtime::temporary, source},
                              {Runtime::entry + 0xf8u, Runtime::temporary}}) &&
                      !runtime.temporary_alive &&
                      IndexOf(runtime, "text assign") <
                          IndexOf(runtime, "temp release"),
                  "selected text copied and temporary released");
        } else {
            Check(runtime.text_assignments.empty() &&
                      !runtime.temporary_alive,
                  "other state skips text selection");
        }
    }
}

void CheckPendingMaterial() {
    Runtime runtime;
    runtime.Set(3, 20u, 1);
    Check(pal4::CraftingExtendedApplication::Apply(
              Runtime::object, 88u, 99u, runtime) == -1 &&
              runtime.removed.size() == 2u && runtime.fees == 0 &&
              runtime.c_strings.empty() &&
              std::find(runtime.calls.begin(), runtime.calls.end(), "entry") ==
                  runtime.calls.end(),
          "pending material fails after consuming direct ingredients");
}

void CheckFallbackAndFee() {
    Runtime runtime;
    runtime.words[Runtime::recipe_data + 0x28u] = 0u;
    runtime.words[Runtime::recipe_data + 0x08u] = 0u;
    Check(pal4::CraftingExtendedApplication::Apply(
              Runtime::object, 88u, 99u, runtime) == 0 &&
              runtime.c_strings ==
                  (std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                          std::uint32_t>>{
                      {Runtime::entry + 0xb8u, 0x9000u, 0u},
                      {Runtime::entry + 0x108u, 0x9000u, 0u}}) &&
              runtime.fees == 1 &&
              IndexOf(runtime, "temp release") < IndexOf(runtime, "fee"),
          "null strings use shared empty string and zero flag pays cost");
}

void CheckExceptionalCleanup() {
    Runtime runtime;
    runtime.throw_on_destination = true;
    bool thrown = false;
    try {
        static_cast<void>(pal4::CraftingExtendedApplication::Apply(
            Runtime::object, 88u, 99u, runtime));
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && !runtime.temporary_alive && runtime.fees == 0 &&
              runtime.calls.back() == "temp release",
          "temporary is released when destination assignment throws");
}

}  // namespace

int main() {
    try {
        CheckSuccessBranches();
        CheckPendingMaterial();
        CheckFallbackAndFee();
        CheckExceptionalCleanup();
        std::cout << "crafting_extended_application_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
