#pragma once
#include "pal4/native_gob_object_image.h"
#include <functional>
#include <memory>
#include <array>
#include <string>
#include <string_view>
namespace pal4 {
class NativeCameraManagerOwner;
class NativeEffectSceneSession;
class IScriptObjectVisibilityRuntime;
class NativeMilesSound2dRuntime;
class IGobMachineInventoryRuntime;
class NativeInventorySession;
class NativeGameDatabase;
class INativeGobMachineTaskSpawns;
class IScriptInventoryChangeRuntime;
struct NativeGobMachineRewardServices {
    NativeInventorySession* inventory{};
    const NativeGameDatabase* database{};
    IScriptInventoryChangeRuntime* hints{};
    std::function<bool()> hint_visible;
};
struct NativeGobMachineScriptServices {
    std::function<bool()> actor_active, busy;
    std::function<void()> initialize, cleanup;
    std::function<bool(std::string_view)> has_module, has_path;
    std::function<void(std::string_view)> load_module;
    std::function<void(std::string_view,std::string_view)> push_module;
    std::uint32_t arena_context{};
};
struct NativeGobMachineServices {
    INativeGobMachineTaskSpawns* task_spawns{}; // bound by the owning Machine
    NativeCameraManagerOwner* cameras{};
    IScriptObjectVisibilityRuntime* objects{};
    NativeMilesSound2dRuntime* sound{};
    NativeEffectSceneSession* effects{}; // borrowed until both task containers drain
    IGobMachineInventoryRuntime* inventory{};
    std::function<void(float)> animation;
    std::function<void(std::uint32_t)> game_event;
    std::function<void(std::uint32_t)> queue_fade_out;
    std::function<void()> refresh_minimap;
    std::string arena,scene;
    NativeGobMachineScriptServices script;
    NativeGobMachineRewardServices reward;
    std::function<std::uint32_t(std::string_view)> find_object;
    std::function<void(std::uint32_t,const std::array<float,3>&)> touch_object;
    std::function<bool(std::uint32_t)> object_done;
};
// Owns task dispatch and list contents; borrows the scene's object/config/memory.
// The object retains its two container sentinels and task-pointer allocation.
class NativeGobMachineRuntime final : public INativeGobMachineService {
public:
    NativeGobMachineRuntime(NativeGobObjectImage&,const NativeSceneGobRecord&,
        NativeBaseObjectMemory&,NativeGobMachineServices);
    ~NativeGobMachineRuntime();
    std::uint32_t Trigger() override;
    std::uint32_t ClearContainer(std::uint32_t) override;
    void SceneActivity(bool active) override;
    void Advance(float);
    std::uint32_t ResetTriggeredRuntime();
    std::size_t TaskCount() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
