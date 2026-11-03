#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerRenderRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t context_reads = 0;
    bool mutate_context = false;
    bool mutate_collection = false;
    bool throw_render = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read " + std::to_string(address));
        if (address == 0x9000u) {
            ++context_reads;
            if (mutate_context)
                return 0x1000u + context_reads;
        }
        return words[address];
    }
    void RenderGui(std::uint32_t system) override {
        events.emplace_back("render GUI");
        Check(system == 0x8800u, "CEGUI system receiver");
        if (throw_render) throw std::runtime_error("render failure");
    }
    std::uint32_t RenderAnimations(std::uint32_t list,
                                   std::uint32_t context) override {
        events.emplace_back("render animations");
        Check(list == 0x51dcu && context == 0x1200u,
              "animation list and dereferenced context");
        return 0x7654u;
    }
    void GetGameObjectCount(std::uint32_t collection,
                            std::uint32_t context) override {
        events.emplace_back("object count");
        Check(collection == 0x8900u && context == 0x1001u,
              "first pre-render call uses initial collection and context");
    }
    void DrawEffects(std::uint32_t manager,
                     std::uint32_t context) override {
        events.emplace_back("draw effects");
        Check(manager == 0x5000u && context == 0x1002u,
              "effects use manager and second context read");
        if (mutate_collection) words[0x519cu] = 0x8a00u;
    }
    void GetGameObject(std::uint32_t collection,
                       std::uint32_t context) override {
        events.emplace_back("object get");
        Check(collection == 0x8a00u && context == 0x1003u,
              "final call re-reads collection and context");
    }
};

void CheckRenderBranches() {
    Runtime absent;
    absent.words[0x9000u] = 0x1200u;
    Check(pal4::UiFrameManagerSingleton::Render(
              0x5000u, 0x9000u, absent) == 0x7654u &&
              absent.events == std::vector<std::string>{
                  "read 20888", "read 36864", "render animations"},
          "animation render runs without CEGUI system");

    Runtime present;
    present.words[0x5198u] = 0x8800u;
    present.words[0x9000u] = 0x1200u;
    Check(pal4::UiFrameManagerSingleton::Render(
              0x5000u, 0x9000u, present) == 0x7654u &&
              present.events == std::vector<std::string>{
                  "read 20888", "render GUI", "read 36864",
                  "render animations"},
          "CEGUI draws before animations and final return is forwarded");

    Runtime throwing;
    throwing.words[0x5198u] = 0x8800u;
    throwing.throw_render = true;
    bool threw = false;
    try {
        (void)pal4::UiFrameManagerSingleton::Render(
            0x5000u, 0x9000u, throwing);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "render failure";
    }
    Check(threw && throwing.events.back() == "render GUI" &&
              throwing.context_reads == 0u,
          "CEGUI exception stops animation render");
}

void CheckPreRenderBranches() {
    Runtime absent;
    pal4::UiFrameManagerSingleton::PreRender(
        0x5000u, 0x9000u, absent);
    Check(absent.events == std::vector<std::string>{"read 20892"} &&
              absent.context_reads == 0u,
          "null collection skips context dereference and all render calls");

    Runtime present;
    present.words[0x519cu] = 0x8900u;
    present.mutate_context = true;
    present.mutate_collection = true;
    pal4::UiFrameManagerSingleton::PreRender(
        0x5000u, 0x9000u, present);
    Check(present.events == std::vector<std::string>{
              "read 20892", "read 36864", "object count",
              "read 36864", "draw effects", "read 36864",
              "read 20892", "object get"},
          "pre-render follows assembly sequence and re-reads fields");
    Check(present.context_reads == 3u,
          "context pointer dereferenced for each native call");
}

}  // namespace

int main() {
    try {
        CheckRenderBranches();
        CheckPreRenderBranches();
        std::cout << "ui_frame_manager_render_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
