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

class Runtime final : public pal4::IMinimapObjectRemoveRuntime {
public:
    std::uint32_t group = 0x6000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6008u, 0x7000u}, {0x600cu, 0x700cu},
        {0x7000u, 0x8000u}, {0x7004u, 0x8100u},
        {0x7008u, 0x8200u}};
    std::map<std::uint32_t, std::string> names{
        {0x8000u, "Home"}, {0x8100u, "Inn"},
        {0x8200u, "Shop"}};
    std::vector<std::string> events;

    std::uint32_t FindGroup(std::uint32_t collection,
                            std::uint32_t id_pointer) override {
        Check(collection == 0x5000u && id_pointer == 0x4000u,
              "remove lookup receives collection and ID pointer");
        events.emplace_back("find");
        return group;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    bool IconNameEquals(std::uint32_t icon,
                        std::string_view name) override {
        events.emplace_back("compare:" + std::to_string(icon));
        return names[icon] == name;
    }
    void DestroyIcon(std::uint32_t icon) override {
        events.emplace_back("destroy:" + std::to_string(icon));
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
};

std::uint32_t Remove(Runtime& runtime, std::string_view name) {
    return pal4::MinimapObjectCollection::RemoveIcon(
        0x5000u, name, 0x4000u, runtime);
}

void TestNoMatchPaths() {
    Runtime missing_group;
    missing_group.group = 0u;
    Check(Remove(missing_group, "Inn") == 0u &&
              missing_group.events == std::vector<std::string>{"find"},
          "missing group returns without memory access");
    Runtime empty_group;
    empty_group.words[0x600cu] = 0x7000u;
    Check(Remove(empty_group, "Inn") == 0u &&
              empty_group.events == std::vector<std::string>{
                  "find", "read:24584", "read:24588"},
          "empty nested pointer range returns without comparison");
    Runtime missing_name;
    Check(Remove(missing_name, "Castle") == 0u &&
              missing_name.words[0x600cu] == 0x700cu &&
              missing_name.events.size() == 9u,
          "missing name preserves every pointer and group end");
}

void TestRemovalAndCompaction() {
    Runtime middle;
    Check(Remove(middle, "Inn") == 0x700cu &&
              middle.words[0x7000u] == 0x8000u &&
              middle.words[0x7004u] == 0x8200u &&
              middle.words[0x600cu] == 0x7008u &&
              middle.words[0x7008u] == 0x8200u,
          "middle deletion shifts later pointers and leaves old tail intact");
    Check(middle.events == std::vector<std::string>{
              "find", "read:24584", "read:24588",
              "read:28672", "compare:32768", "read:28676",
              "compare:33024", "destroy:33024", "read:28680",
              "write:28676", "write:24588"},
          "comparison, destruction and compaction preserve call order");
    Runtime last;
    Check(Remove(last, "Shop") == 0x700cu &&
              last.words[0x600cu] == 0x7008u &&
              last.words[0x7008u] == 0x8200u,
          "last deletion returns old end without moving tail memory");
    Runtime first;
    Check(Remove(first, "Home") == 0x700cu &&
              first.words[0x7000u] == 0x8100u &&
              first.words[0x7004u] == 0x8200u &&
              first.words[0x600cu] == 0x7008u,
          "first deletion shifts both surviving icons");
}

}  // namespace

int main() {
    TestNoMatchPaths();
    TestRemovalAndCompaction();
    return EXIT_SUCCESS;
}
