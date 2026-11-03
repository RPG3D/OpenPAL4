#include "pal4/trade_window_model_display.h"

#include <array>
#include <cstdlib>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ITradeWindowModelRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x819cu, 0x9000u}, {0x711cu, 0x123u},
        {0xa170u, 0x456u}};
    std::vector<std::string> calls;
    std::uint32_t previous = 0u;
    std::string previous_name = "Original";
    std::uint32_t created = 0xa000u;
    std::uint32_t removal_result = 0x111u;
    std::uint32_t texture_result = 0x222u;
    std::uint32_t cleanup_result = 0x333u;
    std::uint32_t removed_id = 0u;
    std::uint32_t textured_object = 0u;
    std::uint32_t texture = 0u;
    std::string path;
    std::string name;
    std::uint32_t scale_address = 0u;
    std::array<float, 3> object_vector{};
    std::array<float, 3> position{};
    std::array<float, 3> rotation{};

    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t RemoveGameObject(std::uint32_t collection,
                                   std::uint32_t id) override {
        Check(collection == 0x9000u, "remove collection");
        removed_id = id;
        calls.emplace_back("remove");
        return removal_result;
    }
    std::uint32_t FindGameObject(std::uint32_t collection,
                                 std::uint32_t id) override {
        Check(collection == 0x9000u && id == 0x123u,
              "find collection and stored id");
        calls.emplace_back("find");
        return previous;
    }
    std::string_view ObjectName(std::uint32_t object) override {
        Check(object == 0xb000u, "existing object identity");
        calls.emplace_back("object-name");
        return previous_name;
    }
    std::uint32_t SetTexture(std::uint32_t object,
                             std::uint32_t value) override {
        textured_object = object;
        texture = value;
        calls.emplace_back("texture");
        return texture_result;
    }
    void ProcessObjectScript() override {
        calls.emplace_back("script-object");
    }
    void ProcessWeaponScript() override {
        calls.emplace_back("script-weapon");
    }
    std::uint32_t CreateGameObject(std::uint32_t collection,
        std::string_view object_path, std::string_view object_name,
        bool flag) override {
        Check(collection == 0x9000u && flag, "create collection and flag");
        path = object_path;
        name = object_name;
        calls.emplace_back("create");
        return created;
    }
    void SetScaleFromMember(std::uint32_t object,
                            std::uint32_t address) override {
        Check(object == 0xa000u, "weapon scale object");
        scale_address = address;
        calls.emplace_back("scale");
    }
    void SetObjectVector(std::uint32_t object,
        const std::array<float, 3>& vector, std::uint32_t flag) override {
        Check(object == 0xa000u && flag == 0u, "object vector target");
        object_vector = vector;
        calls.emplace_back("vector");
    }
    void SetPosition(std::uint32_t object,
                     const std::array<float, 3>& value) override {
        Check(object == 0xa000u, "object position target");
        position = value;
        calls.emplace_back("position");
    }
    void SetRotation(std::uint32_t object, std::uint32_t axis,
                     const std::array<float, 3>& value) override {
        Check(object == 0xa000u && axis == 1u, "object rotation target");
        rotation = value;
        calls.emplace_back("rotation");
    }
    std::uint32_t CleanupScripts() override {
        calls.emplace_back("cleanup");
        return cleanup_result;
    }
};

void TestClearModel() {
    Runtime runtime;
    Check(pal4::TradeWindowModelDisplay::SetGameObjectDisplay(
              0x7000u, std::nullopt, 0u, std::nullopt, runtime) == 0x111u &&
              runtime.removed_id == 0x123u &&
              runtime.words.at(0x711cu) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "remove", "write"},
          "null model removes object and clears id without script processing");
}

void TestExistingModel() {
    Runtime runtime;
    runtime.previous = 0xb000u;
    Check(pal4::TradeWindowModelDisplay::SetGameObjectDisplay(
              0x7000u, "original", 0x700u, "ignored", runtime) == 0x222u &&
              runtime.textured_object == 0xb000u &&
              runtime.texture == 0x700u &&
              runtime.words.at(0x711cu) == 0x123u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "find", "object-name", "texture"},
          "same model name only updates texture, ignoring path");
}

void TestCreateObject() {
    for (const auto path : {
         std::optional<std::string_view>{},
         std::optional<std::string_view>{"PALOBJECT\\"}}) {
        Runtime runtime;
        runtime.previous = 0xb000u;
        Check(pal4::TradeWindowModelDisplay::SetGameObjectDisplay(
                  0x7000u, "New", 0x701u, path, runtime) == 0x333u &&
                  runtime.removed_id == 0x123u &&
                  runtime.path == path.value_or("palObject\\") &&
                  runtime.name == "New" &&
                  runtime.words.at(0x711cu) == 0x456u &&
                  runtime.textured_object == 0xa000u &&
                  runtime.texture == 0x701u &&
                  runtime.scale_address == 0u &&
                  runtime.object_vector ==
                      std::array<float, 3>{30.0f, 10.0f, 0.0f} &&
                  runtime.position == std::array<float, 3>{
                      -59.669998f, 369.85999f, 298.26001f} &&
                  runtime.rotation ==
                      std::array<float, 3>{0.0f, 50.0f, 0.0f} &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "find", "object-name", "remove",
                      "script-object", "create", "texture", "write",
                      "vector", "position", "rotation", "cleanup"},
              "default and explicit object paths rebuild the model");
    }
}

void TestWeaponAndFailures() {
    Runtime weapon;
    Check(pal4::TradeWindowModelDisplay::SetGameObjectDisplay(
              0x7000u, "Sword", 0x702u, "PalWeapon\\", weapon) == 0x333u &&
              weapon.scale_address == 0xa0c4u &&
              weapon.calls == std::vector<std::string>{
                  "manager", "find", "remove", "script-weapon", "create",
                  "scale", "texture", "write", "vector", "position",
                  "rotation", "cleanup"},
          "weapon path uses weapon script and object member scale");

    Runtime unknown;
    Check(pal4::TradeWindowModelDisplay::SetGameObjectDisplay(
              0x7000u, "Other", 0u, "other\\", unknown) == 0x333u &&
              unknown.words.at(0x711cu) == 0x123u &&
              unknown.calls == std::vector<std::string>{
                  "manager", "find", "remove", "cleanup"},
          "unsupported path removes old object and leaves id as in IDA");

    Runtime missing;
    missing.created = 0u;
    Check(pal4::TradeWindowModelDisplay::SetGameObjectDisplay(
              0x7000u, "Other", 0u, std::nullopt, missing) == 0x333u &&
              missing.words.at(0x711cu) == 0x123u &&
              missing.calls == std::vector<std::string>{
                  "manager", "find", "remove", "script-object", "create",
                  "cleanup"},
          "failed object creation still cleans scripts without replacing id");
}

}  // namespace

int main() {
    TestClearModel();
    TestExistingModel();
    TestCreateObject();
    TestWeaponAndFailures();
    return EXIT_SUCCESS;
}
