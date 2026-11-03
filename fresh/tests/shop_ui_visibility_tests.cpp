#include "pal4/shop_ui_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiVisibilityRuntime {
    std::vector<std::string> calls;
    std::uint32_t manager_calls = 0u;
    std::uint32_t frame = 0u;
    std::uint8_t visible = 0u;
    std::uint32_t parent = 0u;
    std::uint32_t list = 0u;
    std::uint32_t item = 0u;
    std::uint32_t visibility_manager = 0u;
    std::uint32_t state = 0u;

    void Update(const std::uint32_t value) override {
        calls.emplace_back("update");
        frame = value;
    }
    void SetParent(const std::uint32_t value, const std::uint8_t flag,
                   const std::uint32_t parent_value) override {
        calls.emplace_back("parent");
        frame = value;
        visible = flag;
        parent = parent_value;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        ++manager_calls;
        return manager_calls == 1u ? 0x7000u : 0x8000u;
    }
    void InsertActive(const std::uint32_t list_value,
                      const std::uint32_t item_value) override {
        calls.emplace_back("insert");
        list = list_value;
        item = item_value;
    }
    void RemoveActive(const std::uint32_t list_value,
                      const std::uint32_t item_value) override {
        calls.emplace_back("remove");
        list = list_value;
        item = item_value;
    }
    void UpdateSystemDialogVisibility(const std::uint32_t manager_value,
                                      const std::uint32_t state_value) override {
        calls.emplace_back("system visibility");
        visibility_manager = manager_value;
        state = state_value;
    }
};

void TestVisible() {
    Runtime runtime;
    pal4::ShopUiVisibility::SetVisible(0x5000u, 2u, 0x6000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "update", "parent", "manager", "insert", "manager",
              "system visibility"} &&
              runtime.frame == 0x5000u && runtime.visible == 2u &&
              runtime.parent == 0x6000u && runtime.list == 0x7134u &&
              runtime.item == 0x5028u &&
              runtime.visibility_manager == 0x8000u && runtime.state == 8u,
          "visible frame update, parent, active list, and system state");
}

void TestHidden() {
    Runtime runtime;
    pal4::ShopUiVisibility::SetVisible(0x5000u, 0u, 0x6000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent", "manager", "remove"} &&
              runtime.manager_calls == 1u && runtime.frame == 0x5000u &&
              runtime.visible == 0u && runtime.parent == 0x6000u &&
              runtime.list == 0x7134u && runtime.item == 0x5028u,
          "hidden frame removes the active item after parent update");
}

void TestNullFrame() {
    Runtime runtime;
    pal4::ShopUiVisibility::SetVisible(0u, 1u, 0u, runtime);
    Check(runtime.item == 0u && runtime.list == 0x7134u &&
              runtime.manager_calls == 2u,
          "null frame passes null active item");
}

}  // namespace

int main() {
    TestVisible();
    TestHidden();
    TestNullFrame();
    return EXIT_SUCCESS;
}
