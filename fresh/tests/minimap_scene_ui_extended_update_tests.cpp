#include "pal4/minimap_scene_ui_extended_update.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMinimapSceneUiExtendedRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x9008u, 0x111u}, {0x900cu, 0x222u},
        {0x90e0u, 0x333u}, {0xb060u, 0xc000u}};
    std::vector<std::uint32_t> matching_indices;
    std::vector<std::uint32_t> scanned_rows;
    std::vector<double> rounding_inputs;
    std::vector<std::uint32_t> releases;
    std::vector<std::uint32_t> count_plan;
    std::uint32_t row_count = 3u;
    std::uint32_t mode_written = 0xffffffffu;
    std::uint32_t flag_written = 0xffffffffu;
    std::uint32_t displayed_text = 0u;
    std::uint32_t displayed_window = 0u;
    std::uint32_t trigger_releases = 0u;
    std::int32_t screen_width = 800;
    std::int32_t screen_height = 600;
    std::int32_t texture_x = 0;
    std::int32_t texture_y = 0;
    std::int32_t texture_size = 0;
    std::uint32_t count_calls = 0u;
    bool throw_compare = false;
    bool throw_text = false;
    bool throw_texture = false;
    bool throw_second_copy = false;
    std::uint32_t copy_calls = 0u;

    Runtime() {
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            words[0xc000u + 60u * index + 40u] = 0xe000u + index;
            words[0xc000u + 60u * index + 52u] = 1u;
        }
    }

    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x9000u;
    }
    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap");
        return 0xa000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0xa050u, "minimap mode field offset");
        mode_written = value;
        calls.emplace_back("write-mode");
    }
    void LoadScriptData(std::uint32_t minimap,
                        std::uint32_t path,
                        std::uint32_t script) override {
        Check(minimap == 0xa000u && path == 0x111u && script == 0x222u,
              "arena script paths and minimap manager");
        calls.emplace_back("load-script");
    }
    void InitializeDialogSystem() override {
        calls.emplace_back("dialog-init");
    }
    pal4::MinimapSceneTriggerVariable QueryTriggerVariable(
        std::uint32_t path, std::uint32_t name) override {
        Check(path == 0x111u && name == 0x333u,
              "trigger-variable arena path and variable name");
        calls.emplace_back("trigger-query");
        return {0xd000u, 0xd100u};
    }
    std::uint32_t CopyFloorAttribute(std::uint32_t source) override {
        Check(source == 0xd000u || source == 0xd100u,
              "trigger attribute copy source");
        calls.emplace_back("copy-attribute");
        ++copy_calls;
        if (throw_second_copy && copy_calls == 2u)
            throw std::runtime_error("copy");
        return source + 0x1000u;
    }
    void ReleaseFloorAttribute(std::uint32_t copy) override {
        Check(copy == 0xe000u || copy == 0xe100u,
              "copied floor attribute cleanup");
        releases.push_back(copy);
        calls.emplace_back("release-attribute");
    }
    void ReleaseTriggerVariable(
        pal4::MinimapSceneTriggerVariable value) override {
        Check(value.primary == 0xd000u && value.secondary == 0xd100u,
              "trigger variable cleanup values");
        ++trigger_releases;
        calls.emplace_back("release-trigger");
    }
    std::uint32_t ItemPropertyTable() override {
        calls.emplace_back("item-table");
        return 0xb000u;
    }
    std::uint32_t ItemPropertyCount(
        std::uint32_t collection) override {
        Check(collection == 0xb05cu, "item table collection offset");
        ++count_calls;
        calls.emplace_back("count");
        if (count_calls <= count_plan.size())
            return count_plan[count_calls - 1u];
        return row_count;
    }
    bool MatchesFloorAttribute(std::uint32_t key,
                               std::uint32_t row) override {
        Check(key == 0xe000u && row >= 0xc000u &&
                  (row - 0xc000u) % 60u == 0u,
              "floor attribute key and 60-byte row stride");
        scanned_rows.push_back(row);
        calls.emplace_back("compare");
        if (throw_compare && scanned_rows.size() == 2u)
            throw std::runtime_error("compare");
        const auto index = (row - 0xc000u) / 60u;
        for (const auto matched : matching_indices) {
            if (matched == index) return true;
        }
        return false;
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty");
        return 0xf000u;
    }
    std::uint32_t ConvertToUtf8(std::uint32_t text) override {
        displayed_text = text;
        calls.emplace_back("utf8");
        return 0xf100u;
    }
    void SetWindowText(std::uint32_t window,
                       std::uint32_t utf8) override {
        Check(utf8 == 0xf100u, "converted UI text pointer");
        displayed_window = window;
        calls.emplace_back("set-text");
        if (throw_text) throw std::runtime_error("text");
    }
    void SetFlagAndUpdate(std::uint32_t owner,
                          std::uint8_t value) override {
        Check(owner == 0x5000u, "scene flag owner");
        flag_written = value;
        calls.emplace_back("set-flag");
    }
    void UpdateVisibilityAndMinimap(std::uint32_t owner) override {
        Check(owner == 0x5000u, "scene visibility owner");
        calls.emplace_back("visibility");
    }
    void UpdateVisibilityBasedOnState(std::uint32_t owner) override {
        Check(owner == 0x5000u, "linked visibility owner");
        calls.emplace_back("linked-visibility");
    }
    std::pair<std::int32_t, std::int32_t> ConfigSize() override {
        calls.emplace_back("config");
        return {screen_width, screen_height};
    }
    std::int32_t FloatToLong(double value) override {
        rounding_inputs.push_back(value);
        calls.emplace_back("ftol");
        return static_cast<std::int32_t>(std::nearbyint(value));
    }
    void SetupMinimapTexture(std::uint32_t minimap,
                             std::int32_t x, std::int32_t y,
                             std::int32_t width,
                             std::int32_t height) override {
        Check(minimap == 0xa000u && width == height,
              "texture manager and square dimensions");
        texture_x = x;
        texture_y = y;
        texture_size = width;
        calls.emplace_back("texture");
        if (throw_texture) throw std::runtime_error("texture");
    }
};

pal4::MinimapSceneUiState MakeState() {
    pal4::MinimapSceneUiState state;
    state.WriteWord(0xccu, 0xffffffffu);
    state.WriteWord(0x58u, 0x7000u);
    return state;
}

std::size_t Position(const Runtime& runtime, const std::string& call) {
    for (std::size_t index = 0u; index < runtime.calls.size(); ++index) {
        if (runtime.calls[index] == call) return index;
    }
    return runtime.calls.size();
}

void CheckFirstMatchAndModes() {
    for (const auto [mode, expected_map_mode, expected_flag] :
         {std::tuple{0u, 1u, 0u}, std::tuple{1u, 0u, 1u},
          std::tuple{2u, 2u, 0u},
          std::tuple{3u, 0xffffffffu, 0xffffffffu}}) {
        auto state = MakeState();
        Runtime runtime;
        runtime.matching_indices = {1u, 2u};
        runtime.words[0xc000u + 60u + 52u] = mode;
        Check(pal4::MinimapSceneUiExtendedUpdate::Update(
                  state, 0x5000u, runtime) == 1u &&
                  state.ReadWord(0xccu) == 0xc000u + 60u &&
                  runtime.scanned_rows == std::vector<std::uint32_t>{
                                              0xc000u, 0xc000u + 60u,
                                              0xc000u + 120u} &&
                  runtime.count_calls == 4u &&
                  runtime.displayed_window == 0x7000u &&
                  runtime.displayed_text == 0xe001u &&
                  runtime.mode_written == expected_map_mode &&
                  runtime.flag_written == expected_flag &&
                  runtime.texture_x == 2 && runtime.texture_y == 399 &&
                  runtime.texture_size == 173 &&
                  runtime.releases == std::vector<std::uint32_t>{
                                          0xe000u, 0xe100u} &&
                  runtime.trigger_releases == 1u,
              "all rows scanned and first match selects text/mode/texture");
        Check(Position(runtime, "load-script") <
                  Position(runtime, "dialog-init") &&
                  Position(runtime, "release-attribute") <
                  Position(runtime, "set-text") &&
                  Position(runtime, "set-text") <
                  Position(runtime, "visibility") &&
                  Position(runtime, "visibility") <
                  Position(runtime, "linked-visibility") &&
                  Position(runtime, "linked-visibility") <
                  Position(runtime, "texture") &&
                  runtime.calls.back() == "release-trigger",
              "scene load, text, visibility, texture and cleanup order");
    }
}

void CheckNoMatchAndScaledTexture() {
    auto state = MakeState();
    Runtime runtime;
    runtime.row_count = 0u;
    runtime.screen_width = 1280;
    runtime.screen_height = 720;
    Check(pal4::MinimapSceneUiExtendedUpdate::Update(
              state, 0x5000u, runtime) == 1u &&
              state.ReadWord(0xccu) == 0u && runtime.scanned_rows.empty() &&
              runtime.count_calls == 1u && runtime.mode_written == 2u &&
              Position(runtime, "set-text") == runtime.calls.size() &&
              Position(runtime, "visibility") == runtime.calls.size() &&
              runtime.rounding_inputs.size() == 3u &&
              runtime.rounding_inputs[0] > 276.7 &&
              runtime.rounding_inputs[0] < 276.9 &&
              runtime.rounding_inputs[1] > 398.3 &&
              runtime.rounding_inputs[1] < 398.5 &&
              runtime.rounding_inputs[2] > 3.1 &&
              runtime.rounding_inputs[2] < 3.3 &&
              runtime.texture_x == 3 && runtime.texture_y == 398 &&
              runtime.texture_size == 277,
          "no match selects mode two and scales texture from width 1280");

    auto changing_state = MakeState();
    Runtime changing;
    changing.count_plan = {3u, 1u};
    (void)pal4::MinimapSceneUiExtendedUpdate::Update(
        changing_state, 0x5000u, changing);
    Check(changing.scanned_rows == std::vector<std::uint32_t>{0xc000u} &&
              changing.count_calls == 2u && changing.mode_written == 2u,
          "property count is reread after each row and can shorten scan");
}

void CheckEmptyTextAndFailures() {
    auto state = MakeState();
    Runtime empty;
    empty.matching_indices = {0u};
    empty.words[0xc000u + 40u] = 0u;
    (void)pal4::MinimapSceneUiExtendedUpdate::Update(
        state, 0x5000u, empty);
    Check(empty.displayed_text == 0xf000u &&
              Position(empty, "empty") < Position(empty, "utf8"),
          "null selected text uses empty-string fallback");

    for (int failure = 0; failure < 3; ++failure) {
        auto failure_state = MakeState();
        Runtime runtime;
        runtime.matching_indices = {0u};
        runtime.throw_compare = failure == 0;
        runtime.throw_text = failure == 1;
        runtime.throw_texture = failure == 2;
        bool caught = false;
        try {
            (void)pal4::MinimapSceneUiExtendedUpdate::Update(
                failure_state, 0x5000u, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.releases ==
                            std::vector<std::uint32_t>{0xe000u, 0xe100u} &&
                  runtime.trigger_releases == 1u &&
                  runtime.calls.back() == "release-trigger",
              "failure releases both attribute copies and trigger variable");
    }

    Runtime copy_failure;
    copy_failure.throw_second_copy = true;
    bool caught = false;
    try {
        (void)pal4::MinimapSceneUiExtendedUpdate::Update(
            state, 0x5000u, copy_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && copy_failure.releases ==
                        std::vector<std::uint32_t>{0xe000u} &&
              copy_failure.trigger_releases == 1u &&
              copy_failure.calls.back() == "release-trigger",
          "second copy failure releases first copy and trigger variable");
}

}  // namespace

int main() {
    CheckFirstMatchAndModes();
    CheckNoMatchAndScaledTexture();
    CheckEmptyTextAndFailures();
}
