#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalGameObjectManagerLoadRuntime {
public:
    virtual ~IPalGameObjectManagerLoadRuntime() = default;
    [[nodiscard]] virtual std::string_view BasePath() = 0;
    [[nodiscard]] virtual std::uint32_t OpenStream(
        std::string_view path, std::uint32_t context_type,
        std::uint32_t access_mode) = 0;
    [[nodiscard]] virtual bool ReadU32(std::uint32_t stream,
                                       std::uint32_t& value) = 0;
    virtual void CloseStream(std::uint32_t stream) = 0;
    virtual void DeactivateAndClear(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t FactoryForType(
        std::uint32_t manager, std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t CreateObject(
        std::uint32_t factory) = 0;
    virtual void DestroyObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool LoadRecord(
        std::uint32_t object, std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::string_view ObjectName(
        std::uint32_t object) = 0;
    virtual void InsertObject(std::uint32_t manager,
                              std::string_view name,
                              std::uint32_t object) = 0;
};

class PalGameObjectManagerLoad final {
public:
    [[nodiscard]] static bool LoadSceneObjects(
        std::uint32_t manager, std::string_view region,
        std::string_view scene, IPalGameObjectManagerLoadRuntime& runtime);
};

}  // namespace pal4
