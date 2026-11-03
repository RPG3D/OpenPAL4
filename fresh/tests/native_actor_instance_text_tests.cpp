#include "pal4/actor_instance_lifecycle.h"
#include "pal4/native_player_slot_copy_runtime.h"

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

static_assert(sizeof(void*) == sizeof(std::uint32_t));

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

// Only the text boundary is native in this fixture. Resource callbacks record
// targets; they do not claim model loading or RenderWare cleanup coverage.
struct Runtime final : pal4::IActorInstanceRuntime {
    pal4::NativePlayerSlotCopyRuntime text;
    std::vector<std::uint32_t> released;
    std::vector<std::uint32_t> components;
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        text.WriteDword(address, value);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        text.WriteByte(address, value);
    }
    void ReleaseText(std::uint32_t record) override {
        text.ReleaseText(record);
        released.push_back(record);
    }
    void SetTextFlag(std::uint32_t record, std::uint8_t flag) override {
        Check(flag == 1u, "destruction requests text cleanup");
        ReleaseText(record);
    }
    void InitializeResourceManager(std::uint32_t) override { throw std::logic_error("unused fixture constructor"); }
    void InitializeObjectState(std::uint32_t) override { throw std::logic_error("unused fixture constructor"); }
    void InitializeTga(std::uint32_t) override { throw std::logic_error("unused fixture constructor"); }
    void InitializeComponent(std::uint32_t) override { throw std::logic_error("unused fixture constructor"); }
    void InitializeNameNode(std::uint32_t) override { throw std::logic_error("unused fixture constructor"); }
    void ResetEffect(std::uint32_t) override {}
    std::uint32_t DestroyComponent(std::uint32_t component) override {
        components.push_back(component);
        return component;
    }
    void ResetActorProperties(std::uint32_t) override {}
    void CleanupObjectData(std::uint32_t) override {}
    std::uint32_t GameInstance() override { return 0u; }
    std::uint32_t CleanupClump(std::uint32_t, std::uint32_t) override { return 0u; }
    void CleanupObjectState(std::uint32_t) override {}
    void DestroyResourceManager(std::uint32_t) override {}
};

void Exercise(bool long_text) {
    // Original child allocation is 0x610, distinct from the scene NPC layout.
    std::array<std::uint32_t, 0x610u / 4u> image{};
    const auto actor = reinterpret_cast<std::uint32_t>(image.data());
    Runtime runtime;
    const std::array offsets{496u, 1036u, 1052u, 1068u, 1500u, 1516u, 1532u};
    const std::string value = long_text ? std::string(200u, 'M') : "B01";
    for (const auto offset : offsets) runtime.text.AssignText(actor + offset, value);
    Check(runtime.text.OwnedTextCount() == 7u, "seven independent host string owners");
    pal4::ActorInstanceLifecycle::Destroy(actor, runtime);
    Check(runtime.text.OwnedTextCount() == 0u, "host strings released by shared destructor");
    Check(runtime.released == std::vector<std::uint32_t>{actor+1532u, actor+1516u,
          actor+1500u, actor+1068u, actor+1052u, actor+1036u, actor+496u},
          "native text release follows original reverse member order");
    Check(runtime.components == std::vector<std::uint32_t>{actor+1088u,actor+624u},
          "component destruction order");
    for (const auto offset : offsets)
        for (const auto member : {4u, 8u, 12u})
            Check(runtime.text.ReadDword(actor+offset+member) == 0u, "host view cleared");

    // 52A720 has a separate text release path used for component retirement.
    runtime.released.clear();
    for (const auto offset : {412u, 428u, 444u})
        runtime.text.AssignText(actor+624u+offset, value);
    (void)pal4::ActorInstanceLifecycle::CleanupComponentText(actor+624u, runtime);
    Check(runtime.text.OwnedTextCount() == 0u && runtime.released ==
          std::vector<std::uint32_t>{actor+1068u,actor+1052u,actor+1036u},
          "separate component cleanup uses the same native owner");

    // Reject borrowed text without clearing or freeing its foreign buffer.
    const char foreign[] = "borrowed model";
    runtime.text.WriteDword(actor+500u, reinterpret_cast<std::uint32_t>(foreign));
    bool rejected = false;
    try { runtime.ReleaseText(actor+496u); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected && std::strcmp(foreign, "borrowed model") == 0 &&
          runtime.text.ReadDword(actor+500u) == reinterpret_cast<std::uint32_t>(foreign),
          "foreign pointer rejected before mutation");
    runtime.text.WriteDword(actor+500u, 0u);
}
}

int main() {
    try {
        for (unsigned repeat=0; repeat<16; ++repeat) {
            Exercise(false);
            Exercise(true);
        }
        std::cout << "native_actor_instance_text_tests passed (SSO/heap/foreign ownership)\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
