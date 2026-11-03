#include "pal4/property_window_magic_item.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowMagicItemRuntime {
    std::vector<std::string> calls;
    bool visible = false;
    std::uint32_t entry = 0u;

    bool IsDialogVisible(pal4::PropertyWindowState&) override {
        calls.emplace_back("visible");
        return visible;
    }
    void ToggleModel(pal4::PropertyWindowState&, bool value) override {
        Check(value, "model toggle receives true");
        calls.emplace_back("model");
    }
    std::uint32_t FindInventoryEntry(std::uint32_t id,
                                     std::uint32_t category) override {
        Check(id == 3182u && category == 5u,
              "magic item lookup uses fixed ID and inventory category");
        calls.emplace_back("find");
        return entry;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == entry + 4u,
              "present entry reads replacement item at +4");
        calls.emplace_back("next");
        return 0x8300u;
    }
    void SetProperty(std::uint32_t category,
                     std::uint32_t item) override {
        Check(category == 5u, "property uses selected category");
        calls.push_back("property:" + std::to_string(item));
    }
    void ShowOrHideDialog(std::uint32_t mode, std::uint32_t callback,
                          std::uint32_t item) override {
        Check(mode == 0u && callback == 0u && item == 0u,
              "missing entry closes dialog with three zero arguments");
        calls.emplace_back("hide");
    }
    void PopulateList(pal4::PropertyWindowState&,
                      std::uint32_t category,
                      bool preserve_scroll) override {
        Check(category == 5u && !preserve_scroll,
              "magic item refreshes selected category without scroll");
        calls.emplace_back("populate");
    }
};

void TestVisibilityAndInventory() {
    pal4::PropertyWindowState state;
    state.Write(0x6cu, 5u);
    Runtime runtime;
    Check(pal4::PropertyWindowMagicItem::Use(state, runtime),
          "hidden dialog returns true");
    Check(runtime.calls == std::vector<std::string>{"visible"},
          "hidden dialog has no inventory side effects");

    runtime.calls.clear();
    runtime.visible = true;
    Check(pal4::PropertyWindowMagicItem::Use(state, runtime),
          "missing inventory entry returns true");
    Check(runtime.calls == std::vector<std::string>{
              "visible", "model", "find", "property:0", "hide", "populate"},
          "missing magic item clears property and closes dialog before list refresh");

    runtime.calls.clear();
    runtime.entry = 0x8100u;
    Check(pal4::PropertyWindowMagicItem::Use(state, runtime),
          "present inventory entry returns true");
    Check(runtime.calls == std::vector<std::string>{
              "visible", "model", "find", "next", "property:33536",
              "populate"},
          "present magic item selects next entry without closing dialog");
}

}  // namespace

int main() { TestVisibilityAndInventory(); }
