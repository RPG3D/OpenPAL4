#pragma once

#include "pal4/effect_property_state.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_scene_evf_manager_owner.h"
#include "pal4/scene_evf_record.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct RpClump;
struct RwStream;

namespace pal4 {

struct NativeSceneEvfRecord final {
    std::string name;
    std::uint32_t kind = 0u;
    std::uint32_t effect_type = 0u;
    std::string effect_name;
    std::uint32_t effect_field2 = 0u;
    std::uint32_t effect_field3 = 0u;
    std::string world;
    std::string scene;
    std::string script_function;
    std::vector<EffectPropertyState> properties;
    std::uint32_t geometry_flag1 = 0u;
    std::uint32_t geometry_flag2 = 0u;
    RpClump* clump = nullptr;
    std::uint32_t atomic_count = 0u;

    [[nodiscard]] bool ReadFromStream(RwStream* stream,
                                      std::span<const std::uint8_t> bytes,
                                      NativeSceneEvfManagerOwner& manager);
    [[nodiscard]] std::span<const std::uint16_t> Indices() const noexcept;
    [[nodiscard]] std::span<const SceneEvfVertex> Vertices() const noexcept;
    [[nodiscard]] std::span<const SceneEvfTriangle> Triangles() const noexcept;
    [[nodiscard]] const SceneEvfRecordState& GeometryState() const noexcept {
        return geometry_state_;
    }
    [[nodiscard]] std::uint32_t RecordAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&geometry_state_));
    }
    [[nodiscard]] std::size_t GeometryAllocationCount() const noexcept {
        return geometry_memory_.OutstandingAllocations();
    }

    ~NativeSceneEvfRecord();
    NativeSceneEvfRecord() = default;
    NativeSceneEvfRecord(const NativeSceneEvfRecord&) = delete;
    NativeSceneEvfRecord& operator=(const NativeSceneEvfRecord&) = delete;

private:
    SceneEvfRecordState geometry_state_{};
    NativeBaseObjectMemory geometry_memory_;
    bool runtime_initialized_ = false;
};

// Owns one scene's original event records and their RenderWare Clumps.
class NativeSceneEvfAssets final {
public:
    ~NativeSceneEvfAssets() { Clear(); }
    NativeSceneEvfAssets() = default;
    NativeSceneEvfAssets(const NativeSceneEvfAssets&) = delete;
    NativeSceneEvfAssets& operator=(const NativeSceneEvfAssets&) = delete;

    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view world, std::string_view scene);
    void Clear() noexcept;
    [[nodiscard]] std::size_t Size() const noexcept {
        return manager_ ? manager_->RecordCount() : 0u;
    }
    [[nodiscard]] const NativeSceneEvfManagerOwner* Manager() const noexcept {
        return manager_.get();
    }
    [[nodiscard]] NativeSceneEvfManagerOwner* Manager() noexcept {
        return manager_.get();
    }
    [[nodiscard]] const NativeSceneEvfRecord* Find(
        std::string_view name) const noexcept;
    [[nodiscard]] const NativeSceneEvfRecord* FindByResource(
        std::uint32_t resource) const noexcept {
        return manager_ ? manager_->FindByResource(resource) : nullptr;
    }
    void VisitRecords(const std::function<bool(std::uint32_t)>& visitor);
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    std::unique_ptr<NativeSceneEvfManagerOwner> manager_;
    std::string last_error_;
};

}  // namespace pal4
