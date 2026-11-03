#include "pal4/equipment_model_preview.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Equal(pal4::EquipmentPreviewVector value, float x, float y, float z) {
    return value.x == x && value.y == y && value.z == z;
}

struct Runtime final : pal4::EquipmentModelPreviewRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> texts;
    std::vector<std::string> calls;
    std::uint32_t found{};
    std::uint32_t created{0x9000u};
    std::string item_name{"Sword"};
    std::string object_name{"Sword"};
    pal4::EquipmentPreviewEffectHandle effect{0x12u, 0x34u};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::string ReadText(std::uint32_t address) override {
        calls.emplace_back("text:" + std::to_string(address));
        return texts.at(address);
    }
    std::string ItemName(std::uint32_t item) override {
        Check(item == 0x8000u, "item name reads the item buffer");
        calls.emplace_back("item-name");
        return item_name;
    }
    std::uint32_t FindObject(std::uint32_t id) override {
        calls.emplace_back("find:" + std::to_string(id));
        return found;
    }
    std::string ObjectName(std::uint32_t object) override {
        Check(object == found, "object name reads the found object");
        calls.emplace_back("object-name");
        return object_name;
    }
    std::uint32_t ObjectId(std::uint32_t object) override {
        Check(object == created, "object id reads newly created object");
        calls.emplace_back("object-id");
        return 0x77u;
    }
    void RemoveObject(std::uint32_t id) override {
        calls.emplace_back("remove:" + std::to_string(id));
    }
    void LoadItemScript(bool weapon) override {
        calls.emplace_back(weapon ? "script:weapon" : "script:object");
    }
    void LoadFlowerScript() override { calls.emplace_back("script:flower"); }
    void CleanupScripts() override { calls.emplace_back("cleanup"); }
    std::uint32_t CreateObject(std::string_view directory,
                               std::string_view resource,
                               std::uint32_t mode) override {
        Check(mode == 1u, "create object mode");
        calls.emplace_back("create:" + std::string(directory) + ":" +
                           std::string(resource));
        return created;
    }
    void SetTexture(std::uint32_t object,
                    std::string_view texture) override {
        calls.emplace_back("texture:" + std::to_string(object) + ":" +
                           std::string(texture));
    }
    void SetScaleFromMember(std::uint32_t object,
                            std::uint32_t offset) override {
        Check(object == created && offset == 0xc4u,
              "weapon scale reads object member +0xc4");
        calls.emplace_back("member-scale");
    }
    void SetScale(std::uint32_t object,
                  pal4::EquipmentPreviewVector scale) override {
        Check(object == created && Equal(scale, 3.5f, 3.5f, 3.5f),
              "flower scale");
        calls.emplace_back("scale");
    }
    void SetTransform(std::uint32_t object,
                      pal4::EquipmentPreviewVector value,
                      std::uint32_t mode) override {
        Check(object == created && mode == 0u &&
                  Equal(value, 30.0f, 10.0f, 0.0f),
              "item virtual transform");
        calls.emplace_back("transform");
    }
    void SetPosition(std::uint32_t object,
                     pal4::EquipmentPreviewVector position) override {
        Check(object == created, "position targets created object");
        Check(Equal(position, -59.669998f,
                    calls.size() > 0u &&
                            std::find(calls.begin(), calls.end(),
                                      "script:flower") != calls.end() ?
                        309.85999f : 349.85999f,
                    298.26001f), "position matches IDA constants");
        calls.emplace_back("position");
    }
    void SetRotation(std::uint32_t object, std::uint32_t axis,
                     pal4::EquipmentPreviewVector rotation) override {
        Check(object == created && axis == 1u &&
                  Equal(rotation, 0.0f, 50.0f, 0.0f),
              "item rotation");
        calls.emplace_back("rotation");
    }
    pal4::EquipmentPreviewEffectHandle CreateEffect(
        std::string_view name, pal4::EquipmentPreviewVector position,
        std::int32_t parent, std::uint32_t mode) override {
        Check(name == "LH" && Equal(position, -59.669998f, 200.86f,
                                     298.26001f) && parent == -1 &&
                  mode == 0u, "effect creation arguments");
        calls.emplace_back("effect:create");
        return effect;
    }
    void ResetTemporaryEffect() override {
        calls.emplace_back("effect:reset-temp");
    }
    void SetEffectScale(pal4::EquipmentPreviewEffectHandle handle,
                        pal4::EquipmentPreviewVector scale,
                        bool enabled) override {
        Check(handle.first == effect.first &&
                  handle.second == effect.second && !enabled &&
                  Equal(scale, 2.2f, 2.2f, 2.2f), "effect scale");
        calls.emplace_back("effect:scale");
    }
    void MarkEffectDirty(pal4::EquipmentPreviewEffectHandle handle) override {
        Check(handle.first == effect.first &&
                  handle.second == effect.second, "dirty effect handle");
        calls.emplace_back("effect:dirty");
    }
    void CloseEffect(pal4::EquipmentPreviewEffectHandle handle,
                     std::uint32_t event) override {
        Check(handle.first == 0x12u && handle.second == 0x34u &&
                  event == 0u, "close saved effect handle");
        calls.emplace_back("effect:close");
    }
};

Runtime MakeRuntime(std::uint32_t category) {
    Runtime runtime;
    runtime.words = {{0x8094u, category}, {0x80dcu, 0x1000u},
                     {0x80ecu, 0x2000u}};
    runtime.texts = {{0x1000u, "sword_model"}, {0x2000u, "sword_skin"}};
    return runtime;
}

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    state.Write(0xf8u, 5u);
    state.Write(0xfcu, 6u);
    state.Write(0x100u, 7u);
    state.Write(0x104u, 0x12u);
    state.Write(0x108u, 0x34u);
    return state;
}

void TestNullAndReuse() {
    auto state = MakeState();
    auto runtime = MakeRuntime(1u);
    pal4::EquipmentModelPreview::UpdateItem(state, 0u, runtime);
    Check(runtime.calls.empty(), "null item makes no runtime calls");

    runtime.found = 0x9000u;
    pal4::EquipmentModelPreview::UpdateItem(state, 0x8000u, runtime);
    const std::vector<std::string> expected{
              "find:5", "item-name", "object-name", "read:32916",
              "script:weapon", "read:33004", "text:8192",
              "texture:36864:sword_skin", "cleanup"};
    if (runtime.calls != expected) {
        for (const auto& call : runtime.calls) std::cerr << call << '\n';
    }
    Check(runtime.calls == expected,
          "matching name updates texture without recreation");
    Check(state.Read(0xf8u) == 5u, "reuse keeps existing id");
}

void TestRecreateAndInvalidCategory() {
    for (const auto category : {0u, 2u, 3u, 6u}) {
        auto state = MakeState();
        auto runtime = MakeRuntime(category);
        runtime.found = 0x9000u;
        runtime.object_name = "Other";
        pal4::EquipmentModelPreview::UpdateItem(state, 0x8000u, runtime);
        const bool weapon = category <= 2u;
        Check(state.Read(0xf8u) == 0x77u, "created id stored in state");
        Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                        "remove:5") != runtime.calls.end(),
              "old object removed on name mismatch");
        Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                        weapon ? "script:weapon" : "script:object") !=
                  runtime.calls.end(), "category picks script family");
        Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                        weapon ? "create:PALWeapon\\:sword_model" :
                                 "create:palObject\\:sword_model") !=
                  runtime.calls.end(), "category picks object directory");
        Check(runtime.calls.back() == "cleanup", "scripts cleaned last");
        const auto scale = std::find(runtime.calls.begin(),
                                     runtime.calls.end(), "member-scale");
        Check((scale != runtime.calls.end()) == weapon,
              "embedded scale is weapon-only");
    }

    auto state = MakeState();
    auto runtime = MakeRuntime(7u);
    pal4::EquipmentModelPreview::UpdateItem(state, 0x8000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "find:5", "remove:5", "read:32916"} &&
              state.Read(0xf8u) == 5u,
          "invalid category removes old object without creating one");

    runtime = MakeRuntime(3u);
    runtime.created = 0u;
    runtime.words[0x80dcu] = 0u;
    pal4::EquipmentModelPreview::UpdateItem(state, 0x8000u, runtime);
    Check(runtime.calls.back() == "cleanup" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "create:palObject\\:") != runtime.calls.end() &&
              state.Read(0xf8u) == 5u,
          "creation failure still cleans scripts and preserves stored id");
}

void TestVisibility() {
    auto state = MakeState();
    auto runtime = MakeRuntime(0u);
    pal4::EquipmentModelPreview::SetVisible(state, true, runtime);
    Check(state.Read(0xfcu) == 0x77u &&
              state.Read(0x104u) == 0x12u &&
              state.Read(0x108u) == 0x34u,
          "show stores flower id and two-word effect handle");
    Check(runtime.calls == std::vector<std::string>{
              "find:6", "effect:create", "effect:reset-temp",
              "effect:scale", "effect:dirty", "script:flower",
              "create:ui\\model\\:_ui_flower", "object-id", "scale",
              "position", "cleanup"}, "show call order");

    runtime.calls.clear();
    runtime.found = 0x9000u;
    pal4::EquipmentModelPreview::SetVisible(state, true, runtime);
    Check(runtime.calls == std::vector<std::string>{"find:119"},
          "existing flower makes show idempotent");

    runtime.calls.clear();
    pal4::EquipmentModelPreview::SetVisible(state, false, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "remove:119", "remove:7", "remove:5", "effect:close"},
          "hide removes three objects before effect event");
    Check(state.Read(0xfcu) == 0u && state.Read(0x100u) == 0u &&
              state.Read(0xf8u) == 0u, "hide zeros all object ids");
}

}  // namespace

int main() {
    TestNullAndReuse();
    TestRecreateAndInvalidCategory();
    TestVisibility();
}
