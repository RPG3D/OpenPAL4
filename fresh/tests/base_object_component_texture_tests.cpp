#include "pal4/base_object_component_texture.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::IBaseObjectComponentTextureRuntime {
    std::vector<char> events;
    bool allocated{};
    bool throw_on_load{};
    std::uint32_t texture{0x9000u};
    std::uint32_t loaded_texture{0xffffffffu};
    std::string cleaned_path;

    void InitializeTemporary(std::uint8_t mode) override {
        Check(mode == 3u,
              "temporary mode begins with source address low byte");
        events.push_back('I');
    }
    std::uint8_t SetErrorFlag(std::uint8_t flag) override {
        Check(flag == 0u || flag == 1u,
              "temporary cleanup uses zero and one error flags");
        events.push_back(flag == 0u ? '0' : '1');
        return flag == 0u ? 0u : 0x55u;
    }
    bool AllocateAndCopy(std::string_view name,
                         std::uint8_t mode) override {
        Check(name == "skin" && mode == 1u,
              "temporary text allocation copies name with mode one");
        events.push_back('A');
        return allocated;
    }
    std::uint8_t ReleaseTemporary() override {
        events.push_back('R');
        return 0x7eu;
    }
    std::string CreatePath(std::uint32_t root) override {
        Check(root == 0x3000u,
              "texture path starts from component object name");
        events.push_back('P');
        return "role\\";
    }
    void CleanupPath(std::string& path) override {
        cleaned_path = path;
        events.push_back('p');
    }
    std::uint32_t TextureDictionary() override {
        events.push_back('D');
        return 0x8000u;
    }
    std::uint32_t ResolveTexture(std::uint32_t dictionary,
                                 std::string_view path) override {
        Check(dictionary == 0x8000u && path == "role\\skin.png",
              "dictionary receives appended PNG path");
        events.push_back('T');
        return texture;
    }
    void LoadTexture(pal4::BaseObjectState&,
                     std::uint32_t name_address,
                     std::uint32_t selected_texture) override {
        Check(name_address == 0x6203u,
              "resource manager receives original texture-name address");
        loaded_texture = selected_texture;
        events.push_back('L');
        if (throw_on_load) throw std::runtime_error("load failed");
    }
};

void CheckBranches() {
    pal4::BaseObjectState state{};
    state.Write(8u, 0x3000u);
    Runtime runtime;
    Check(pal4::BaseObjectComponentTexture::SetTexture(
              state, 0x6203u, "skin", runtime) == 0x55u &&
              runtime.events == std::vector<char>{'I', '0', 'A', '1'},
          "missing temporary storage follows error-flag cleanup branch");

    runtime.events.clear();
    runtime.allocated = true;
    Check(pal4::BaseObjectComponentTexture::SetTexture(
              state, 0x6203u, "skin", runtime) == 0x7eu &&
              runtime.events == std::vector<char>{
                  'I', '0', 'A', 'P', 'D', 'T', 'L', 'p', 'R'} &&
              runtime.loaded_texture == 0x9000u &&
              runtime.cleaned_path == "role\\skin.png",
          "loaded texture path cleans string before temporary release");
    runtime.events.clear();
    runtime.texture = 0u;
    Check(pal4::BaseObjectComponentTexture::SetTexture(
              state, 0x6203u, "skin", runtime) == 0x7eu &&
              runtime.loaded_texture == 0u &&
              runtime.events == std::vector<char>{
                  'I', '0', 'A', 'P', 'D', 'T', 'L', 'p', 'R'},
          "zero resolved texture still reaches resource-manager load");
}

void CheckExceptionCleanup() {
    pal4::BaseObjectState state{};
    state.Write(8u, 0x3000u);
    Runtime runtime;
    runtime.allocated = true;
    runtime.throw_on_load = true;
    bool threw = false;
    try {
        (void)pal4::BaseObjectComponentTexture::SetTexture(
            state, 0x6203u, "skin", runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<char>{
              'I', '0', 'A', 'P', 'D', 'T', 'L', 'p', 'R'},
          "exception releases path and temporary storage exactly once");
}

}  // namespace

int main() {
    CheckBranches();
    CheckExceptionCleanup();
}
