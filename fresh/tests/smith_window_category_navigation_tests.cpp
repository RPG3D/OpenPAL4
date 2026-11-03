#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowCategoryNavigationRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::uint8_t selected = 1u;
    std::uint32_t mode = 1u;
    std::uint32_t begin = 0x2000u;
    std::uint32_t end = 0x2008u;
    std::uint32_t index = 0u;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == object + 0xacu, "selection offset");
        calls.emplace_back("selection");
        return selected;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == object + 0xb4u) {
            calls.emplace_back("mode");
            return mode;
        }
        if (address == object + 0xccu) return begin;
        if (address == object + 0xd0u) return end;
        Check(address == object + 0xb0u, "category index offset");
        return index;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == object + 0xb0u, "written category index");
        index = value;
        calls.emplace_back("write");
    }
    void RefreshAfterInventoryChange(std::uint32_t owner) override {
        Check(owner == object, "inventory refresh owner");
        calls.emplace_back("refresh");
    }
    void UpdateCategoryPageButtons(std::uint32_t owner) override {
        Check(owner == object, "button update owner");
        calls.emplace_back("buttons");
    }
};

void CheckGates() {
    Runtime hidden;
    hidden.selected = 0u;
    Check(pal4::SmithWindow::OnCategoryPreviousClick(
              Runtime::object, 0x1234u, hidden) == 1u &&
              hidden.calls == std::vector<std::string>{"selection"},
          "inactive selection returns before mode read");
    Runtime refresh;
    refresh.mode = 0u;
    Check(pal4::SmithWindow::OnCategoryNextClick(
              Runtime::object, 0u, refresh) == 1u &&
              refresh.calls == (std::vector<std::string>{
                  "selection", "mode", "refresh"}),
          "mode zero refreshes inventory without page navigation");
    Runtime absent;
    absent.begin = 0u;
    (void)pal4::SmithWindow::OnCategoryPreviousClick(
        Runtime::object, 0u, absent);
    Check(absent.index == 0u &&
              absent.calls == (std::vector<std::string>{
                  "selection", "mode"}),
          "absent category array does not navigate");
    Runtime single;
    single.end = single.begin + 4u;
    (void)pal4::SmithWindow::OnCategoryNextClick(
        Runtime::object, 0u, single);
    Check(single.index == 0u &&
              single.calls == (std::vector<std::string>{
                  "selection", "mode"}),
          "single category does not navigate");
}

void CheckNavigation() {
    Runtime previous;
    previous.index = 1u;
    Check(pal4::SmithWindow::OnCategoryPreviousClick(
              Runtime::object, 0x1234u, previous) == 1u &&
              previous.index == 0u &&
              previous.calls == (std::vector<std::string>{
                  "selection", "mode", "write", "buttons"}),
          "previous category decrements once");
    Runtime previous_wrap;
    (void)pal4::SmithWindow::OnCategoryPreviousClick(
        Runtime::object, 0u, previous_wrap);
    Check(previous_wrap.index == 1u &&
              previous_wrap.calls == (std::vector<std::string>{
                  "selection", "mode", "write", "write", "buttons"}),
          "previous category wraps to last entry");

    Runtime next;
    Check(pal4::SmithWindow::OnCategoryNextClick(
              Runtime::object, 0x1234u, next) == 1u &&
              next.index == 1u &&
              next.calls == (std::vector<std::string>{
                  "selection", "mode", "write", "buttons"}),
          "next category increments once");
    Runtime next_wrap;
    next_wrap.index = 1u;
    (void)pal4::SmithWindow::OnCategoryNextClick(
        Runtime::object, 0u, next_wrap);
    Check(next_wrap.index == 0u &&
              next_wrap.calls == (std::vector<std::string>{
                  "selection", "mode", "write", "write", "buttons"}),
          "next category wraps to first entry");

    Runtime three_items;
    three_items.end = three_items.begin + 12u;
    (void)pal4::SmithWindow::OnCategoryPreviousClick(
        Runtime::object, 0u, three_items);
    Check(three_items.index == 2u,
          "wrap uses three-entry array length");
}

}  // namespace

int main() {
    try {
        CheckGates();
        CheckNavigation();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
