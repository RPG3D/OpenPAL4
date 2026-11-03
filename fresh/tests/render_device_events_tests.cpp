#include "pal4/render_device_events.h"

#include <cstdint>
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

class Runtime final : public pal4::IRenderDeviceEventRuntime {
public:
    std::int32_t TestCooperativeLevel(const std::uint32_t device) override {
        events.emplace_back("test");
        Check(device == 0x5000u, "cooperative call receives state device");
        return cooperative_result;
    }
    std::int32_t Present(const std::uint32_t device,
                          const std::uint32_t window) override {
        events.emplace_back("present");
        Check(device == 0x5000u && window == 0x6000u,
              "Present receives device and game window");
        return present_result;
    }
    bool HasLostCallback() override {
        events.emplace_back("has-lost");
        return has_lost;
    }
    std::uint32_t InvokeLostCallback() override {
        events.emplace_back("lost");
        return 0x12345678u;
    }
    bool HasRestoredCallback() override {
        events.emplace_back("has-restored");
        return has_restored;
    }
    void InvokeRestoredCallback() override {
        events.emplace_back("restored");
    }
    void ChangeVideoMode(const std::uint32_t mode) override {
        events.emplace_back("change-mode");
        Check(mode == 7u, "reset uses stored video mode");
    }

    std::int32_t cooperative_result = 0;
    std::int32_t present_result = 0;
    bool has_lost = true;
    bool has_restored = true;
    std::vector<std::string> events;
};

struct Fixture {
    pal4::GameRenderState state{0x9000u};
    Runtime runtime;

    Fixture() {
        state.Write(0u, 0x6000u);
        state.Write(0x538u, 7u);
        state.Write(0x560u, 0x5000u);
    }
};

void VideoModeChangePaths() {
    Fixture fixture;
    pal4::RenderDeviceEvents handler(fixture.state);
    Check(handler.HandleVideoModeChange(fixture.runtime) == 1u &&
              fixture.runtime.events == std::vector<std::string>{"test"},
          "cooperative success returns one without callbacks");

    fixture.runtime.events.clear();
    fixture.runtime.cooperative_result = -1;
    Check(handler.HandleVideoModeChange(fixture.runtime) == 1u &&
              fixture.runtime.events == std::vector<std::string>{"test"},
          "other negative result returns one without recovery");

    fixture.runtime.events.clear();
    fixture.runtime.cooperative_result =
        static_cast<std::int32_t>(0x88760868u);
    Check(handler.HandleVideoModeChange(fixture.runtime) == 0u &&
              fixture.runtime.events ==
                  std::vector<std::string>{"test", "has-lost", "lost"},
          "DEVICELOST invokes loss callback and returns zero");
    fixture.runtime.events.clear();
    fixture.runtime.has_lost = false;
    Check(handler.HandleVideoModeChange(fixture.runtime) == 0u &&
              fixture.runtime.events ==
                  std::vector<std::string>{"test", "has-lost"},
          "DEVICELOST without callback still returns zero");

    fixture.runtime.events.clear();
    fixture.runtime.cooperative_result =
        static_cast<std::int32_t>(0x88760869u);
    Check(handler.HandleVideoModeChange(fixture.runtime) == 1u &&
              fixture.runtime.events ==
                  std::vector<std::string>{"test", "has-lost",
                                           "change-mode", "has-restored",
                                           "restored"},
          "DEVICENOTRESET changes mode then invokes restore callback");
    fixture.runtime.events.clear();
    fixture.runtime.has_lost = true;
    fixture.runtime.has_restored = false;
    Check(handler.HandleVideoModeChange(fixture.runtime) == 1u &&
              fixture.runtime.events ==
                  std::vector<std::string>{"test", "has-lost", "lost",
                                           "change-mode", "has-restored"},
          "DEVICENOTRESET respects missing restore callback");
}

void PresentPaths() {
    Fixture fixture;
    pal4::RenderDeviceEvents handler(fixture.state);
    fixture.runtime.present_result = -2;
    Check(handler.HandleDeviceLoss(fixture.runtime) == -2 &&
              fixture.runtime.events ==
                  std::vector<std::string>{"present"},
          "ordinary Present error returns unchanged");

    for (const std::uint32_t code : {0x88760868u, 0x88760827u}) {
        fixture.runtime.events.clear();
        fixture.runtime.present_result = static_cast<std::int32_t>(code);
        Check(handler.HandleDeviceLoss(fixture.runtime) ==
                  static_cast<std::int32_t>(0x12345678u) &&
                  fixture.runtime.events ==
                      std::vector<std::string>{"present", "has-lost",
                                               "lost"},
              "loss Present code tail-forwards callback result");
    }
    fixture.runtime.events.clear();
    fixture.runtime.has_lost = false;
    Check(handler.HandleDeviceLoss(fixture.runtime) == 0 &&
              fixture.runtime.events ==
                  std::vector<std::string>{"present", "has-lost"},
          "loss Present code without callback returns zero");
}

}  // namespace

int main() {
    VideoModeChangePaths();
    PresentPaths();
    std::cout << "10 render device event paths passed\n";
}
