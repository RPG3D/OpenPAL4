#include "pal4/property_window_special_item.h"

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

struct Runtime final : pal4::IPropertyWindowSpecialItemRuntime {
    std::vector<std::string> calls;
    std::uint8_t event_result = 0x7fu;

    void ToggleModel(std::uint32_t frame, std::uint32_t context,
                     std::uint32_t visible) override {
        Check(frame == 0x1000u && context == 0x2000u && visible == 0u,
              "hide model arguments");
        calls.emplace_back("hide");
    }
    void UpdateQuestModel(std::uint32_t frame,
                          std::uint32_t item) override {
        Check(frame == 0x1000u && item == 0u, "clear current model");
        calls.emplace_back("clear");
    }
    std::uint32_t Inventory() override {
        calls.emplace_back("inventory");
        return 0x3000u;
    }
    void RemoveItem(std::uint32_t inventory, std::uint32_t item,
                    std::uint32_t count) override {
        Check(inventory == 0x3000u && item == 3182u && count == 1u,
              "remove exactly one item 3182");
        calls.emplace_back("remove");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint8_t ProcessEvents(
        std::uint32_t table, std::uint32_t event,
        const std::array<std::uint32_t, 2>& payload) override {
        Check(table == 0x4104u &&
                  event == pal4::PropertyWindowSpecialItem::kEventId &&
                  payload == std::array<std::uint32_t, 2>{1u, 0u},
              "event target and payload");
        calls.emplace_back("event");
        return event_result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::PropertyWindowSpecialItem::UseMagicItem(
              0x1000u, 0x2000u, runtime) == 0x7fu,
          "event return forwarded");
    Check(runtime.calls == std::vector<std::string>{
              "hide", "clear", "inventory", "remove", "manager", "event"},
          "item-use operation order");
    runtime.calls.clear();
    runtime.event_result = 0u;
    Check(pal4::PropertyWindowSpecialItem::UseMagicItem(
              0x1000u, 0x2000u, runtime) == 0u,
          "false event result forwarded");
}
