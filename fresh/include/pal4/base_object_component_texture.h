#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectComponentTextureRuntime {
public:
    virtual ~IBaseObjectComponentTextureRuntime() = default;
    virtual void InitializeTemporary(std::uint8_t) {}
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(std::uint8_t flag) {
        return flag;
    }
    [[nodiscard]] virtual bool AllocateAndCopy(std::string_view,
                                                std::uint8_t) {
        return false;
    }
    [[nodiscard]] virtual std::uint8_t ReleaseTemporary() { return 0u; }
    [[nodiscard]] virtual std::string CreatePath(std::uint32_t) {
        return {};
    }
    virtual void CleanupPath(std::string&) {}
    [[nodiscard]] virtual std::uint32_t TextureDictionary() { return 0u; }
    [[nodiscard]] virtual std::uint32_t ResolveTexture(
        std::uint32_t, std::string_view) { return 0u; }
    virtual void LoadTexture(BaseObjectState&, std::uint32_t,
                             std::uint32_t) {}
};

class BaseObjectComponentTexture final {
public:
    // IDA 0x444940. Returns the low byte produced by temporary cleanup.
    [[nodiscard]] static std::uint8_t SetTexture(
        BaseObjectState& state, std::uint32_t name_address,
        std::string_view name,
        IBaseObjectComponentTextureRuntime& runtime);
};

}  // namespace pal4
