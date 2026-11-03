#include "pal4/scenario_dialog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScenarioTextDisplayRuntime {
    std::vector<bool> end_values;
    std::vector<bool> page_values;
    std::size_t end_index = 0u;
    std::size_t page_index = 0u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1038u, "editbox member offset");
        calls.emplace_back("read");
        return 0x2000u;
    }
    bool IsTypeEnd(std::uint32_t editbox) override {
        Check(editbox == 0x2000u &&
                  end_index < end_values.size(), "type-end query");
        calls.emplace_back("end");
        return end_values[end_index++];
    }
    bool NeedChangePage(std::uint32_t editbox) override {
        Check(editbox == 0x2000u &&
                  page_index < page_values.size(), "page query");
        calls.emplace_back("page");
        return page_values[page_index++];
    }
    void ChangePage(std::uint32_t editbox) override {
        Check(editbox == 0x2000u, "page target");
        calls.emplace_back("change-page");
    }
    void ChangeLine(std::uint32_t editbox) override {
        Check(editbox == 0x2000u, "line target");
        calls.emplace_back("change-line");
    }
    void RequestRedraw(std::uint32_t editbox) override {
        Check(editbox == 0x2000u, "redraw target");
        calls.emplace_back("redraw");
    }
    void HideAndClear(std::uint32_t object) override {
        Check(object == 0x1000u, "hide owner");
        calls.emplace_back("hide");
    }
};

void Run(const std::vector<bool>& ends,
         const std::vector<bool>& pages,
         const std::vector<std::string>& expected) {
    Runtime runtime;
    runtime.end_values = ends;
    runtime.page_values = pages;
    pal4::ScenarioDialog::HandleTextDisplay(0x1000u, runtime);
    Check(runtime.calls == expected &&
              runtime.end_index == ends.size() &&
              runtime.page_index == pages.size(),
          "text advance branch and query order");
}

}  // namespace

int main() {
    try {
        Run({true}, {}, {"read", "end", "hide"});
        Run({false}, {true},
            {"read", "end", "page", "change-page"});
        Run({false}, {false, true},
            {"read", "end", "page", "page", "redraw"});
        Run({false, false, true}, {false, false, false},
            {"read", "end", "page", "page", "change-line",
             "end", "page", "change-line", "end", "redraw"});
        Run({false, false}, {false, false, true},
            {"read", "end", "page", "page", "change-line",
             "end", "page", "redraw"});
        std::cout << "scenario_dialog_text_display_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
