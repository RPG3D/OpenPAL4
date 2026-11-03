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

class Runtime final : public pal4::IMinimapObjectAddRuntime {
public:
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> words{{0x5008u, 0x5010u},
                                                 {0x600cu, 0x7008u}};
    pal4::MinimapObjectGroupRecord inserted{};
    std::uint32_t group = 0x6000u;
    bool group_exists = true;
    bool insertion_visible = true;
    bool duplicate = false;
    bool load_success = true;
    std::int32_t collection_size = 0;
    std::uint32_t icon_allocation = 0x8000u;
    std::uint32_t loaded_icon = 0u;

    std::uint32_t FindGroup(std::uint32_t collection,
                            std::uint32_t id_pointer) override {
        Check(collection == 0x5000u && id_pointer == 0x4000u,
              "group lookup arguments");
        events.emplace_back("find-group");
        return group_exists ? group : 0u;
    }
    std::int32_t CollectionSize(
        const pal4::MinimapObjectGroupRecord& record) override {
        Check(record.id_pointer == 0x4000u &&
                  record.name_pointer_low_byte == 0x42u &&
                  record.begin == 0u && record.end == 0u &&
                  record.capacity == 0u,
              "temporary group starts with ID, pointer low byte and zeros");
        events.emplace_back("size");
        return collection_size;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("allocate:" + std::to_string(bytes));
        return bytes == 0x48u ? icon_allocation : 0x9000u;
    }
    std::uint32_t CopyDwordArray(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination) override {
        Check(begin == 0u && end == 0u && destination == 0x9000u,
              "empty source range copied to temporary allocation");
        events.emplace_back("copy");
        return destination;
    }
    void InsertGroup(std::uint32_t collection,
                     std::uint32_t insertion_address,
                     const pal4::MinimapObjectGroupRecord& record) override {
        Check(collection == 0x5000u && insertion_address == 0x5010u,
              "new group appended at current end");
        inserted = record;
        group_exists = insertion_visible;
        events.emplace_back("insert-group");
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    bool FindIconByName(std::uint32_t collection,
                        std::string_view name) override {
        Check(collection == 0x6004u && name == "Inn",
              "duplicate lookup uses nested group and name");
        events.emplace_back("find-name");
        return duplicate;
    }
    std::uint32_t ConstructIcon(std::uint32_t address) override {
        Check(address == 0x8000u, "icon constructed in 0x48 allocation");
        events.emplace_back("construct-icon");
        return address;
    }
    bool LoadIconTexture(std::uint32_t icon,
                         std::string_view filename,
                         std::uint32_t id_pointer) override {
        Check(filename == "Inn.png" && id_pointer == 0x4000u,
              "texture loading receives filename and ID pointer");
        loaded_icon = icon;
        events.emplace_back("load-icon");
        return load_success;
    }
    void DestroyIcon(std::uint32_t icon) override {
        Check(icon == loaded_icon, "failed icon is destroyed");
        events.emplace_back("destroy-icon");
    }
    void SetIconName(std::uint32_t icon_string,
                     std::string_view name) override {
        Check(icon_string == 0x8008u && name == "Inn",
              "name stored in icon string at +8");
        events.emplace_back("name");
    }
    void SetIconCenter(std::uint32_t icon,
                       std::int32_t x, std::int32_t y) override {
        Check(icon == 0x8000u && x == -12 && y == 34,
              "icon centered after name initialization");
        events.emplace_back("center");
    }
    void InsertIcon(std::uint32_t collection,
                    std::uint32_t insertion_address,
                    std::uint32_t icon) override {
        Check(collection == 0x6004u && insertion_address == 0x7008u &&
                  icon == 0x8000u,
              "icon appended to nested pointer array");
        events.emplace_back("insert-icon");
    }
};

bool Add(Runtime& runtime) {
    return pal4::MinimapObjectCollection::AddIcon(
        0x5000u, "Inn", 0x42u, 0x4000u, "Inn.png", -12, 34,
        runtime);
}

void TestExistingGroupSuccess() {
    Runtime runtime;
    Check(Add(runtime) && runtime.events == std::vector<std::string>{
              "find-group", "find-name", "allocate:72",
              "construct-icon", "load-icon", "name", "center",
              "read:24588", "insert-icon"},
          "existing group inserts loaded icon in binary call order");
}

void TestDuplicateAndFailedLoad() {
    Runtime duplicate;
    duplicate.duplicate = true;
    Check(!Add(duplicate) && duplicate.events ==
              std::vector<std::string>{"find-group", "find-name"},
          "duplicate icon skips allocation and loading");
    Runtime failure;
    failure.load_success = false;
    Check(!Add(failure) && failure.events ==
              std::vector<std::string>{
                  "find-group", "find-name", "allocate:72",
                  "construct-icon", "load-icon", "destroy-icon"},
          "failed texture load destroys constructed icon");
    Runtime no_memory;
    no_memory.icon_allocation = 0u;
    no_memory.load_success = false;
    Check(!Add(no_memory) && no_memory.loaded_icon == 0u &&
              no_memory.events == std::vector<std::string>{
                  "find-group", "find-name", "allocate:72",
                  "load-icon", "destroy-icon"},
          "allocation failure still forwards null icon to texture loader");
}

void TestMissingGroup() {
    Runtime runtime;
    runtime.group_exists = false;
    runtime.collection_size = -5;
    Check(Add(runtime) && runtime.inserted.begin == 0x9000u &&
              runtime.inserted.end == 0x9000u &&
              runtime.inserted.capacity == 0x9000u &&
              runtime.events == std::vector<std::string>{
                  "find-group", "size", "allocate:0", "copy",
                  "read:20488", "insert-group", "free:36864",
                  "free:0", "find-group", "find-name",
                  "allocate:72", "construct-icon", "load-icon",
                  "name", "center", "read:24588", "insert-icon"},
          "negative size clamps to zero and temporary group is released");
    Runtime missing_after_insert;
    missing_after_insert.group_exists = false;
    missing_after_insert.insertion_visible = false;
    Check(!Add(missing_after_insert) &&
              missing_after_insert.events.back() == "find-group" &&
              missing_after_insert.events.size() == 9u,
          "failed group relookup returns before icon allocation");
}

}  // namespace

int main() {
    TestExistingGroupSuccess();
    TestDuplicateAndFailedLoad();
    TestMissingGroup();
    return EXIT_SUCCESS;
}
