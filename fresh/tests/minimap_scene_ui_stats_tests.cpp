#include "pal4/minimap_scene_ui_stats.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMinimapSceneUiStatsRuntime {
    std::vector<std::string> calls;
    std::vector<double> ftol_inputs;
    std::vector<std::string> names;
    std::vector<std::uint32_t> changed_windows;
    std::array<std::int32_t, 2> ftol_results{8, 7};
    std::int32_t hp = 75;
    std::int32_t max_hp = 100;
    std::int32_t mp = 70;
    std::int32_t mode = 1;
    int mode_calls = 0;
    int ftol_calls = 0;
    bool flash_flag = false;
    bool throw_second_image = false;
    pal4::MinimapFlashColor color{};

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x9000u + 0x2c50u,
              "current player index address");
        calls.emplace_back("index");
        return 1u;
    }
    void InitializeNameBuffer() override {
        calls.emplace_back("name-init");
    }
    std::int32_t PlayerStat(std::uint32_t state,
                            std::uint32_t index) override {
        Check(state == 0x9000u + 2836u + 0x654u,
              "current player stat subobject address");
        calls.push_back(index == 7u ? "hp-stat" : "mp-stat");
        Check(index == 7u || index == 8u, "stat selector");
        return index == 7u ? hp : mp;
    }
    std::int32_t PlayerMaxStat(std::uint32_t state,
                               std::uint32_t index) override {
        Check(state == 0x9000u + 2836u + 0x654u && index == 0u,
              "maximum HP virtual call selector");
        calls.emplace_back("max-hp");
        return max_hp;
    }
    std::int32_t FloatToLong(double value) override {
        ftol_inputs.push_back(value);
        calls.emplace_back("ftol");
        return ftol_results[static_cast<std::size_t>(ftol_calls++)];
    }
    std::string FormatName(std::string_view format,
                           std::int32_t value) override {
        Check(format == "hp%d" || format == "mp%d",
              "stat image format string");
        const auto name = std::string(format.substr(0u, 2u)) +
                          std::to_string(value);
        names.push_back(name);
        calls.emplace_back("format");
        return name;
    }
    std::uint32_t GetImage(std::string_view imageset,
                           std::string_view name) override {
        Check(imageset == "zhujiemian0" &&
                  (name == names[0] || name == names.back()),
              "imageset and formatted image name");
        calls.emplace_back("image");
        if (throw_second_image && names.size() == 2u)
            throw std::runtime_error("image");
        return names.size() == 1u ? 0xa000u : 0xb000u;
    }
    std::uint32_t CurrentImage(std::uint32_t window) override {
        Check(window == 0x1000u || window == 0x2000u,
              "HP/MP image window");
        calls.emplace_back("current-image");
        return window == 0x1000u ? 0u : 0xb000u;
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t image) override {
        Check(window == 0x1000u && image == 0xa000u,
              "only changed HP image is replaced");
        changed_windows.push_back(window);
        calls.emplace_back("set-image");
    }
    std::int32_t PlayerMode(std::uint32_t state) override {
        Check(state == 0x9000u + 2836u + 0x654u,
              "player mode subobject");
        ++mode_calls;
        calls.emplace_back("mode");
        return mode;
    }
    bool FlashFlag() override {
        calls.emplace_back("flash-read");
        return flash_flag;
    }
    void SetFlashFlag(bool value) override {
        flash_flag = value;
        calls.emplace_back("flash-write");
    }
    void ApplyFlashColor(std::uint32_t window,
                         pal4::MinimapFlashColor value) override {
        Check(window == 0x3000u, "flashing system button window");
        color = value;
        calls.emplace_back("color");
    }
    void RequestRedraw(std::uint32_t window) override {
        Check(window == 0x3000u, "flashing redraw window");
        calls.emplace_back("redraw");
    }
    std::uint32_t CleanupNameBuffer() override {
        calls.emplace_back("cleanup");
        return 0x77u;
    }
};

pal4::MinimapSceneUiState MakeState() {
    pal4::MinimapSceneUiState state;
    state.WriteWord(0xc4u, 0x1000u);
    state.WriteWord(0xc8u, 0x2000u);
    state.WriteWord(0xb4u, 0x3000u);
    return state;
}

std::size_t Position(const Runtime& runtime, const std::string& name) {
    for (std::size_t index = 0u; index < runtime.calls.size(); ++index) {
        if (runtime.calls[index] == name) return index;
    }
    return runtime.calls.size();
}

void CheckNormal() {
    const auto state = MakeState();
    Runtime runtime;
    Check(pal4::MinimapSceneUiStats::Update(state, runtime) == 0x77u &&
              runtime.names == std::vector<std::string>{"hp8", "mp7"} &&
              runtime.ftol_inputs.size() == 2u &&
              runtime.ftol_inputs[0] == 7.5 &&
              runtime.ftol_inputs[1] > 7.0 &&
              runtime.ftol_inputs[1] < 7.1 &&
              runtime.changed_windows == std::vector<std::uint32_t>{0x1000u},
          "HP ratio and MP tenth select images, unchanged MP image skipped");
    Check(runtime.mode_calls == 1 && runtime.flash_flag &&
              runtime.color.red == 0.57647061f &&
              runtime.color.green == 0.56862748f &&
              runtime.color.blue == 0.79215688f &&
              runtime.color.alpha == 1.0f &&
              Position(runtime, "redraw") < Position(runtime, "cleanup"),
          "mode one starts dark flash, redraws, then cleans name buffer");
}

void CheckOtherModesAndClamps() {
    const auto state = MakeState();
    Runtime bright;
    bright.mode = 2;
    bright.flash_flag = true;
    bright.ftol_results = {12, 15};
    (void)pal4::MinimapSceneUiStats::Update(state, bright);
    Check(bright.names == std::vector<std::string>{"hp9", "mp9"} &&
              bright.mode_calls == 2 && !bright.flash_flag &&
              bright.color.red == 0.81960785f &&
              bright.color.green == 0.80784315f &&
              bright.color.blue == 0.9137255f,
          "mode two rechecks mode, clamps high digits and uses light flash");

    Runtime ordinary;
    ordinary.mode = 3;
    ordinary.ftol_results = {-2, -3};
    (void)pal4::MinimapSceneUiStats::Update(state, ordinary);
    Check(ordinary.names == std::vector<std::string>{"hp-2", "mp-3"} &&
              ordinary.mode_calls == 2 &&
              Position(ordinary, "redraw") == ordinary.calls.size() &&
              ordinary.calls.back() == "cleanup",
          "negative digits are not clamped and other modes skip flash");
}

void CheckExceptionCleanup() {
    const auto state = MakeState();
    Runtime runtime;
    runtime.throw_second_image = true;
    bool caught = false;
    try {
        (void)pal4::MinimapSceneUiStats::Update(state, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.names.size() == 2u &&
              runtime.calls.back() == "cleanup" &&
              Position(runtime, "mode") == runtime.calls.size(),
          "image lookup failure still cleans shared format buffer");
}

}  // namespace

int main() {
    CheckNormal();
    CheckOtherModesAndClamps();
    CheckExceptionCleanup();
}
