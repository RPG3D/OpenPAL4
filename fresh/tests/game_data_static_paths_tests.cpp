#include "pal4/game_data_static_paths.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IGameDataStaticPathsRuntime {
    std::vector<std::string> calls;
    std::uint32_t initialize_result = 0x123u;
    int register_result = -4;

    std::uint32_t InitializeString(std::uint32_t address,
                                   std::string_view value) override {
        calls.emplace_back("init:" + std::to_string(address) + ":" +
                           std::string(value));
        return initialize_result;
    }
    int RegisterAtexit(std::uint32_t cleanup_entry) override {
        calls.emplace_back("atexit:" + std::to_string(cleanup_entry));
        return register_result;
    }
    void CleanupString(std::uint32_t address) override {
        calls.emplace_back("cleanup:" + std::to_string(address));
    }
};

int failures = 0;

void Check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void TestAllStaticPaths() {
    struct Expected {
        std::uint32_t register_entry;
        std::uint32_t initialize_entry;
        std::uint32_t cleanup_entry;
        std::uint32_t global_address;
        std::string_view path;
    };
    constexpr std::array<Expected, 13> expected{{
        {0x66dfe0u, 0x66dff0u, 0x66e010u, 0x94d560u, "gamedata\\"},
        {0x66e020u, 0x66e030u, 0x66e050u, 0x94d554u, "PALWorld\\"},
        {0x66e060u, 0x66e070u, 0x66e090u, 0x94d548u, "PALActor\\"},
        {0x66e0a0u, 0x66e0b0u, 0x66e0d0u, 0x94d53cu, "palObject\\"},
        {0x66e0e0u, 0x66e0f0u, 0x66e110u, 0x94d530u, "PALWeapon\\"},
        {0x66e120u, 0x66e130u, 0x66e150u, 0x94d524u, "PALSound\\"},
        {0x66e160u, 0x66e170u, 0x66e190u, 0x94d518u,
         "gamedata\\Music\\"},
        {0x66e1a0u, 0x66e1b0u, 0x66e1d0u, 0x94d50cu,
         "gamedata\\EditData\\scene\\"},
        {0x66e1e0u, 0x66e1f0u, 0x66e210u, 0x94d500u,
         "gamedata\\EditData\\script\\"},
        {0x66e220u, 0x66e230u, 0x66e250u, 0x94d4f4u,
         "gamedata\\EffData\\"},
        {0x66e260u, 0x66e270u, 0x66e290u, 0x94d4e8u,
         "gamedata\\MatFX\\NorTexture\\"},
        {0x66e2a0u, 0x66e2b0u, 0x66e2d0u, 0x94d4dcu,
         "gamedata\\MatFX\\ReflectionTexture\\"},
        {0x66e2e0u, 0x66e2f0u, 0x66e310u, 0x94d4d0u,
         "gamedata\\MatFX\\DualTexture\\"},
    }};

    Runtime runtime;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto path = static_cast<pal4::GameDataPath>(index);
        const auto& actual = pal4::GameDataStaticPaths::Spec(path);
        const auto& item = expected[index];
        Check(actual.register_entry == item.register_entry &&
                  actual.initialize_entry == item.initialize_entry &&
                  actual.cleanup_entry == item.cleanup_entry &&
                  actual.global_address == item.global_address &&
                  actual.path == item.path,
              "every IDA triplet must match its global and exact path");

        const auto init_call = "init:" +
            std::to_string(item.global_address) + ":" +
            std::string(item.path);
        const auto atexit_call = "atexit:" +
            std::to_string(item.cleanup_entry);
        const auto cleanup_call = "cleanup:" +
            std::to_string(item.global_address);
        runtime.calls.clear();
        Check(pal4::GameDataStaticPaths::Initialize(path, runtime) == 0x123u &&
                  runtime.calls == std::vector<std::string>({init_call}),
              "internal initializer forwards native result and value");
        runtime.calls.clear();
        Check(pal4::GameDataStaticPaths::Register(path, runtime) == -4 &&
                  runtime.calls == std::vector<std::string>({
                      init_call, atexit_call}),
              "entry wrapper initializes before registering cleanup");
        runtime.calls.clear();
        pal4::GameDataStaticPaths::Cleanup(path, runtime);
        Check(runtime.calls == std::vector<std::string>({cleanup_call}),
              "cleanup callback releases its own global");
    }

    runtime.initialize_result = 0;
    runtime.calls.clear();
    Check(pal4::GameDataStaticPaths::Register(
              pal4::GameDataPath::Data, runtime) == -4 &&
              runtime.calls.size() == 2u,
          "atexit is registered even when initialization returns zero");
}

}  // namespace

int main() {
    TestAllStaticPaths();
    return failures == 0 ? 0 : 1;
}
