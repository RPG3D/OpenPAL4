#include "pal4/native_player_model_reload_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/base_object_activity.h"
#include "pal4/game_resource_manager.h"

#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
std::uint32_t Address(const void* value) {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value));
}
// Explicit lifecycle double: no RenderWare actor resources are created here.
struct Activity final : pal4::IBaseObjectActivityRuntime {
    std::string current = "101";
    unsigned resets{}, loads{};
    bool fail{};
    void CleanupClump(pal4::BaseObjectState& state, std::uint32_t) override {
        ++resets;
        state.Write(4u, 0u); state.Write(0xcu, 0u); state.Write(0x14u, 0u);
        for (unsigned i = 0xa0u; i < 0xb8u; i += 4u) state.Write(i, 0u);
    }
    std::string CreateResourcePath() override { return "PALActor\\"; }
    bool LoadClump(pal4::BaseObjectState& state, std::uint32_t name,
        std::string_view path, std::string_view model, std::uint32_t,
        std::uint32_t mode, std::uint32_t extra, std::uint32_t effects) override {
        ++loads;
        Check(std::string_view(reinterpret_cast<const char*>(name)) == "Player-TIANHE" &&
              path == "PALActor\\103\\" && model == "103" &&
              mode == 1u && extra == 0u && effects == 1u,
              "shared Initialize receives saved name and requested model");
        if (fail) throw std::runtime_error("injected actor load failure");
        current = model;
        state.Write(0xcu, Address(current.c_str()));
        state.Write(0x14u, 0x1000u);
        return true;
    }
};
void Run() {
    pal4::PlayerManagerObjectState players;
    pal4::NativeBaseObjectMemory memory;
    pal4::NativePkFileManagerRuntime packages;
    std::array<std::uint32_t, 20> filesystem{};
    filesystem[10] = 0x1010u;
    const auto manager = packages.InitializePackFileManager();
    packages.ProcessPackFiles(manager, Address(filesystem.data()));
    const auto baseline = packages.AllocationCount();
    Activity activity;
    pal4::GameResourceManagerRuntimeStub transforms;
    pal4::NativePlayerModelReloadRuntime runtime(players, memory, packages, activity, transforms);
    auto& slot = players.slots[0];
    const auto object = Address(&slot);
    const char name[] = "Player-TIANHE", same[] = "101", other[] = "103";
    slot.base.Write(4u, Address(name));
    slot.base.Write(0xcu, Address(activity.current.c_str()));
    memory.WriteDword(object + 0xa1cu, Address(same));
    Check(pal4::ScriptProcessAndCleanup::Process(object, runtime) == 0 &&
          activity.resets == 0u && activity.loads == 0u &&
          packages.AllocationCount() == baseline && filesystem[10] == 0x1010u,
          "matching models leave lifecycle and package callbacks untouched");
    const char upper[] = "MODEL", lower[] = "model";
    Check(runtime.CompareCaseInsensitive(Address(upper), Address(lower)) == 0,
          "native CRT compares model names without case");
    memory.WriteDword(object + 0xa1cu, Address(other));
    const std::array<float, 6> values{10.f, 20.f, 30.f, 100.f, 200.f, 300.f};
    for (unsigned i = 0; i < values.size(); ++i)
        slot.base.Write(0xa0u + i * 4u, std::bit_cast<std::uint32_t>(values[i]));
    (void)pal4::ScriptProcessAndCleanup::Process(object, runtime);
    Check(activity.resets == 1u && activity.loads == 1u && activity.current == "103" &&
          filesystem[10] == 0x1010u && packages.AllocationCount() == baseline,
          "changed model executes lifecycle and restores native package scope");
    for (unsigned i = 0; i < values.size(); ++i)
        Check(slot.base.Read(0xa0u + i * 4u) == std::bit_cast<std::uint32_t>(values[i]),
              "saved rotation and position survive actor reset");
    slot.base.Write(4u, Address(name));
    slot.base.Write(0xcu, Address(same));
    activity.fail = true;
    bool threw = false;
    try { (void)pal4::ScriptProcessAndCleanup::Process(object, runtime); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && filesystem[10] == 0x1010u && packages.AllocationCount() == baseline,
          "failed native initialization releases its package scope");
    activity.fail = false;
    slot.base.Write(4u, Address(name)); slot.base.Write(0xcu, Address(same));
    (void)pal4::ScriptProcessAndCleanup::Process(object, runtime);
    Check(activity.loads == 3u && filesystem[10] == 0x1010u,
          "same adapter retries after lifecycle failure");
    memory.WriteDword(object + 0xa1cu, 0u);
    Check(pal4::ScriptProcessAndCleanup::NamesDiffer(object, runtime),
          "missing requested model uses native empty string");
    packages.Shutdown();
    Check(packages.AllocationCount() == 0u, "package owner releases allocations");
}
}
int main() {
    try { Run(); std::cout << "PASS: native player model reload boundary\n"; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
