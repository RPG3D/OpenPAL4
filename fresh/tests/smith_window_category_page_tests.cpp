#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowCategoryPageRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0xccu, 0x8000u},
        {object + 0xd0u, 0x8008u},
        {object + 0xb0u, 1u},
        {object + 0x58u, 0x3000u},
        {object + 0x5cu, 0x3004u},
        {0x8000u, 5u}, {0x8004u, 6u},
    };
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::uint32_t> categories;
    bool shrink_to_one = false;
    bool clear_after_update = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void UpdateEquipmentList(std::uint32_t owner,
                             std::uint32_t category) override {
        Check(owner == object, "equipment-list owner");
        categories.push_back(category);
        calls.emplace_back("update");
        if (shrink_to_one) words[object + 0xd0u] = 0x8004u;
        if (clear_after_update) words[object + 0xccu] = 0u;
    }
    void SetVisible(std::uint32_t window,
                    bool visible) override {
        visibility.emplace_back(window, visible);
        calls.emplace_back("visible");
    }
};

void CheckCategoryCountAndSelection() {
    Runtime multiple;
    pal4::SmithWindow::UpdateCategoryPageButtons(
        Runtime::object, multiple);
    Check(multiple.categories == std::vector<std::uint32_t>{6u} &&
              multiple.visibility == (std::vector<
                  std::pair<std::uint32_t, bool>>{
                      {0x3000u, true}, {0x3004u, true}}),
          "selected second category and two-page arrows");
    Check(multiple.calls.at(0) == "read" &&
              multiple.calls.at(4) == "update",
          "category lookup precedes list refresh");

    Runtime one;
    one.words[Runtime::object + 0xd0u] = 0x8004u;
    one.words[Runtime::object + 0xb0u] = 0u;
    pal4::SmithWindow::UpdateCategoryPageButtons(
        Runtime::object, one);
    Check(one.categories == std::vector<std::uint32_t>{5u} &&
              one.visibility == (std::vector<
                  std::pair<std::uint32_t, bool>>{
                      {0x3000u, false}, {0x3004u, false}}),
          "single category refreshes list and hides arrows");
}

void CheckEmptyAndMutatedArray() {
    Runtime null_begin;
    null_begin.words[Runtime::object + 0xccu] = 0u;
    pal4::SmithWindow::UpdateCategoryPageButtons(
        Runtime::object, null_begin);
    Check(null_begin.categories.empty() &&
              !null_begin.visibility.at(0).second &&
              !null_begin.visibility.at(1).second,
          "null category array hides arrows without refresh");

    Runtime empty;
    empty.words[Runtime::object + 0xd0u] = 0x8000u;
    pal4::SmithWindow::UpdateCategoryPageButtons(
        Runtime::object, empty);
    Check(empty.categories.empty() &&
              !empty.visibility.at(0).second,
          "empty category array hides arrows");

    Runtime shrunk;
    shrunk.shrink_to_one = true;
    pal4::SmithWindow::UpdateCategoryPageButtons(
        Runtime::object, shrunk);
    Check(shrunk.categories == std::vector<std::uint32_t>{6u} &&
              !shrunk.visibility.at(0).second,
          "button visibility rechecks array after list refresh");

    Runtime cleared;
    cleared.clear_after_update = true;
    pal4::SmithWindow::UpdateCategoryPageButtons(
        Runtime::object, cleared);
    Check(cleared.categories.size() == 1u &&
              !cleared.visibility.at(0).second,
          "cleared array hides buttons after list refresh");
}

}  // namespace

int main() {
    try {
        CheckCategoryCountAndSelection();
        CheckEmptyAndMutatedArray();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
