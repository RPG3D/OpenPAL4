#include "support/rw_dict_schema_fake.h"
#include "pal4/game_resource_manager.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::GameResourceManagerRuntimeStub {
public:
    explicit Runtime(std::vector<std::string>& events) : events_(events) {}
    bool FrustumTestSphere(std::uint32_t camera,
                            std::uint32_t* sphere) override {
        Check(camera == 0xC00 && sphere == manager + 56,
              "frustum sphere arguments");
        events_.push_back("frustum"); return visible;
    }
    void RenderClumpRaw(std::uint32_t clump) override {
        Check(clump == 0x700, "render clump"); events_.push_back("clump");
    }
    void RenderCameraRaw(std::uint32_t* state,
                          std::uint32_t camera) override {
        Check(state == manager + 14 && camera == 0xC00,
              "render camera arguments");
        events_.push_back("camera");
    }
    void RenderCameraScene(std::uint32_t* state, float elapsed,
                            std::uint32_t* owner) override {
        Check(state == manager + 14 && owner == manager &&
              std::bit_cast<std::uint32_t>(elapsed) ==
                  std::bit_cast<std::uint32_t>(1.5f),
              "scene render elapsed and object offsets");
        events_.push_back("scene");
    }

    std::uint32_t* manager{};
    bool visible{true};
private:
    std::vector<std::string>& events_;
};

class Schema final : public pal4::test::RwDictSchemaFake {
public:
    explicit Schema(std::vector<std::string>& events) : events_(events) {}
    std::uint32_t ListBegin(std::uint32_t list) override {
        Check(list == 0xB00, "animation list begin"); return 0x1000;
    }
    std::uint32_t ListEnd(std::uint32_t list) override {
        Check(list == 0xB00, "animation list end"); return 0x1008;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1000 || address == 0x1004,
              "animation entry address");
        return address == 0x1000 ? 0x501 : 0x502;
    }
    void AdvanceUVAnim(std::uint32_t material, float elapsed) override {
        Check((material == 0x501 || material == 0x502) && elapsed == 3.0f,
              "scaled UV animation time");
        events_.push_back("advance");
    }
    void ApplyUVAnim(std::uint32_t material) override {
        Check(material == 0x501 || material == 0x502, "UV material");
        events_.push_back("apply");
    }
private:
    std::vector<std::string>& events_;
};

void MainRender() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    std::vector<std::string> events;
    Runtime runtime(events);
    runtime.manager = manager.words.data();
    Check(!manager.RenderClump(0xC00, runtime) && events.empty(),
          "unready clump cannot render");
    manager.words[9] = 1;
    manager.words[5] = 0x700;
    manager.words[7] = 9;
    Check(manager.RenderClump(0xC00, runtime) && events.empty() &&
          manager.words[7] == 9,
          "disabled frustum mode returns true without render or flag change");
    manager.words[6] = 1;
    runtime.visible = false;
    Check(!manager.RenderClump(0xC00, runtime) && manager.words[7] == 0 &&
          events == std::vector<std::string>{"frustum"},
          "culled clump clears visibility flag");
    events.clear();
    runtime.visible = true;
    Check(manager.RenderClump(0xC00, runtime) && manager.words[7] == 1 &&
          events == std::vector<std::string>{"frustum", "clump"},
          "visible clump renders and marks visibility");
}

void ShadowRender() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    std::vector<std::string> events;
    Runtime runtime(events);
    runtime.manager = manager.words.data();
    Check(!manager.RenderClump2(0xC00, runtime) && events.empty(),
          "shadow render requires ready clump");
    manager.words[9] = 1;
    manager.words[11] = 1;
    manager.words[12] = 0x701;
    manager.words[13] = 0x702;
    Check(manager.RenderClump2(0xC00, runtime) &&
          events == std::vector<std::string>{"camera"},
          "unculled mode renders shadow camera without frustum test");
    events.clear();
    manager.words[6] = 1;
    runtime.visible = false;
    Check(manager.RenderClump2(0xC00, runtime) &&
          events == std::vector<std::string>{"frustum"},
          "culled shadow path still returns true");
    events.clear();
    runtime.visible = true;
    manager.words[13] = 0;
    Check(manager.RenderClump2(0xC00, runtime) &&
          events == std::vector<std::string>{"frustum"},
          "missing light skips camera render");
    events.clear();
    manager.words[13] = 0x702;
    Check(manager.RenderClump2(0xC00, runtime) &&
          events == std::vector<std::string>{"frustum", "camera"},
          "visible shadow renders camera");
}

void UpdateAndRender() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    std::vector<std::string> events;
    Runtime runtime(events);
    Schema schema(events);
    runtime.manager = manager.words.data();
    Check(!manager.UpdateAndRenderClump(1.5f, runtime, schema) &&
          events.empty(), "unready update gate");
    manager.words[9] = 1;
    manager.words[6] = 1;
    Check(!manager.UpdateAndRenderClump(1.5f, runtime, schema) &&
          events.empty(), "culled update gate");
    manager.words[7] = 1;
    manager.words[38] = 0xB00;
    manager.words[39] = std::bit_cast<std::uint32_t>(2.0f);
    manager.words[11] = 1;
    manager.words[13] = 0x702;
    Check(manager.UpdateAndRenderClump(1.5f, runtime, schema) &&
          events == std::vector<std::string>{
              "advance", "apply", "advance", "apply"},
          "UV animation updates before optional scene render");
    events.clear();
    manager.words[12] = 0x701;
    Check(manager.UpdateAndRenderClump(1.5f, runtime, schema) &&
          events == std::vector<std::string>{
              "advance", "apply", "advance", "apply", "scene"},
          "ready shadow resources render after UV animation");
}

}  // namespace

int main() {
    MainRender();
    ShadowRender();
    UpdateAndRender();
}
