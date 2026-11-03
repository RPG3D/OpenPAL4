#include "pal4/combat_scene_frame.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSceneFrameRuntime,
                       pal4::ICombatSystemRenderRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool inactive = false;

    Runtime() {
        words[0x5000u] = 0x1234u;
        words[0x3000u + 424u] = 0x4000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("byte " + std::to_string(address));
        return bytes[address];
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat-system");
        return 0x3000u;
    }
    bool IsCombatSystemInactive(std::uint32_t system) override {
        Check(system == 0x3000u, "inactive check receives combat system");
        calls.emplace_back("inactive");
        return inactive;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x6000u;
    }
    void ProcessArenaFrame(std::uint32_t manager,
                           std::uint32_t frame_value) override {
        Check(manager == 0x6000u && frame_value == 0x1234u,
              "arena receives manager and dereferenced frame word");
        calls.emplace_back("process-arena");
    }
    void PlaySound(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    void RenderWorld(std::uint32_t frame) override {
        Check(frame == 0x5000u, "world receives frame pointer");
        calls.emplace_back("render-world");
    }
    void RenderGameObject(std::uint32_t object,
                          std::uint32_t frame) override {
        Check(object == 0x4000u && frame == 0x5000u,
              "object render receives combat game object");
        calls.emplace_back("render-object");
    }
    void RenderActors(std::uint32_t value) override {
        Check(value == 0x1234u, "actors receive frame word");
        calls.emplace_back("render-actors");
    }
    void RenderUi(std::uint32_t frame) override {
        Check(frame == 0x5000u, "UI receives frame pointer");
        calls.emplace_back("render-ui");
    }
    void RenderConsole(std::uint32_t value) override {
        Check(value == 0x1234u, "console receives frame word");
        calls.emplace_back("render-console");
    }
    std::uint32_t GetGameState() override { return 0u; }
    std::uint32_t CountEffects(std::uint32_t) override { return 0u; }
    std::string GetBgmName() override { return {}; }
    std::string ReadCString(std::uint32_t) override { return {}; }
    void DrawText(std::string_view, std::uint32_t, std::uint32_t,
                  std::uint32_t, std::uint32_t, std::uint32_t,
                  std::uint32_t, std::uint32_t) override {}
};

std::size_t Position(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    return found == runtime.calls.end()
               ? runtime.calls.size()
               : static_cast<std::size_t>(found - runtime.calls.begin());
}

void InactiveAndZeroState() {
    Runtime runtime;
    runtime.inactive = true;
    pal4::CombatSceneFrame::Render(0x5000u, runtime, runtime);
    Check(Position(runtime, "process-arena") < Position(runtime, "render-ui") &&
          Position(runtime, "render-ui") < Position(runtime, "render-console") &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "combat-system") == 2,
          "inactive system advances arena before reconstructed combat renderer");
    runtime.calls.clear();
    runtime.inactive = false;
    pal4::CombatSceneFrame::Render(0x5000u, runtime, runtime);
    Check(Position(runtime, "process-arena") < Position(runtime, "render-ui") &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "combat-system") == 3,
          "active zero state advances arena after reading combat state");
}

void TypedRenderer() {
    Runtime runtime;
    class Renderer final : public pal4::ICombatSystemFrameRenderer {
        Runtime& runtime;
    public:
        explicit Renderer(Runtime& r):runtime(r){}
        void Render(std::uint32_t f)override{Check(f==0x5000u,"typed frame pointer");runtime.calls.emplace_back("typed-render");}
    } renderer(runtime);
    pal4::CombatSceneFrame::Render(0x5000u,runtime,renderer);
    Check(Position(runtime,"process-arena")<Position(runtime,"typed-render") &&
          std::count(runtime.calls.begin(),runtime.calls.end(),"combat-system")==3,"typed state0 fallback preserves original singleton acquisition");
    runtime.calls.clear();runtime.inactive=true;
    pal4::CombatSceneFrame::Render(0x5000u,runtime,renderer);
    Check(Position(runtime,"process-arena")<Position(runtime,"typed-render") &&
          std::count(runtime.calls.begin(),runtime.calls.end(),"combat-system")==2,"typed inactive fallback short circuits state read");
    runtime.calls.clear();runtime.inactive=false;runtime.words[0x3000u+508u]=1u;
    pal4::CombatSceneFrame::Render(0x5000u,runtime,renderer);
    Check(Position(runtime,"process-arena")==runtime.calls.size() && Position(runtime,"typed-render")<runtime.calls.size(),"typed active state skips arena");
}

void ActiveState() {
    Runtime runtime;
    runtime.words[0x3000u + 508u] = 1u;
    pal4::CombatSceneFrame::Render(0x5000u, runtime, runtime);
    Check(Position(runtime, "process-arena") == runtime.calls.size() &&
          Position(runtime, "render-world") <
              Position(runtime, "render-object") &&
          Position(runtime, "render-object") <
              Position(runtime, "render-actors") &&
          Position(runtime, "render-actors") <
              Position(runtime, "render-ui"),
          "active combat skips arena prepass and keeps renderer order");
}

}  // namespace

int main() {
    try {
        InactiveAndZeroState();
        ActiveState();
        TypedRenderer();
        std::cout << "combat scene frame tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
