#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/base_object_lifecycle.h"
#include "pal4/base_object_motion.h"

namespace pal4 {

class IBaseObjectComponentRuntime {
public:
    virtual ~IBaseObjectComponentRuntime() = default;
    virtual void ClearAnimationFlag(std::uint32_t, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t FindAnimation(
        std::uint32_t, const char*) { return 0u; }
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t) { return 0u; }
    virtual void SetAnimationTime(std::uint32_t, std::uint32_t) {}
    virtual void WriteDword(std::uint32_t, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t GameInstance() { return 0u; }
    [[nodiscard]] virtual bool LoadClump(
        BaseObjectState&, std::uint32_t, std::string_view,
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) { return false; }
    virtual void CleanupClump(BaseObjectState&, std::uint32_t) {}
    virtual void ConstructResource(BaseObjectState&) {}
    virtual void DestroyResource(BaseObjectState&) {}
    virtual void RemoveFromWorld(BaseObjectState&);
    virtual void FreeObject(std::uint32_t) {}
    [[nodiscard]] virtual std::string CreatePath() { return {}; }
    virtual void CleanupPath(std::string&) {}
    virtual void RotateFrame(std::uint32_t, std::uint32_t,
                             float, std::uint32_t) {}
    virtual void UpdateFrame(std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t AddClumpToWorld(
        std::uint32_t, std::uint32_t) { return 0u; }
    [[nodiscard]] virtual std::uint32_t WorldFromClump(
        std::uint32_t) { return 0u; }
    virtual void RemoveClumpFromWorld(std::uint32_t, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t UpdateClumpTransform(
        BaseObjectState&, const BaseObjectVector3&, std::uint32_t) {
        return 0u;
    }
    virtual void UpdateClumpPosition(
        BaseObjectState&, const BaseObjectVector3&, std::uint32_t) {}
};

class BaseObjectComponent final {
public:
    // IDA 0x4444f0; trailing source arguments are unused in the machine code.
    [[nodiscard]] static std::uint32_t Initialize(
        const BaseObjectState& state, std::uint32_t unused0,
        std::uint32_t unused1, IBaseObjectComponentRuntime& runtime);
    // IDA 0x444510.
    [[nodiscard]] static std::uint32_t Process(
        const BaseObjectState& state, std::uint32_t animation_container,
        std::uint32_t output_address, IBaseObjectComponentRuntime& runtime);
    // IDA 0x444590 and 0x4445d0.
    [[nodiscard]] static std::uint32_t Setup(
        BaseObjectState& state, std::uint32_t label,
        std::string_view path, std::uint32_t model_address,
        std::uint8_t flag, IBaseObjectComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        BaseObjectState& state, IBaseObjectComponentRuntime& runtime);
    // IDA 0x4445f0, 0x444610, 0x444630 and 0x444680.
    [[nodiscard]] static std::uint32_t Construct(
        BaseObjectState& state, std::uint32_t address,
        IBaseObjectComponentRuntime& runtime);
    static void Destroy(BaseObjectState& state,
                        IBaseObjectComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        BaseObjectState& state, std::uint32_t address,
        std::uint32_t flags, IBaseObjectComponentRuntime& runtime);
    static void CleanupResourceThunk(
        BaseObjectState& state, IBaseObjectComponentRuntime& runtime);
    // IDA 0x444690 and 0x444770.
    [[nodiscard]] static std::uint32_t LoadFromPrefixAndName(
        BaseObjectState& state, std::uint32_t attachment,
        std::string_view prefix, std::string_view model,
        std::uint32_t model_address,
        std::uint32_t unused, std::uint8_t flag,
        IBaseObjectComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupAndRelated(
        BaseObjectState& state, IBaseObjectComponentRuntime& runtime);
    // IDA 0x444780 through 0x444920.
    static void Rotate(BaseObjectState& state, double elapsed,
                       IBaseObjectComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddToWorld(
        const BaseObjectState& state, std::uint32_t world,
        IBaseObjectComponentRuntime& runtime);
    static void RemoveFromWorld(BaseObjectState& state,
                                IBaseObjectComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPosition(
        BaseObjectState& state, const BaseObjectVector3& position,
        IBaseObjectComponentRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetRotation(
        BaseObjectState& state, std::uint8_t flag,
        const BaseObjectVector3& rotation);
    static void SetScale(BaseObjectState& state,
                         const BaseObjectVector3& scale,
                         IBaseObjectComponentRuntime& runtime);
};

}  // namespace pal4
