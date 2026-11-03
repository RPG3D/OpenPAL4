#include "pal4/game_world_material_operations.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameWorldMaterialTextureRuntime {
public:
    std::uint32_t GeometryOfAtomic(std::uint32_t atomic) override {
        Check(atomic == 0xa000u, "atomic address reaches geometry lookup");
        events.push_back("geometry");
        return geometry;
    }
    void ForAllMaterials(
        std::uint32_t geometry_address,
        pal4::GameWorldMaterialTraversal traversal,
        std::uint32_t& value) override {
        Check(geometry_address == 0xb000u,
              "material traversal receives atomic geometry");
        if (traversal == pal4::GameWorldMaterialTraversal::SetTexture) {
            Check(value == 0xd000u, "set traversal forwards texture pointer");
            events.push_back("traverse_set");
            static_cast<void>(pal4::GameWorldMaterialOperations::SetTexture(
                0xc000u, value, *this));
        } else {
            Check(value == 0u, "get traversal initializes texture output");
            events.push_back("traverse_get");
            static_cast<void>(pal4::GameWorldMaterialOperations::CaptureTexture(
                0xc000u, value, *this));
        }
    }
    void SetMaterialTexture(std::uint32_t material,
                            std::uint32_t texture) override {
        Check(material == 0xc000u && texture == 0xd000u,
              "material setter receives source texture");
        events.push_back("set_texture");
    }
    std::uint32_t MaterialTexture(std::uint32_t material) override {
        Check(material >= 0xc000u && material <= 0xc200u,
              "getter reads a material in geometry range");
        events.push_back("get_texture");
        const auto index = (material - 0xc000u) / 0x100u;
        return material_textures.empty() ? texture : material_textures[index];
    }
    std::int32_t MaterialCount(std::uint32_t geometry_address) override {
        Check(geometry_address == 0xb000u,
              "check reads material count from atomic geometry");
        events.push_back("count");
        return material_count;
    }
    std::uint32_t MaterialAt(std::uint32_t geometry_address,
                             std::int32_t index) override {
        Check(geometry_address == 0xb000u && index >= 0 &&
                  index < material_count,
              "check visits each material by index");
        events.push_back("material");
        return 0xc000u + static_cast<std::uint32_t>(index) * 0x100u;
    }
    std::uint32_t CopyTexturePath(
        std::uint32_t text_address, std::string& destination,
        std::uint32_t capacity) override {
        Check((text_address == 0xd010u || text_address == 0xd110u) &&
                  capacity == 256u,
              "texture path begins at texture plus sixteen");
        events.push_back("copy");
        destination = text_address == 0xd010u ? "scene\\hero" : "other";
        return copy_result;
    }
    bool CompareTexturePath(std::string_view candidate,
                            std::string_view actual) override {
        Check(candidate == "other", "check compares supplied candidate path");
        events.push_back("compare");
        return candidate == actual;
    }
    void CaptureFirstAtomic(std::uint32_t clump,
                            std::uint32_t& selected_atomic) override {
        Check(clump == 0xe000u, "clump capture traverses requested object");
        events.push_back("capture_atomic");
        if (first_atomic != 0u)
            static_cast<void>(pal4::GameWorldMaterialOperations::CaptureAtomic(
                first_atomic, selected_atomic));
    }
    void CheckAllAtomicTextures(
        std::uint32_t clump, std::string_view candidate,
        std::uint32_t& match_flag) override {
        Check(clump == 0xe000u && candidate == "scene\\hero" &&
                  match_flag == 1u,
              "clump comparison receives first atomic's texture path");
        events.push_back("check_all");
        if (clump_matches)
            match_flag = 0u;
    }
    std::uint32_t SetAllAtomicTextures(
        std::uint32_t clump, std::uint32_t new_texture) override {
        Check(clump == 0xe000u && new_texture == 0xd000u,
              "clump setter receives requested texture");
        events.push_back("set_all");
        return clump_set_result;
    }

    std::uint32_t geometry = 0xb000u;
    std::uint32_t texture = 0xd000u;
    std::uint32_t copy_result = 0xe000u;
    std::int32_t material_count = 0;
    std::vector<std::uint32_t> material_textures;
    std::uint32_t first_atomic = 0xa000u;
    std::uint32_t clump_set_result = 0xf000u;
    bool clump_matches = false;
    std::vector<std::string> events;
};

}  // namespace

int main() {
    Runtime runtime;
    std::uint32_t selected_atomic = 0u;
    Check(pal4::GameWorldMaterialOperations::CaptureAtomic(
              0xa000u, selected_atomic) == 0u &&
              selected_atomic == 0xa000u,
          "atomic capture callback stores pointer and returns zero");
    Check(pal4::GameWorldMaterialOperations::SetTexture(
              0xc000u, 0xd000u, runtime) == 0xc000u &&
              runtime.events == std::vector<std::string>{"set_texture"},
          "material callback forwards setter call and original pointer");

    runtime.events.clear();
    Check(pal4::GameWorldMaterialOperations::SetAtomicTexture(
              0xa000u, 0xd000u, runtime) == 0xa000u &&
              runtime.events == std::vector<std::string>{
                  "geometry", "traverse_set", "set_texture"},
          "atomic setter visits geometry materials with set callback");
    runtime.events.clear();
    runtime.geometry = 0u;
    Check(pal4::GameWorldMaterialOperations::SetAtomicTexture(
              0xa000u, 0xd000u, runtime) == 0xa000u &&
              runtime.events == std::vector<std::string>{"geometry"},
          "missing geometry skips material traversal");

    runtime.events.clear();
    std::uint32_t texture = 0u;
    Check(pal4::GameWorldMaterialOperations::CaptureTexture(
              0xc000u, texture, runtime) == 0u &&
              texture == 0xd000u &&
              runtime.events == std::vector<std::string>{"get_texture"},
          "getter callback stores material texture and returns zero");

    runtime.events.clear();
    std::string path = "untouched";
    Check(pal4::GameWorldMaterialOperations::GetAtomicTexturePath(
              0u, path, 256u, runtime) == 0u &&
              runtime.events.empty() && path == "untouched",
          "null atomic returns zero without touching destination");
    Check(pal4::GameWorldMaterialOperations::GetAtomicTexturePath(
              0xa000u, path, 256u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"geometry"},
          "missing geometry returns zero without traversal");

    runtime.events.clear();
    runtime.geometry = 0xb000u;
    runtime.texture = 0u;
    Check(pal4::GameWorldMaterialOperations::GetAtomicTexturePath(
              0xa000u, path, 256u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "geometry", "traverse_get", "get_texture"} &&
              path == "untouched",
          "missing texture skips path copy");
    runtime.events.clear();
    runtime.texture = 0xd000u;
    Check(pal4::GameWorldMaterialOperations::GetAtomicTexturePath(
              0xa000u, path, 256u, runtime) == 0xe000u &&
              path == "scene\\hero" &&
              runtime.events == std::vector<std::string>{
                  "geometry", "traverse_get", "get_texture", "copy"},
          "texture path copy forwards source offset, size and return value");

    runtime.events.clear();
    std::uint32_t match_flag = 7u;
    Check(pal4::GameWorldMaterialOperations::CheckAtomicTexturePath(
              0xa000u, match_flag, "other", runtime) == 0xa000u &&
              match_flag == 7u && runtime.events ==
                  std::vector<std::string>{"geometry", "count"},
          "empty material array preserves match flag");
    runtime.events.clear();
    runtime.material_count = 3;
    runtime.material_textures = {0xd000u, 0xd100u, 0u};
    Check(pal4::GameWorldMaterialOperations::CheckAtomicTexturePath(
              0xa000u, match_flag, "other", runtime) == 0xa000u &&
              match_flag == 0u && runtime.events ==
                  std::vector<std::string>{
                      "geometry", "count", "material", "get_texture",
                      "copy", "compare", "material", "get_texture",
                      "copy", "compare", "material", "get_texture"},
          "all material textures are compared; match clears flag without exit");

    runtime.events.clear();
    runtime.material_textures.clear();
    runtime.first_atomic = 0u;
    Check(pal4::GameWorldMaterialOperations::CheckClumpTexturePath(
              0xe000u, 0xd000u, runtime) == 0u &&
              runtime.events ==
                  std::vector<std::string>{"capture_atomic"},
          "empty clump texture path returns before comparison or replacement");

    runtime.events.clear();
    runtime.first_atomic = 0xa000u;
    Check(pal4::GameWorldMaterialOperations::CheckClumpTexturePath(
              0xe000u, 0xd000u, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "capture_atomic", "geometry", "traverse_get",
                  "get_texture", "copy", "check_all", "set_all"},
          "unmatched clump path sets new texture and keeps flag one");

    runtime.events.clear();
    runtime.clump_matches = true;
    Check(pal4::GameWorldMaterialOperations::CheckClumpTexturePath(
              0xe000u, 0xd000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "capture_atomic", "geometry", "traverse_get",
                  "get_texture", "copy", "check_all"},
          "matched path suppresses clump texture replacement");

    runtime.events.clear();
    runtime.clump_matches = false;
    Check(pal4::GameWorldMaterialOperations::CheckClumpTexturePathOnly(
              0xe000u, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "capture_atomic", "geometry", "traverse_get",
                  "get_texture", "copy", "check_all"},
          "check-only variant returns flag without setting texture");
    runtime.events.clear();
    Check(pal4::GameWorldMaterialOperations::SetClumpTexture(
              0xe000u, 0xd000u, runtime) == 0xf000u &&
              runtime.events == std::vector<std::string>{"set_all"},
          "clump texture wrapper forwards traversal result");
}
