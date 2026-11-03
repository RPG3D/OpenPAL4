#include "pal4/script_binding_catalog.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptBindingCatalogRuntime {
    std::vector<std::uint32_t> storage;
    std::vector<std::uint32_t> cleanup;
    std::vector<pal4::ScriptBindingName> names;

    void InitializeStorage(std::uint32_t address) override {
        storage.push_back(address);
    }
    void RegisterCleanup(std::uint32_t callback) override {
        cleanup.push_back(callback);
    }
    std::string ResolveName(const pal4::ScriptBindingName& recipe) override {
        names.push_back(recipe);
        if (recipe.kind == pal4::ScriptBindingNameKind::Literal)
            return recipe.literal;
        return std::to_string(recipe.source) + ":" +
               std::to_string(recipe.index) +
               (recipe.suffix ? recipe.suffix : "");
    }
};

void CatalogRecipesAndOnceOnlyState() {
    Runtime runtime;
    pal4::ScriptBindingCatalog catalog;
    const auto& entries = catalog.Initialize(runtime);
    Check(catalog.IsStorageInitialized() && catalog.IsPopulated(),
          "both native initialization guards are set");
    Check(runtime.storage == std::vector<std::uint32_t>{
              0x8faa1c, 0x8faa2c, 0x8faa3c} &&
              runtime.cleanup == std::vector<std::uint32_t>{0x5852b0},
          "three static objects and cleanup registration preserve native order");
    Check(entries.size() == 84u && runtime.names.size() == 84u,
          "all static and loop-generated bindings are present");
    Check(std::is_sorted(entries.begin(), entries.end(),
                         [](const auto& a, const auto& b) {
                             return a.name < b.name;
                         }),
          "collection is sorted after construction");

    Check(runtime.names[0].kind == pal4::ScriptBindingNameKind::Literal &&
              runtime.names[0].source == 0x8b26e8 &&
              runtime.names[1].source == 0x8b8a94,
          "two fixed fields precede enum groups");
    Check(runtime.names[2].source == 0x8efa08 &&
              runtime.names[2].count == 5 &&
              runtime.names[2].stride == 16 &&
              runtime.names[6].index == 4,
          "first five-item group preserves source and stride");
    Check(runtime.names[7].source == 0x8ee3b8 &&
              runtime.names[7].count == 10 &&
              runtime.names[7].stride == 16 &&
              runtime.names[16].index == 9 &&
              runtime.names[17].source == 0x8ee4f8,
          "ten-item group and following boundary are exact");
    Check(runtime.names[34].source == 0x8ee458 &&
              std::string(runtime.names[34].suffix) == "_ADDITIONAL" &&
              runtime.names[40].source == 0x8ee458 &&
              std::string(runtime.names[40].suffix) == "_REACT",
          "derived property names use the native format groups");
    struct ExpectedGroup {
        std::uint32_t source;
        std::uint32_t count;
        std::uint32_t stride;
        const char* suffix;
        std::uint32_t getter;
        std::uint32_t setter;
    };
    const std::array<ExpectedGroup, 11> groups{{
        {0x8efa08, 5, 16, nullptr, 0x588c60, 0x4af760},
        {0x8ee3b8, 10, 16, nullptr, 0x588e30, 0x588d00},
        {0x8ee4f8, 8, 16, nullptr, 0x4543a0, 0x588cb0},
        {0x8ee4b8, 4, 16, nullptr, 0x588da0, 0x588c50},
        {0x8ee368, 5, 16, nullptr, 0x588d60, 0x588e80},
        {0x8ee458, 6, 16, "_ADDITIONAL", 0x588e70, 0x588c40},
        {0x8ee458, 6, 16, "_REACT", 0x588cf0, 0x588d50},
        {0x8ee458, 6, 16, "_EXTRACT", 0x588de0, 0x588e20},
        {0x8ee2e8, 6, 16, "_IMMUNITY", 0x588e60, 0x588ce0},
        {0x8ee278, 7, 16, "_IMMUNITY", 0x588e10, 0x588ca0},
        {0x8ee348, 2, 16, nullptr, 0x588dd0, 0x588c30},
    }};
    std::size_t position = 2u;
    for (const auto& group : groups) {
        for (std::uint32_t index = 0; index < group.count; ++index) {
            const auto& name = runtime.names[position++];
            Check(name.source == group.source && name.count == group.count &&
                      name.stride == group.stride && name.index == index &&
                      std::string(name.suffix ? name.suffix : "") ==
                          (group.suffix ? group.suffix : ""),
                  "each enum group preserves every native source and index");
            const auto key = std::to_string(group.source) + ":" +
                             std::to_string(index) +
                             (group.suffix ? group.suffix : "");
            const auto found = std::find_if(
                entries.begin(), entries.end(),
                [&](const auto& entry) { return entry.name == key; });
            Check(found != entries.end() &&
                      found->getter_callback == group.getter &&
                      found->setter_callback == group.setter &&
                      found->callback_index == index,
                  "each group keeps getter, setter and callback index");
        }
    }
    Check(position == 67u, "eleven enum groups end at the standalone fields");
    Check(runtime.names[67].source == 0x8ee018 &&
              runtime.names[70].kind ==
                  pal4::ScriptBindingNameKind::EnumItem &&
              runtime.names[70].index == 3 &&
              runtime.names[76].source == 0x8b935c &&
              runtime.names[77].source == 0x8ef7a8 &&
              runtime.names[83].index == 6,
          "standalone, role and seven race-attack entries retain order");

    const auto find = [&](const std::string& name) {
        return std::find_if(entries.begin(), entries.end(),
                            [&](const auto& entry) { return entry.name == name; });
    };
    const auto ai = find("AI_TYPE");
    const auto combat = find("CombatPosition");
    const auto race = find("ROLE_RACE");
    Check(ai != entries.end() && ai->object_vtable == 0x848200 &&
              ai->getter_callback == 0x4bb620 &&
              ai->setter_callback == 0x4bb630,
          "AI type uses its two native virtual callbacks");
    Check(combat != entries.end() && combat->object_size == 0x2c &&
              combat->constructor_source == 0x8ef688 &&
              combat->setter_callback == 0x588e90,
          "combat position stores the six-item constructor source");
    Check(race != entries.end() && race->object_constructor == 0x585b10 &&
              race->getter_callback == 0x4a09c0 &&
              race->setter_callback == 0x588d10,
          "role race keeps its dedicated constructor and accessors");

    const auto* original = entries.data();
    const auto& repeated = catalog.InitializeForwarded(runtime);
    Check(repeated.data() == original && repeated.size() == 84u &&
              runtime.storage.size() == 3u && runtime.cleanup.size() == 1u &&
              runtime.names.size() == 84u,
          "repeat calls return the existing sorted collection");

    catalog.Cleanup();
    Check(catalog.IsStorageInitialized() && catalog.IsPopulated() &&
              entries.empty() && entries.capacity() == 0u,
          "CRT cleanup releases the collection without resetting init guards");
    Check(catalog.Initialize(runtime).empty() &&
              runtime.names.size() == 84u && runtime.cleanup.size() == 1u,
          "native once-only guard prevents repopulation after CRT cleanup");
}

}  // namespace

int main() {
    try {
        CatalogRecipesAndOnceOnlyState();
        std::cout << "script binding catalog tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
