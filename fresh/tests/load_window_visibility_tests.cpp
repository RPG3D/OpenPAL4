#include "pal4/load_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::LoadWindowVisibilityRuntimeStub {
    std::uint32_t save_count = 47u;
    std::int32_t page{};
    std::uint32_t inserted_member{};
    std::uint32_t removed_member{};
    std::vector<std::string> calls;

    void SetParent(pal4::LoadWindowState&,
                   std::uint32_t showing,
                   std::uint32_t parent) override {
        Check(parent == 0x7000u &&
                  (showing == 0x101u || showing == 0x100u),
              "dialog receives full show word and parent");
        calls.emplace_back("parent");
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x8000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x802cu,
              "save count is read at global data +0x2c");
        calls.emplace_back("read");
        return save_count;
    }
    void PopulateSaveList(pal4::LoadWindowState&,
                          std::int32_t value,
                          std::uint32_t refresh) override {
        Check(refresh == 1u, "show refreshes the save list");
        page = value;
        calls.emplace_back("populate");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::int32_t InsertActive(std::uint32_t collection,
                              std::uint32_t member) override {
        Check(collection == 0x9134u,
              "active array is UIFrameManager +0x134");
        inserted_member = member;
        calls.emplace_back("insert");
        return -31;
    }
    std::int32_t RemoveActive(std::uint32_t collection,
                              std::uint32_t member) override {
        Check(collection == 0x9134u,
              "removal uses UIFrameManager +0x134");
        removed_member = member;
        calls.emplace_back("remove");
        return 42;
    }
};

}  // namespace

int main() {
    pal4::LoadWindowState state{};
    Runtime runtime;
    Check(pal4::LoadWindowVisibility::SetVisibility(
              state, 0x1000u, 0x101u, 0x7000u, runtime) == -31 &&
              runtime.page == 4 &&
              runtime.inserted_member == 0x1028u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "global", "read", "populate",
                  "manager", "insert"},
          "show computes signed page and inserts active member");

    runtime = Runtime{};
    runtime.save_count = static_cast<std::uint32_t>(-29);
    Check(pal4::LoadWindowVisibility::SetVisibility(
              state, 0u, 0x101u, 0x7000u, runtime) == -31 &&
              runtime.page == -2 && runtime.inserted_member == 0u,
          "negative count truncates toward zero and null object stays null");

    runtime = Runtime{};
    Check(pal4::LoadWindowVisibility::SetVisibility(
              state, 0x1000u, 0x100u, 0x7000u, runtime) == 42 &&
              runtime.removed_member == 0x1028u &&
              runtime.calls == std::vector<std::string>{
                  "parent", "manager", "remove"},
          "low show byte selects hide path and returns removal result");
    std::cout << "Load window visibility tests passed\n";
}
