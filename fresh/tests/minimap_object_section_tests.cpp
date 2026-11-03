#include "pal4/minimap_object_collection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapObjectSectionRuntime {
public:
    std::uint32_t group = 0x6000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6008u, 0x7000u}, {0x600cu, 0x7008u},
        {0x7000u, 0x8000u}, {0x7004u, 0x8100u}};
    std::vector<std::string> events;
    bool shorten_on_first_destroy = false;

    std::uint32_t FindGroup(std::uint32_t collection,
                            std::uint32_t id_pointer) override {
        Check(collection == 0x5000u && id_pointer == 0x4000u,
              "section lookup arguments");
        events.emplace_back("find");
        return group;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void DestroyIcon(std::uint32_t icon) override {
        events.emplace_back("destroy:" + std::to_string(icon));
        if (shorten_on_first_destroy && icon == 0x8000u) {
            words[0x600cu] = 0x7004u;
        }
    }
    int CleanupGroup(std::uint32_t) override {
        Check(false, "section cleanup does not destroy group");
        return 0;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
};

void TestMissingAndEmpty() {
    Runtime missing;
    missing.group = 0u;
    Check(pal4::MinimapObjectCollection::ClearSection(
              0x5000u, 0x4000u, missing) == 0u &&
              missing.events == std::vector<std::string>{"find"},
          "missing group returns find result without memory access");
    Runtime empty;
    empty.words[0x6008u] = 0u;
    empty.words[0x600cu] = 0u;
    Check(pal4::MinimapObjectCollection::ClearSection(
              0x5000u, 0x4000u, empty) == 0u &&
              empty.events == std::vector<std::string>{
                  "find", "read:24584", "read:24588",
                  "read:24584", "write:24588"},
          "null icon array still resets end to begin");
}

void TestFullAndDynamicClear() {
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::ClearSection(
              0x5000u, 0x4000u, runtime) == 0x7008u &&
              runtime.words[0x6008u] == 0x7000u &&
              runtime.words[0x600cu] == 0x7000u &&
              runtime.words[0x7000u] == 0x8000u,
          "both icons are destroyed and only the section end changes");
    Check(runtime.events == std::vector<std::string>{
              "find", "read:24584", "read:24588",
              "read:28672", "destroy:32768", "read:24584",
              "read:24588", "read:28676", "destroy:33024",
              "read:24584", "read:24588", "read:24588",
              "read:24584", "write:24588"},
          "section count is read again after every icon destruction");
    Runtime changed;
    changed.shorten_on_first_destroy = true;
    Check(pal4::MinimapObjectCollection::ClearSection(
              0x5000u, 0x4000u, changed) == 0x7004u &&
              changed.words[0x600cu] == 0x7000u &&
              changed.events.size() == 10u,
          "destructor changes to group end stop later icon destruction");
}

}  // namespace

int main() {
    TestMissingAndEmpty();
    TestFullAndDynamicClear();
    return EXIT_SUCCESS;
}
