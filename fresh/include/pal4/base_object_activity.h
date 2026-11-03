#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectActivityRuntime {
public:
    virtual ~IBaseObjectActivityRuntime() = default;
    virtual void ApplyMaterialTexture(BaseObjectState&) {}
    virtual void CleanupObject(BaseObjectState&) {}
    [[nodiscard]] virtual bool DialogNeedsUpdate(BaseObjectState&,
                                                  std::uint32_t) { return false; }
    virtual void UpdateDialog(BaseObjectState&, std::uint32_t) {}
    virtual void CleanupAnimationData(BaseObjectState&, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t GameInstance() { return 0u; }
    virtual void CleanupClump(BaseObjectState&, std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t GetProperty(BaseObjectState&) {
        return 0u;
    }

    [[nodiscard]] virtual std::string CreateResourcePath() { return {}; }
    [[nodiscard]] virtual bool LoadClump(
        BaseObjectState&, std::uint32_t, std::string_view,
        std::string_view, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) { return false; }
    virtual void HideHookAtomics(std::uint32_t) {}
    virtual void LoadAnimation(BaseObjectState&, std::uint32_t,
                               std::string_view, std::string_view) {}
    [[nodiscard]] virtual std::uint32_t HookCount(
        std::uint32_t, std::string_view) { return 0u; }
    [[nodiscard]] virtual bool LoadShadowClump(
        BaseObjectState&, std::uint32_t) { return false; }
    virtual void AttachAnimation(BaseObjectState&, std::uint32_t) {}
    virtual void ProcessAtomics(BaseObjectState&) {}
    virtual void ProcessAtomics2(BaseObjectState&) {}
    [[nodiscard]] virtual std::uint32_t CleanupPath(std::string&) {
        return 0u;
    }
    [[nodiscard]] virtual bool RenderClump(BaseObjectState&,
                                            std::uint32_t) { return false; }
    virtual void ProcessAnimation(BaseObjectState&, std::uint32_t,
                                  std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t UpdateCharacter(
        BaseObjectState&, std::uint32_t, std::uint32_t) { return 0u; }
    [[nodiscard]] virtual std::uint32_t SetPropertyVirtual(
        BaseObjectState&, std::uint32_t, std::uint32_t,
        std::uint32_t) { return 0u; }
};

class BaseObjectActivity final {
public:
    [[nodiscard]] static std::uint32_t Reset(
        BaseObjectState& state, IBaseObjectActivityRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        BaseObjectState& state, std::uint32_t kind,
        std::string_view model, std::uint32_t option,
        IBaseObjectActivityRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetStateFlag(
        BaseObjectState& state, std::uint32_t value);
    [[nodiscard]] static std::uint32_t UpdateState(
        BaseObjectState& state, std::uint32_t address,
        std::uint32_t argument, IBaseObjectActivityRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetProperty(
        BaseObjectState& state, std::uint32_t address,
        std::uint32_t value, IBaseObjectActivityRuntime& runtime);
};

}  // namespace pal4
