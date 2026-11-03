#include "pal4/pal_game_iv_component_destructors.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPalGameIVComponentDestructorRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t ResetEffectHandle(std::uint32_t address) override {
        calls.push_back("effect:" + std::to_string(address));
        return 0x1234u;
    }
    std::uint32_t DestroyXmlNode(std::uint32_t address) override {
        calls.push_back("xml:" + std::to_string(address));
        return 0x5678u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("vtable:" + std::to_string(address));
        words[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void Destructors() {
    Runtime runtime;
    Check(pal4::PalGameIVComponentDestructors::DestructScene(
              0x1000u, runtime) == 0x1234u &&
              runtime.words[0x1000u] == 0x8485c8u &&
              runtime.calls == std::vector<std::string>({
                  "effect:4100", "vtable:4096"}), "scene destructor");
    runtime.calls.clear();
    Check(pal4::PalGameIVComponentDestructors::DestructCombat(
              0x2000u, runtime) == 0x5678u &&
              runtime.words[0x2000u] == 0x8485c8u &&
              runtime.calls == std::vector<std::string>({
                  "xml:8204", "vtable:8192"}), "combat destructor");
    runtime.calls.clear();
    pal4::PalGameIVComponentDestructors::DestructBase(0x3000u, runtime);
    Check(runtime.words[0x3000u] == 0x8485c8u &&
              runtime.calls == std::vector<std::string>({"vtable:12288"}),
          "base destructor");
}

void DeletingThunks() {
    Runtime runtime;
    Check(pal4::PalGameIVComponentDestructors::DeleteScene(
              0x1000u, 0u, runtime) == 0x1000u &&
              runtime.calls == std::vector<std::string>({
                  "effect:4100", "vtable:4096"}),
          "scene nondeleting thunk");
    runtime.calls.clear();
    Check(pal4::PalGameIVComponentDestructors::DeleteScene(
              0x1000u, 3u, runtime) == 0x1000u &&
              runtime.calls == std::vector<std::string>({
                  "effect:4100", "vtable:4096", "free:4096"}),
          "scene deleting thunk");
    runtime.calls.clear();
    Check(pal4::PalGameIVComponentDestructors::DeleteCombat(
              0x2000u, 1u, runtime) == 0x2000u &&
              runtime.calls == std::vector<std::string>({
                  "xml:8204", "vtable:8192", "free:8192"}),
          "combat deleting thunk");
    runtime.calls.clear();
    Check(pal4::PalGameIVComponentDestructors::DeleteBase(
              0x3000u, 2u, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>({"vtable:12288"}),
          "base nondeleting thunk");
    runtime.calls.clear();
    Check(pal4::PalGameIVComponentDestructors::DeleteBase(
              0x3000u, 1u, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>({
                  "vtable:12288", "free:12288"}),
          "base deleting thunk");
}

}  // namespace

int main() {
    try {
        Destructors();
        DeletingThunks();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
