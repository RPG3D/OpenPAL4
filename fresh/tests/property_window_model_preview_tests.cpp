#include "pal4/property_window_model_preview.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowModelPreviewRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t existing = 0u;
    std::uint32_t created = 0x9000u;
    int name_comparison = 0;
    bool throw_texture = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t GameObjectCollection() override {
        calls.emplace_back("collection");
        return 0xc000u;
    }
    std::uint32_t FindObject(std::uint32_t collection,
                             std::uint32_t id) override {
        Check(collection == 0xc000u, "lookup uses frame-manager collection");
        calls.push_back("find:" + std::to_string(id));
        return existing;
    }
    std::uint32_t RemoveObject(std::uint32_t collection,
                               std::uint32_t id) override {
        Check(collection == 0xc000u, "removal uses frame-manager collection");
        calls.push_back("remove:" + std::to_string(id));
        return 0xbeefu;
    }
    int CompareNamesIgnoreCase(std::uint32_t left,
                               std::uint32_t right) override {
        Check(left == 0xa000u && right == 0x8400u,
              "comparison receives current name and definition +0x54");
        calls.emplace_back("compare");
        return name_comparison;
    }
    void PrepareScripts() override { calls.emplace_back("prepare"); }
    std::uint32_t CleanupScripts() override {
        calls.emplace_back("cleanup");
        return 0xdeadu;
    }
    std::uint32_t CreateObject(std::uint32_t collection,
                               std::string_view directory,
                               std::uint32_t model_name,
                               std::uint32_t flag) override {
        Check(collection == 0xc000u && directory == "palObject\\" &&
                  model_name == 0x8400u && flag == 1u,
              "creation receives exact source directory, model and flag");
        calls.emplace_back("create");
        return created;
    }
    void SetTexture(std::uint32_t object,
                    std::uint32_t texture) override {
        calls.push_back("texture:" + std::to_string(object) + ":" +
                        std::to_string(texture));
        if (throw_texture) throw std::runtime_error("texture");
    }
    void SetBaseTransform(
        std::uint32_t object, const std::array<float, 3>& value,
        std::uint32_t flag) override {
        Check(object == 0x9000u && flag == 0u &&
                  value == std::array<float, 3>{30.0f, 10.0f, 0.0f},
              "new object receives base transform and zero flag");
        calls.emplace_back("base-transform");
    }
    void SetPosition(std::uint32_t object,
                     const std::array<float, 3>& value) override {
        Check(object == 0x9000u &&
                  std::bit_cast<std::uint32_t>(value[0]) == 0xc26eae14u &&
                  std::bit_cast<std::uint32_t>(value[1]) == 0x43aeee14u &&
                  std::bit_cast<std::uint32_t>(value[2]) == 0x43952148u,
              "new object position preserves original float bits");
        calls.emplace_back("position");
    }
    void SetRotation(std::uint32_t object, std::uint32_t axis,
                     const std::array<float, 3>& value) override {
        Check(object == 0x9000u && axis == 1u &&
                  value == std::array<float, 3>{0.0f, 50.0f, 0.0f},
              "new object receives source rotation and axis");
        calls.emplace_back("rotation");
    }
};

void SetupRecord(Runtime& runtime) {
    runtime.words[0x8100u] = 0x8200u;
    runtime.words[0x8254u] = 0x8400u;
    runtime.words[0x8264u] = 0x8500u;
    runtime.words[0x9000u + 0x14u] = 1u;
    runtime.words[0x9000u + 0x170u] = 0x4321u;
    runtime.words[0x8804u] = 0xa000u;
}

void TestAbsentRecord() {
    pal4::PropertyWindowState state;
    state.Write(0x78u, 0x1234u);
    Runtime runtime;
    Check(pal4::PropertyWindowModelPreview::Update(state, 0u, runtime) == 0u,
          "missing preview object returns null");
    Check(state.Read(0x78u) == 0x1234u &&
          runtime.calls == std::vector<std::string>{
              "collection", "find:4660"},
          "missing object leaves cached ID unchanged");

    runtime.calls.clear();
    runtime.existing = 0x8800u;
    Check(pal4::PropertyWindowModelPreview::Update(
              state, 0u, runtime) == 0xbeefu,
          "removed preview returns collection result");
    Check(state.Read(0x78u) == 0u &&
          runtime.calls == std::vector<std::string>{
              "collection", "find:4660", "remove:4660"},
          "present object is removed and cached ID cleared");
}

void TestReuseAndRecreate() {
    pal4::PropertyWindowState state;
    state.Write(0x78u, 0x1234u);
    Runtime runtime;
    SetupRecord(runtime);
    runtime.existing = 0x8800u;
    Check(pal4::PropertyWindowModelPreview::Update(
              state, 0x8100u, runtime) == 0xdeadu,
          "same model returns script cleanup result");
    Check(runtime.calls == std::vector<std::string>{
              "read:33024", "read:33364", "collection", "find:4660",
              "read:34820", "compare", "prepare", "read:33380",
              "texture:34816:34048", "cleanup"},
          "same model updates texture without reconstruction");
    Check(state.Read(0x78u) == 0x1234u,
          "reused preview keeps cached ID");

    runtime.calls.clear();
    runtime.name_comparison = 1;
    Check(pal4::PropertyWindowModelPreview::Update(
              state, 0x8100u, runtime) == 0xdeadu,
          "different model returns cleanup result");
    Check(runtime.calls == std::vector<std::string>{
              "read:33024", "read:33364", "collection", "find:4660",
              "read:34820", "compare", "prepare", "remove:4660",
              "create", "read:36884", "read:33380",
              "texture:36864:34048", "read:37232", "base-transform",
              "position", "rotation", "cleanup"},
          "different model rebuilds and applies original transforms");
    Check(state.Read(0x78u) == 0x4321u,
          "rebuilt model stores new object ID");
}

void TestCreationFailuresAndCleanup() {
    pal4::PropertyWindowState state;
    state.Write(0x78u, 0x1234u);
    Runtime runtime;
    SetupRecord(runtime);
    runtime.created = 0u;
    Check(pal4::PropertyWindowModelPreview::Update(
              state, 0x8100u, runtime) == 0xdeadu,
          "null creation still cleans script resources");
    Check(runtime.calls.back() == "cleanup" &&
          state.Read(0x78u) == 0x1234u,
          "null creation leaves stored model ID unchanged");

    runtime.calls.clear();
    runtime.created = 0x9000u;
    runtime.words[0x9014u] = 0u;
    (void)pal4::PropertyWindowModelPreview::Update(
        state, 0x8100u, runtime);
    Check(runtime.calls.back() == "cleanup" &&
          state.Read(0x78u) == 0x1234u,
          "object without +0x14 component skips texture and transform");

    runtime.calls.clear();
    runtime.words[0x9014u] = 1u;
    runtime.words[0x8264u] = 0u;
    runtime.throw_texture = true;
    try {
        (void)pal4::PropertyWindowModelPreview::Update(
            state, 0x8100u, runtime);
        Check(false, "texture exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls[runtime.calls.size() - 2u] ==
                  "texture:36864:8657252" &&
              runtime.calls.back() == "cleanup",
              "null texture uses empty literal and exception cleans scripts");
    }
}

}  // namespace

int main() {
    TestAbsentRecord();
    TestReuseAndRecreate();
    TestCreationFailuresAndCleanup();
}
