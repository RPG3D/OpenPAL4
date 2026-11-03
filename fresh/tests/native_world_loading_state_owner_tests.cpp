#include "pal4/native_world_loading_state_owner.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
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

std::uint32_t ReadWord(std::uint32_t address) {
    return *reinterpret_cast<const std::uint32_t*>(
        static_cast<std::uintptr_t>(address));
}

struct LoadingRuntime final : pal4::IWorldLoadingConfigRuntime,
                              pal4::IWorldLoadingFlowRuntime {
    pal4::NativeWorldLoadingStateOwner& owner;
    std::vector<std::string> calls;
    bool visible = false;

    explicit LoadingRuntime(pal4::NativeWorldLoadingStateOwner& state)
        : owner(state) {}
    std::uint32_t GameState() override { return 0x4000u; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (address == 0x4000u + 40u) {
            Check(value == 0u, "loading clears game state flag");
            return;
        }
        Check(address >= owner.Address() &&
                  address < owner.Address() + 56u,
              "loading write stays in owned state");
        *reinterpret_cast<std::uint32_t*>(
            static_cast<std::uintptr_t>(address)) = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address >= owner.Address() &&
                  address < owner.Address() + 56u,
              "loading read stays in owned state");
        return ::ReadWord(address);
    }
    std::uint32_t CameraManager() override { return 0x1000u; }
    std::uint32_t DefaultCameraName() override { return 0x2000u; }
    void ActivateCamera(std::uint32_t manager,
                        std::uint32_t name) override {
        Check(manager == 0x1000u && name == 0x2000u,
              "loading activates default camera");
        calls.emplace_back("camera");
    }
    std::uint32_t UIFrameManager() override { return 0x3000u; }
    std::uint32_t LoadingWindow(std::uint32_t manager) override {
        Check(manager == 0x3000u, "loading uses UI manager");
        return 0x3004u;
    }
    void SetLoadingWindowVisible(std::uint32_t window,
                                 bool show) override {
        Check(window == 0x3004u, "loading uses loading window");
        visible = show;
        calls.emplace_back(show ? "show" : "hide");
    }
    std::optional<std::uint32_t> ReadType(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x6000u && key == "PALGAMEIV_LOADING_TYPE",
              "loading reads type from config");
        return 0u;
    }
    std::optional<std::string> ReadString(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x6000u, "loading reads matching config");
        if (key == "PALGAMEIV_LOADING_ARENA") return "Q01";
        if (key == "PALGAMEIV_LOADING_AREA") return "N01";
        if (key == "PALGAMEIV_LOADING_AREA_RES") return "";
        Check(false, "loading uses known config key");
        return std::nullopt;
    }
    void AssignString(std::uint32_t destination,
                      std::string_view value) override {
        owner.AssignString(destination - owner.Address(), value);
    }
    std::optional<std::uint32_t> ReadTreeInteger(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x6000u &&
                  key == "PALGAMEIV_LOADING_AUTOREADY",
              "loading reads optional ready flag");
        return 0u;
    }
    std::uint32_t RenderUI(std::uint32_t, std::uint32_t) override {
        return 1u;
    }
    void LoadSave(std::uint32_t) override {
        Check(false, "new game must not load save slot");
    }
    std::uint32_t GameInstance() override { return 0x7000u; }
    std::uint32_t InitializeStateAndUI(
        std::uint32_t game, std::uint32_t state) override {
        Check(game == 0x7000u && state == 1u,
              "loading enters Scene state");
        calls.emplace_back("scene");
        return 1u;
    }
    std::uint32_t ArenaManager() override { return 0x5000u; }
    void LoadSceneResources(std::uint32_t arena,
                            std::uint32_t scene,
                            std::uint32_t area) override {
        Check(arena == 0x5000u &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(scene))) == "Q01" &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(area))) == "N01",
              "native Loading string pointers reach resource loader");
        calls.emplace_back("resources");
    }
    void LoadSceneRuntime(std::uint32_t arena,
                          std::uint32_t resources) override {
        Check(arena == 0x5000u &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(resources))).empty(),
              "native Loading resource suffix reaches runtime loader");
        calls.emplace_back("runtime");
    }
    void InitializePlayerScene(std::uint32_t arena) override {
        Check(arena == 0x5000u, "loading uses arena for player scene");
        calls.emplace_back("player");
    }
};

}  // namespace

int main() {
    pal4::NativeWorldLoadingStateOwner loading;
    const auto object = loading.Construct();
    Check(object != 0u && loading.Address() == object &&
              ReadWord(object) == pal4::WorldLoadingState::kVtable &&
              ReadWord(object + 4u) == 0u &&
              ReadWord(object + 8u) == 0u &&
              ReadWord(object + 48u) == 0u &&
              ReadWord(object + 52u) == 0u &&
              loading.OutstandingAllocations() == 3u,
          "native Loading state constructs the original 56-byte image");
    for (const auto offset : {12u, 24u, 36u}) {
        Check(ReadWord(object + offset) != 0u &&
                  ReadWord(object + offset + 4u) == 0u &&
                  ReadWord(object + offset + 8u) == 32u &&
                  loading.String(offset).empty(),
              "three owned native string records begin empty");
    }
    loading.AssignString(12u, "Q01");
    loading.AssignString(24u, "N01");
    loading.AssignString(36u, "N01/N01_scene_resources_long_name");
    Check(loading.String(12u) == "Q01" &&
              loading.String(24u) == "N01" &&
              loading.String(36u) ==
                  "N01/N01_scene_resources_long_name" &&
              ReadWord(object + 36u + 8u) > 32u &&
              loading.OutstandingAllocations() == 3u,
          "Loading request replaces text and grows its third buffer");
    loading.AssignString(36u, "");
    Check(loading.String(36u).empty() &&
              loading.OutstandingAllocations() == 3u,
          "Loading text can be reset without leaking its buffer");
    LoadingRuntime runtime(loading);
    Check(!loading.Enter(13u, 0u, runtime) && runtime.calls.empty(),
          "null config cannot enter Loading state");
    Check(loading.Enter(13u, 0x6000u, runtime) && runtime.visible &&
              loading.String(12u) == "Q01" &&
              loading.String(24u) == "N01" &&
              loading.String(36u).empty() &&
              ReadWord(object + 4u) == 0u &&
              loading.OutstandingAllocations() == 3u,
          "native Loading entry assigns the original scene request");
    Check(loading.Advance(runtime) == 0u &&
              ReadWord(object + 4u) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "camera", "show", "resources", "runtime", "player",
                  "scene"},
          "Loading advances resource, runtime, player and Scene stages");
    Check(loading.Advance(runtime) == 1u &&
              runtime.calls.size() == 6u &&
              loading.Hide(runtime) == 1u && !runtime.visible &&
              runtime.calls.back() == "hide",
          "Loading does not repeat a completed scene load");
    try {
        (void)loading.Construct();
        Check(false, "duplicate construction must reject");
    } catch (const std::logic_error&) {
    }
    try {
        loading.AssignString(8u, "invalid");
        Check(false, "non-string field must reject assignment");
    } catch (const std::out_of_range&) {
    }
    loading.Destroy();
    try {
        (void)loading.Advance(runtime);
        Check(false, "destroyed Loading state must reject advance");
    } catch (const std::logic_error&) {
    }
    Check(loading.Address() == 0u &&
              loading.OutstandingAllocations() == 0u,
          "Loading state destroys all three string allocations");
    Check(loading.Construct() != 0u &&
              loading.String(12u).empty(),
          "Loading state can be reconstructed after teardown");
    loading.Destroy();
    return EXIT_SUCCESS;
}
