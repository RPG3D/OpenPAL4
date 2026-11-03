#include "pal4/smith_window_model_preview.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowModelPreviewRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t collection = 0x3000u;
    static constexpr std::uint32_t flower = 0x4000u;
    static constexpr std::uint32_t model = 0x5000u;
    std::uint32_t model_id = 11u;
    std::uint32_t flower_id = 22u;
    std::uint32_t existing = 0u;
    bool create_flower = true;
    bool create_model = true;
    std::string existing_name = "other";
    std::string created_path;
    std::string created_name;
    std::optional<std::string_view> applied_texture;
    std::uint32_t found_id = 0xffffffffu;
    std::uint32_t cleanup_count = 0u;
    std::vector<std::uint32_t> removed;
    std::vector<std::string> calls;

    std::uint32_t FrameCollection() override {
        calls.emplace_back("collection");
        return collection;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == object + 0x9cu) return model_id;
        if (address == object + 0xa0u) return flower_id;
        if (address == flower + 0x170u) return 44u;
        Check(address == model + 0x170u, "model ID offset");
        return 55u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == object + 0x9cu) {
            model_id = value;
            calls.emplace_back("write-model-id");
        } else {
            Check(address == object + 0xa0u, "flower ID destination");
            flower_id = value;
            calls.emplace_back("write-flower-id");
        }
    }
    std::uint32_t RemoveGameObject(std::uint32_t target_collection,
                                   std::uint32_t id) override {
        Check(target_collection == collection, "removal collection");
        removed.push_back(id);
        calls.emplace_back("remove");
        return id + 0x1000u;
    }
    void LoadFlowerScript() override { calls.emplace_back("flower-script"); }
    void LoadObjectScript() override { calls.emplace_back("object-script"); }
    void LoadWeaponScript() override { calls.emplace_back("weapon-script"); }
    std::uint32_t CleanupScripts() override {
        ++cleanup_count;
        calls.emplace_back("cleanup");
        return 0xca00u + cleanup_count;
    }
    std::uint32_t CreateGameObject(
        std::uint32_t target_collection, std::string_view path,
        std::string_view name, bool active) override {
        Check(target_collection == collection && active,
              "creation collection and active flag");
        calls.emplace_back("create");
        if (name == "_ui_flower") {
            Check(path == "ui\\model\\", "flower model path");
            return create_flower ? flower : 0u;
        }
        created_path = path;
        created_name = name;
        return create_model ? model : 0u;
    }
    std::uint32_t FindGameObject(std::uint32_t target_collection,
                                 std::uint32_t id) override {
        Check(target_collection == collection, "find collection");
        found_id = id;
        calls.emplace_back("find");
        return existing;
    }
    std::string_view ObjectName(std::uint32_t target) override {
        Check(target == existing, "existing object name target");
        return existing_name;
    }
    void SetScale(std::uint32_t target,
                  const std::array<float, 3>& scale) override {
        Check(target == flower && scale ==
                  std::array<float, 3>{3.5f, 3.5f, 3.5f},
              "flower scale");
        calls.emplace_back("scale");
    }
    void SetPosition(std::uint32_t target,
                     const std::array<float, 3>& position) override {
        Check(position[0] == std::bit_cast<float>(0xc26eae14u) &&
                  position[2] == std::bit_cast<float>(0x43952148u),
              "preview x/z position bits");
        Check(position[1] == std::bit_cast<float>(
                  target == flower ? 0x439aee14u : 0x43b8ee14u),
              "flower/model y position bits");
        calls.emplace_back("position");
    }
    void SetTexture(std::uint32_t target,
                    std::optional<std::string_view> texture) override {
        Check(target == model, "texture target");
        applied_texture = texture;
        calls.emplace_back("texture");
    }
    void SetObjectVector(std::uint32_t target,
                         const std::array<float, 3>& value,
                         std::uint32_t flag) override {
        Check(target == model && flag == 0u &&
                  value == std::array<float, 3>{30.0f, 10.0f, 0.0f},
              "model vector and flag");
        calls.emplace_back("vector");
    }
    void SetRotation(std::uint32_t target, std::uint32_t axis,
                     const std::array<float, 3>& rotation) override {
        Check(target == model && axis == 1u &&
                  rotation == std::array<float, 3>{0.0f, 50.0f, 0.0f},
              "model rotation");
        calls.emplace_back("rotation");
    }
};

void CheckClearAndCreate() {
    Runtime clear;
    Check(pal4::SmithWindowModelPreview::Update(
              Runtime::object, std::nullopt, std::nullopt,
              std::nullopt, clear) == 0x1016u &&
              clear.removed == (std::vector<std::uint32_t>{11u, 22u}) &&
              clear.model_id == 0u && clear.flower_id == 0u &&
              clear.cleanup_count == 0u,
          "null name only removes old preview objects");

    Runtime created;
    Check(pal4::SmithWindowModelPreview::Update(
              Runtime::object, "new-model", "new-texture",
              std::nullopt, created) == 0xca02u &&
              created.model_id == 55u && created.flower_id == 44u &&
              created.created_path == "palObject\\" &&
              created.created_name == "new-model" &&
              created.applied_texture == "new-texture" &&
              created.found_id == 0u &&
              created.removed == (std::vector<std::uint32_t>{11u, 22u, 0u}),
          "default path creates flower and model after zero-ID lookup");
    Check(created.calls == (std::vector<std::string>{
              "collection", "remove", "remove", "write-model-id",
              "write-flower-id", "flower-script", "collection",
              "create", "write-flower-id", "scale", "position",
              "cleanup", "collection", "find", "remove",
              "object-script", "create", "texture", "write-model-id",
              "vector", "position", "rotation", "cleanup"}),
          "model preview operation order");
}

void CheckExistingAndFailurePaths() {
    Runtime existing;
    existing.existing = 0x6000u;
    existing.existing_name = "NEW-MODEL";
    Check(pal4::SmithWindowModelPreview::Update(
              Runtime::object, "new-model", "texture",
              std::nullopt, existing) == 0u &&
              existing.cleanup_count == 1u &&
              existing.created_path.empty() && existing.model_id == 0u,
          "matching object name exits after flower cleanup");

    Runtime invalid_path;
    Check(pal4::SmithWindowModelPreview::Update(
              Runtime::object, "new-model", std::nullopt,
              "unknown\\", invalid_path) == 0xca02u &&
              invalid_path.created_path.empty() &&
              invalid_path.model_id == 0u,
          "unsupported class path cleans scripts without model creation");

    Runtime missing_flower;
    missing_flower.create_flower = false;
    Check(pal4::SmithWindowModelPreview::Update(
              Runtime::object, "new-model", std::nullopt,
              "PALWEAPON\\", missing_flower) == 0xca02u &&
              missing_flower.flower_id == 0u &&
              missing_flower.model_id == 55u &&
              missing_flower.created_path == "PALWEAPON\\" &&
              !missing_flower.applied_texture &&
              std::find(missing_flower.calls.begin(), missing_flower.calls.end(),
                        "weapon-script") != missing_flower.calls.end(),
          "weapon path is case-insensitive and tolerates missing flower");

    Runtime missing_model;
    missing_model.create_model = false;
    Check(pal4::SmithWindowModelPreview::Update(
              Runtime::object, "new-model", "texture",
              std::nullopt, missing_model) == 0xca02u &&
              missing_model.model_id == 0u &&
              !missing_model.applied_texture,
          "missing model skips texture and transforms");
}

}  // namespace

int main() {
    try {
        CheckClearAndCreate();
        CheckExistingAndFailurePaths();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
