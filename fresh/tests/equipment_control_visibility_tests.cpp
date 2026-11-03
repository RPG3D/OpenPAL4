#include "pal4/equipment_control_visibility.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentControlVisibilityRuntimeStub {
    std::vector<std::string> calls;

    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back("visible:" + std::to_string(window) + ":" +
                           (visible ? "1" : "0"));
    }
    std::uint32_t IconImage(std::uint32_t index, std::uint32_t variant,
                            std::uint32_t flags) override {
        Check(index < 4u && variant == 0u && flags == 0u,
              "icon lookup parameters");
        calls.emplace_back("icon:" + std::to_string(index));
        return 0x2000u + index;
    }
    void SetImage(std::uint32_t window, std::uint32_t image) override {
        calls.emplace_back("image:" + std::to_string(window) + ":" +
                           std::to_string(image));
    }
    std::uint32_t GetImage(std::string_view imageset,
                            std::string_view image) override {
        Check(imageset == "zi0" && image == "zhuangbei",
              "hidden-state image resource");
        calls.emplace_back("hidden-image");
        return 0x3000u;
    }
    void SetModelVisible(pal4::EquipmentWindowState&, bool visible) override {
        calls.emplace_back(visible ? "model:1" : "model:0");
    }
};

pal4::EquipmentWindowState MakeState(std::uint32_t category) {
    pal4::EquipmentWindowState state{};
    state.Write(0xbcu, 1u);
    state.Write(0xb8u, 2u);
    state.Write(0xc0u, 3u);
    state.Write(0xc4u, 4u);
    state.Write(0xb4u, 5u);
    for (std::uint32_t index = 0u; index < 4u; ++index)
        state.Write(0xe0u + index * 4u, 10u + index);
    state.Write(0xb0u, 20u);
    state.Write(0xf0u, category);
    state.Write(0xc8u + 4u * category, 0x4000u + category);
    state.Write(0xdcu, 0x5000u);
    return state;
}

void TestShow(std::uint32_t category, std::uint32_t image) {
    auto state = MakeState(category);
    Runtime runtime;
    pal4::EquipmentControlVisibility::SetVisible(state, true, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "visible:1:1", "visible:2:1", "visible:3:1",
              "visible:4:1", "visible:5:1", "visible:10:0", "icon:0",
              "image:10:8192", "visible:11:0", "icon:1",
              "image:11:8193", "visible:12:0", "icon:2",
              "image:12:8194", "visible:13:0", "icon:3",
              "image:13:8195", "image:20:" + std::to_string(image),
              "model:1"}, "show control and icon order");
}

void TestHide() {
    auto state = MakeState(2u);
    Runtime runtime;
    pal4::EquipmentControlVisibility::SetVisible(state, false, runtime);
    Check(runtime.calls.size() == 20u, "hide executes all control operations");
    Check(runtime.calls[0] == "visible:1:0" &&
              runtime.calls[4] == "visible:5:0" &&
              runtime.calls[17] == "hidden-image" &&
              runtime.calls[18] == "image:20:12288" &&
              runtime.calls[19] == "model:0",
          "hide uses zi0/zhuangbei then hides model");
}

}  // namespace

int main() {
    TestShow(2u, 0x4002u);
    TestShow(5u, 0x5000u);
    TestHide();
}
