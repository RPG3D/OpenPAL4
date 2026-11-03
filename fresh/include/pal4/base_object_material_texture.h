#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectMaterialRuntime {
public:
    virtual ~IBaseObjectMaterialRuntime() = default;
    [[nodiscard]] virtual std::string CreatePath(std::uint32_t) { return {}; }
    virtual void CleanupPath(std::string&) {}
    virtual void AddAndProcessScripts() {}
    virtual void CleanupScripts() {}
    [[nodiscard]] virtual std::uint32_t TextureDictionary() { return 0u; }
    [[nodiscard]] virtual std::uint32_t ResolveTexture(
        std::uint32_t, std::string_view) { return 0u; }
    [[nodiscard]] virtual std::uint32_t ResolveStoredTexture(
        std::uint32_t, std::uint32_t) { return 0u; }
    virtual void SetMaterialTexture(std::uint32_t, std::uint32_t) {}
};

class BaseObjectMaterialTexture final {
public:
    // IDA 0x443d00 and 0x443de0.
    [[nodiscard]] static std::uint32_t SetFace(
        BaseObjectState& state, std::string_view texture_name,
        IBaseObjectMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetLeg(
        BaseObjectState& state, std::string_view texture_name,
        IBaseObjectMaterialRuntime& runtime);
    // IDA 0x443ec0 and 0x443f20.
    [[nodiscard]] static std::uint32_t ApplyFace(
        BaseObjectState& state, IBaseObjectMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyLeg(
        BaseObjectState& state, IBaseObjectMaterialRuntime& runtime);
};

}  // namespace pal4
