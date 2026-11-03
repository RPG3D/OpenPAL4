#include "pal4/base_object_material_texture.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<char, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectMaterialRuntime {
    std::vector<Event> events;
    std::uint32_t texture{0x7000u};
    bool throw_on_resolve{};
    std::string cleaned_path;

    std::string CreatePath(std::uint32_t root) override {
        Check(root == 0x3000u, "path construction receives object name");
        events.emplace_back('P', root);
        return "role\\";
    }
    void CleanupPath(std::string& path) override {
        cleaned_path = path;
        events.emplace_back('p', 0u);
    }
    void AddAndProcessScripts() override { events.emplace_back('S', 0u); }
    void CleanupScripts() override { events.emplace_back('s', 0u); }
    std::uint32_t TextureDictionary() override {
        events.emplace_back('D', 0u);
        return 0x4000u;
    }
    std::uint32_t ResolveTexture(std::uint32_t dictionary,
                                 std::string_view path) override {
        Check(dictionary == 0x4000u && path == "role\\skin.dds",
              "direct set resolves the appended texture path");
        events.emplace_back('R', 0u);
        if (throw_on_resolve) throw std::runtime_error("lookup failed");
        return texture;
    }
    std::uint32_t ResolveStoredTexture(std::uint32_t dictionary,
                                       std::uint32_t path) override {
        Check(dictionary == 0x4000u && path == 0x5000u,
              "apply resolves the stored string pointer");
        events.emplace_back('r', path);
        return texture;
    }
    void SetMaterialTexture(std::uint32_t material,
                            std::uint32_t selected_texture) override {
        Check((material == 0x5500u || material == 0x5600u) &&
                  selected_texture == 0x7000u,
              "material setter receives selected material and texture");
        events.emplace_back('M', material);
    }
};

std::uint32_t Set(pal4::BaseObjectState& state, Runtime& runtime,
                  bool face) {
    return face ? pal4::BaseObjectMaterialTexture::SetFace(
                      state, "skin.dds", runtime) :
                  pal4::BaseObjectMaterialTexture::SetLeg(
                      state, "skin.dds", runtime);
}

std::uint32_t Apply(pal4::BaseObjectState& state, Runtime& runtime,
                    bool face) {
    return face ? pal4::BaseObjectMaterialTexture::ApplyFace(state, runtime) :
                  pal4::BaseObjectMaterialTexture::ApplyLeg(state, runtime);
}

void CheckSet(bool face) {
    pal4::BaseObjectState state{};
    state.Write(8u, 0x3000u);
    Runtime runtime;
    const auto material = face ? 0x5500u : 0x5600u;
    Check(Set(state, runtime, face) == 0u &&
              runtime.events == std::vector<Event>{{'P', 0x3000u},
                                                   {'p', 0u}} &&
              runtime.cleaned_path == "role\\skin.dds",
          "missing material still builds and releases temporary path");

    state.Write(face ? 0x550u : 0x560u, material);
    runtime.events.clear();
    Check(Set(state, runtime, face) == 1u &&
              runtime.events == std::vector<Event>{{'P', 0x3000u},
                                                   {'S', 0u}, {'D', 0u},
                                                   {'R', 0u}, {'M', material},
                                                   {'s', 0u}, {'p', 0u}},
          "direct set loads and applies texture before script/path cleanup");
    runtime.events.clear();
    runtime.texture = 0u;
    Check(Set(state, runtime, face) == 0u &&
              runtime.events == std::vector<Event>{{'P', 0x3000u},
                                                   {'S', 0u}, {'D', 0u},
                                                   {'R', 0u}, {'s', 0u},
                                                   {'p', 0u}},
          "failed direct lookup still cleans scripts and path");
}

void CheckApply(bool face) {
    pal4::BaseObjectState state{};
    Runtime runtime;
    const auto material_offset = face ? 0x550u : 0x560u;
    const auto string_offset = face ? 0x544u : 0x554u;
    Check(Apply(state, runtime, face) == 0u && runtime.events.empty(),
          "missing material bypasses stored-texture resolution");
    state.Write(material_offset, face ? 0x5500u : 0x5600u);
    state.Write(string_offset + 4u, 0u);
    Check(Apply(state, runtime, face) == 1u && runtime.events.empty(),
          "empty stored texture returns one without script calls");
    state.Write(string_offset + 4u, 0xffffffffu);
    Check(Apply(state, runtime, face) == 1u && runtime.events.empty(),
          "stored length uses signed comparison");
    state.Write(string_offset, 0x5000u);
    state.Write(string_offset + 4u, 8u);
    const auto material = face ? 0x5500u : 0x5600u;
    Check(Apply(state, runtime, face) == 1u &&
              runtime.events == std::vector<Event>{{'S', 0u}, {'D', 0u},
                                                   {'r', 0x5000u},
                                                   {'M', material},
                                                   {'s', 0u}},
          "stored texture path loads and applies in order");
    runtime.events.clear();
    runtime.texture = 0u;
    Check(Apply(state, runtime, face) == 1u &&
              runtime.events == std::vector<Event>{{'S', 0u}, {'D', 0u},
                                                   {'r', 0x5000u},
                                                   {'s', 0u}},
          "stored lookup failure still returns one with material present");
}

void CheckExceptionCleanup() {
    pal4::BaseObjectState state{};
    state.Write(8u, 0x3000u);
    state.Write(0x550u, 0x5500u);
    Runtime runtime;
    runtime.throw_on_resolve = true;
    bool threw = false;
    try {
        (void)Set(state, runtime, true);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.cleaned_path == "role\\skin.dds" &&
              runtime.events.back() == Event{'p', 0u},
          "exception releases temporary path once");
}

}  // namespace

int main() {
    for (bool face : {true, false}) {
        CheckSet(face);
        CheckApply(face);
    }
    CheckExceptionCleanup();
}
