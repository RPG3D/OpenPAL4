#include "pal4/minimap_manager.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapManagerRestoreRuntime {
public:
    pal4::MinimapManagerObjectState manager;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    bool shrink_on_first_mask = false;

    Runtime() {
        manager.WriteDword(0x18u, 0x5000u);
        manager.WriteDword(0x1cu, 0x5008u);
        manager.WriteDword(0x50u, 2u);
        words[0x5000u] = 0x1111u;
        words[0x5004u] = 0x2222u;
        words[0x8008u] = 0x9000u;
        words[0x800cu] = 0x9008u;
        words[0x8014u] = 1u;
        words[0x9000u] = 0xa000u;
        words[0x9004u] = 0xa010u;
    }
    pal4::MinimapManagerObjectState& AccessManager(
        std::uint32_t address) override {
        Check(address == 0x1000u, "manager address");
        events.emplace_back("access");
        return manager;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void ApplyGridMask(std::uint32_t grid,
                       std::uint32_t mask_record) override {
        events.emplace_back("apply:" + std::to_string(grid) + ":" +
                            std::to_string(mask_record));
        if (shrink_on_first_mask) {
            manager.WriteDword(0x1cu, 0x5004u);
        }
    }
    void SelectTexture(std::uint32_t manager_address,
                       std::uint32_t index) override {
        Check(manager_address == 0x1000u && index == 1u,
              "snapshot texture index forwarded");
        events.emplace_back("select");
    }
};

void Restore(Runtime& runtime) {
    pal4::MinimapManagerObject::RestoreGridData(0x1000u, 0x8000u, runtime);
}

void TestMatchingLists() {
    Runtime runtime;
    Restore(runtime);
    Check(runtime.events == std::vector<std::string>{
              "access", "read:32776", "read:32780",
              "read:20480", "read:36864", "apply:4369:40960",
              "read:20484", "read:36868", "apply:8738:40976",
              "read:32788", "select"},
          "matching layers apply masks in order then select texture");
}

void TestMismatchAndOtherMode() {
    Runtime runtime;
    runtime.words[0x800cu] = 0x9004u;
    Restore(runtime);
    Check(runtime.events == std::vector<std::string>{
              "access", "read:32776", "read:32780"},
          "mismatched mask count returns without applying or selecting");
    runtime.events.clear();
    runtime.manager.WriteDword(0x50u, 1u);
    Restore(runtime);
    Check(runtime.events ==
              std::vector<std::string>{"access", "read:32788", "select"},
          "other mode selects without examining mask collection");
}

void TestEmptyAndChangingList() {
    Runtime empty;
    empty.manager.WriteDword(0x18u, 0u);
    empty.words[0x8008u] = 0u;
    Restore(empty);
    Check(empty.events == std::vector<std::string>{
              "access", "read:32776", "read:32788", "select"},
          "two empty lists still select saved texture index");
    Runtime changing;
    changing.shrink_on_first_mask = true;
    Restore(changing);
    Check(changing.events.back() == "select" &&
              changing.events.size() == 8u &&
              changing.events[5] == "apply:4369:40960",
          "loop observes changed grid end after each mask");
}

}  // namespace

int main() {
    TestMatchingLists();
    TestMismatchAndOtherMode();
    TestEmptyAndChangingList();
    return EXIT_SUCCESS;
}
