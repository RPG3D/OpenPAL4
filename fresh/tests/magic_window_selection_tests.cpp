#include "pal4/magic_window_selection.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowSelectionRuntimeStub {
    std::set<std::uint32_t> selected;
    std::vector<std::uint32_t> queried;
    bool IsSelected(std::uint32_t button) override {
        queried.push_back(button);
        return selected.contains(button);
    }
};

pal4::MagicWindowPanelState Panel() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    return panel;
}

void TestMagic() {
    const auto panel = Panel();
    for (std::int32_t index = 0; index < 6; ++index) {
        Runtime runtime;
        runtime.selected.insert(panel.At(0xc0u + 4u * index));
        Check(pal4::MagicWindowSelection::SelectedMagicRow(panel, runtime) == index &&
                  runtime.queried.size() == static_cast<std::size_t>(index + 1) &&
                  runtime.queried.front() == panel.At(0xc0u),
              "six magic rows scanned in order and stop on first selected");
    }
    Runtime none;
    Check(pal4::MagicWindowSelection::SelectedMagicRow(panel, none) == -1 &&
              none.queried.size() == 6u && none.queried.back() == panel.At(0xd4u),
          "no magic selection returns sentinel after six rows");
    Runtime multiple;
    multiple.selected = {panel.At(0xc8u), panel.At(0xd0u)};
    Check(pal4::MagicWindowSelection::SelectedMagicRow(panel, multiple) == 2,
          "first selected magic row wins");
}

void TestStunt() {
    const auto panel = Panel();
    for (std::int32_t index = 0; index < 7; ++index) {
        Runtime runtime;
        runtime.selected.insert(panel.At(0x108u + 4u * index));
        Check(pal4::MagicWindowSelection::SelectedStuntRow(panel, runtime) == index &&
                  runtime.queried.size() == static_cast<std::size_t>(index + 1) &&
                  runtime.queried.front() == panel.At(0x108u),
              "seven stunt rows scanned in order and stop on first selected");
    }
    Runtime none;
    Check(pal4::MagicWindowSelection::SelectedStuntRow(panel, none) == -1 &&
              none.queried.size() == 7u && none.queried.back() == panel.At(0x120u),
          "no stunt selection returns sentinel after seven rows");
    Runtime multiple;
    multiple.selected = {panel.At(0x110u), panel.At(0x11cu)};
    Check(pal4::MagicWindowSelection::SelectedStuntRow(panel, multiple) == 2,
          "first selected stunt row wins");
}

}  // namespace

int main() {
    TestMagic();
    TestStunt();
}
