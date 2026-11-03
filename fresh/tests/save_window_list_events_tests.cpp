#include "pal4/save_window_list_events.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::SaveWindowListEventRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool fail_populate{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void PopulateSaveList(pal4::SaveWindowState&, std::int32_t page,
                          std::uint8_t reset) override {
        Check(page == 3 && reset == 0u,
              "click uses button page index without reset");
        calls.emplace_back("populate");
        if (fail_populate) throw std::runtime_error("populate");
    }
    void DisplayHoverItemInfo(pal4::SaveWindowState&) override {
        calls.emplace_back("hover-info");
    }
};

}  // namespace

int main() {
    pal4::SaveWindowState state{};
    Runtime runtime;
    runtime.words[0x5008u] = 0x2100u;
    runtime.words[0x2218u] = 3u;
    Check(pal4::SaveWindowListEvents::OnPageButtonClick(
              state, 0x5000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:20488", "read:8728", "populate"},
          "save page click reads event window then button page");
    runtime.calls.clear();
    Check(pal4::SaveWindowListEvents::OnListClick(
              state, 0x5000u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"hover-info"},
          "list click ignores event and opens item confirmation");
    runtime.calls.clear();
    runtime.fail_populate = true;
    try {
        (void)pal4::SaveWindowListEvents::OnPageButtonClick(
            state, 0x5000u, runtime);
        Check(false, "populate failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "populate",
              "failure does not consume event after throwing");
    }
    std::cout << "Save window list event tests passed\n";
}
