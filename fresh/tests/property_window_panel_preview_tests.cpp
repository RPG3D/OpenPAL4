#include "pal4/property_window_panel_preview.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowPanelPreviewRuntime {
    std::vector<std::string> calls;
    std::uint32_t existing = 0u;
    std::uint32_t created = 0x9000u;
    std::uint32_t component = 1u;

    void SetVisible(std::uint32_t window, std::uint32_t value) override {
        calls.push_back("visible:" + std::to_string(window) + ":" +
                        std::to_string(value));
    }
    void SetSelected(std::uint32_t button, bool selected) override {
        Check(!selected, "hidden panel clears all six category buttons");
        calls.push_back("selected:" + std::to_string(button));
    }
    std::uint32_t Image(std::string_view imageset,
                        std::string_view image) override {
        Check(imageset == "zi0" && image == "wupin",
              "hidden panel loads source category image");
        calls.emplace_back("image");
        return 0xa000u;
    }
    void SetImage(std::uint32_t window, std::uint32_t image) override {
        Check(window == 0x6800u && image == 0xa000u,
              "category image targets frame +0x68");
        calls.emplace_back("set-image");
    }
    std::uint32_t GameObjectCollection() override {
        calls.emplace_back("collection");
        return 0xb000u;
    }
    std::uint32_t FindObject(std::uint32_t collection,
                             std::uint32_t id) override {
        Check(collection == 0xb000u, "model lookup uses UI collection");
        calls.push_back("find:" + std::to_string(id));
        return existing;
    }
    pal4::PropertyEffectHandle CreateEffect(
        std::string_view name, const std::array<float, 3>& position,
        std::uint32_t sentinel, std::uint32_t flag) override {
        Check(name == "LH" && sentinel == 0xffffffffu && flag == 0u &&
                  std::bit_cast<std::uint32_t>(position[0]) == 0xc26eae14u &&
                  std::bit_cast<std::uint32_t>(position[1]) == 0x4348dc29u &&
                  std::bit_cast<std::uint32_t>(position[2]) == 0x43952148u,
              "effect creation preserves name, sentinels and position bits");
        calls.emplace_back("effect-create");
        return {0x1111u, 0x2222u};
    }
    void ResetTemporaryEffect() override {
        calls.emplace_back("effect-reset");
    }
    void UpdateEffectProperties(
        pal4::PropertyEffectHandle handle,
        const std::array<float, 3>& scale,
        std::uint8_t flag) override {
        Check(handle.first == 0x1111u && handle.second == 0x2222u &&
                  scale == std::array<float, 3>{2.2f, 2.2f, 2.2f} &&
                  flag == 0u,
              "effect update receives stored handle, scale and cleared flag");
        calls.emplace_back("effect-scale");
    }
    void MarkEffectForUpdate(pal4::PropertyEffectHandle handle) override {
        Check(handle.first == 0x1111u && handle.second == 0x2222u,
              "effect update uses original handle");
        calls.emplace_back("effect-dirty");
    }
    void PrepareScripts() override { calls.emplace_back("prepare"); }
    std::uint32_t CreateObject(std::uint32_t collection,
                               std::string_view directory,
                               std::string_view name,
                               std::uint32_t flag) override {
        Check(collection == 0xb000u && directory == "ui\\model\\" &&
                  name == "_ui_flower" && flag == 1u,
              "flower model uses source resource names and creation flag");
        calls.emplace_back("create-object");
        return created;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == 0x9014u) return component;
        if (address == 0x9170u) return 0x3333u;
        Check(false, "unexpected object field address");
        return 0u;
    }
    void SetScale(std::uint32_t object,
                  const std::array<float, 3>& value) override {
        Check(object == 0x9000u &&
                  value == std::array<float, 3>{3.5f, 3.5f, 3.5f},
              "flower object scale is 3.5 on every axis");
        calls.emplace_back("object-scale");
    }
    void SetPosition(std::uint32_t object,
                     const std::array<float, 3>& value) override {
        Check(object == 0x9000u &&
                  std::bit_cast<std::uint32_t>(value[0]) == 0xc26eae14u &&
                  std::bit_cast<std::uint32_t>(value[1]) == 0x439aee14u &&
                  std::bit_cast<std::uint32_t>(value[2]) == 0x43952148u,
              "flower object position preserves source float bits");
        calls.emplace_back("object-position");
    }
    void CleanupScripts() override { calls.emplace_back("cleanup"); }
    void RemoveObject(std::uint32_t collection,
                      std::uint32_t id) override {
        Check(collection == 0xb000u, "hide removes from UI collection");
        calls.push_back("remove:" + std::to_string(id));
    }
    void HandleEffectEvent(pal4::PropertyEffectHandle handle,
                           std::uint32_t event,
                           std::uint32_t flag) override {
        Check(handle.first == 0x1111u && handle.second == 0x2222u &&
                  event == 0xc000u && flag == 0u,
              "hide forwards effect handle and event with zero flag");
        calls.emplace_back("effect-event");
    }
};

void SetupState(pal4::PropertyWindowState& state) {
    for (std::uint32_t index = 0u; index < 6u; ++index)
        state.Write(0x40u + index * 4u, 0x4000u + index);
    state.Write(0x68u, 0x6800u);
    state.Write(0x8cu, 0x8c00u);
    state.Write(0x90u, 0x9000u);
    state.Write(0x94u, 0x9400u);
    state.Write(0x78u, 0x7777u);
    state.Write(0x7cu, 0x4444u);
    state.Write(0x80u, 0x5555u);
    state.Write(0x84u, 0x1111u);
    state.Write(0x88u, 0x2222u);
}

void TestShowAndReuse() {
    pal4::PropertyWindowState state;
    SetupState(state);
    Runtime runtime;
    runtime.existing = 0x9000u;
    pal4::PropertyWindowPanelPreview::SetItemPanel(
        state, 2u, 0xc000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "visible:35840:2", "visible:36864:2", "visible:37888:2",
              "collection", "find:17476"},
          "nonzero show updates three controls then reuses existing model");

    runtime.calls.clear();
    runtime.existing = 0u;
    pal4::PropertyWindowPanelPreview::ToggleModel(
        state, 1u, 0xc000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "collection", "find:17476", "effect-create", "effect-reset",
              "effect-scale", "effect-dirty", "prepare", "create-object",
              "read:36884", "read:37232", "object-scale",
              "object-position", "cleanup"},
          "new model builds effect and flower object in original order");
    Check(state.Read(0x7cu) == 0x3333u &&
          state.Read(0x84u) == 0x1111u && state.Read(0x88u) == 0x2222u,
          "new effect and model IDs are cached at observed offsets");
}

void TestMissingFlowerAndHide() {
    pal4::PropertyWindowState state;
    SetupState(state);
    Runtime runtime;
    runtime.created = 0u;
    pal4::PropertyWindowPanelPreview::ToggleModel(
        state, 1u, 0xc000u, runtime);
    Check(runtime.calls.back() == "cleanup" &&
          state.Read(0x7cu) == 0x4444u,
          "null flower creation cleans resources without changing model ID");

    runtime.calls.clear();
    pal4::PropertyWindowPanelPreview::SetItemPanel(
        state, 0u, 0xc000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "visible:35840:0", "visible:36864:0", "visible:37888:0",
              "selected:16384", "selected:16385", "selected:16386",
              "selected:16387", "selected:16388", "selected:16389",
              "image", "set-image", "collection", "remove:17476",
              "remove:21845", "remove:30583", "effect-event"},
          "hide clears controls, image, three models and effect in order");
    Check(state.Read(0x7cu) == 0u && state.Read(0x80u) == 0u &&
          state.Read(0x78u) == 0u,
          "hide clears three cached object IDs");

    runtime.calls.clear();
    SetupState(state);
    pal4::PropertyWindowPanelPreview::SetItemPanel(
        state, 0x100u, 0xc000u, runtime);
    Check(runtime.calls[0] == "visible:35840:256" &&
          runtime.calls[3] == "selected:16384" &&
          runtime.calls.back() == "effect-event",
          "high-byte-only value reaches CEGUI but takes low-byte hide path");
}

}  // namespace

int main() {
    TestShowAndReuse();
    TestMissingFlowerAndHide();
}
