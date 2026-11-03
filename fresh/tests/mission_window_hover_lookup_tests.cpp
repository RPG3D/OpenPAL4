#include "pal4/mission_window_hover_lookup.h"

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

struct Runtime final : pal4::IMissionWindowHoverLookupRuntime {
    std::uint32_t hover{};
    std::uint32_t mission{0x8000u};
    std::uint32_t chapter{0x9000u};
    std::vector<std::string> calls;

    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x5400u, "hover list offset");
        calls.emplace_back("hover");
        return hover;
    }
    std::uint32_t ItemId(std::uint32_t item,
                         std::uint32_t offset) override {
        Check(item == 0x6000u && offset == 0x138u,
              "list item ID field");
        calls.emplace_back("id");
        return 17u;
    }
    std::uint32_t FindMission(std::uint32_t id) override {
        Check(id == 17u, "mission database ID");
        calls.emplace_back("mission");
        return mission;
    }
    std::uint32_t FindChapter(std::uint32_t id) override {
        Check(id == 17u, "chapter database ID");
        calls.emplace_back("chapter");
        return chapter;
    }
};

}  // namespace

int main() {
    pal4::MissionWindowState state{};
    state.Write(0x54u, 0x5400u);
    Runtime runtime;
    Check(pal4::MissionWindowHoverLookup::Mission(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"hover"},
          "missing mission hover exits before database lookup");
    runtime.calls.clear();
    Check(pal4::MissionWindowHoverLookup::Chapter(state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"hover"},
          "missing chapter hover exits before database lookup");

    runtime.hover = 0x6000u;
    runtime.calls.clear();
    Check(pal4::MissionWindowHoverLookup::Mission(state, runtime) ==
              0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "hover", "id", "mission"},
          "mission lookup uses hovered item ID");
    runtime.calls.clear();
    Check(pal4::MissionWindowHoverLookup::Chapter(state, runtime) ==
              0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "hover", "id", "chapter"},
          "chapter lookup uses hovered item ID");
    runtime.mission = 0u;
    runtime.chapter = 0u;
    Check(pal4::MissionWindowHoverLookup::Mission(state, runtime) == 0u &&
              pal4::MissionWindowHoverLookup::Chapter(state, runtime) == 0u,
          "database misses propagate without fabricated records");
    std::cout << "Mission window hover lookup tests passed\n";
}
