#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

struct MinimapLayerCollection {
    std::uint8_t header_byte{};
    std::vector<std::uint32_t> grid_objects;
};

class IMinimapFileRuntime {
public:
    virtual ~IMinimapFileRuntime() = default;
    virtual void EnterPackageScope() = 0;
    virtual void LeavePackageScope() = 0;
    virtual void InitializeXmlDocument() = 0;
    virtual void DestroyXmlDocument() = 0;
    [[nodiscard]] virtual std::string BasePath() = 0;
    [[nodiscard]] virtual std::uint32_t OpenRead(const std::string& path) = 0;
    virtual void InitializeFileStream(std::uint32_t handle) = 0;
    virtual void ReadBytes(std::uint32_t handle, void* destination,
                           std::uint32_t count) = 0;
    virtual void SetImageBasePath(const std::string& path) = 0;
    [[nodiscard]] virtual std::uint32_t FindImagePath(
        const std::string& name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateGridObject(
        std::uint32_t bytes) = 0;
    virtual void ConstructGridObject(std::uint32_t object) = 0;
    virtual void InitializeGridObject(
        std::uint32_t object, std::uint32_t image,
        std::uint32_t scale, const std::array<std::uint8_t, 24>& metadata) = 0;
    virtual void CloseFile(std::uint32_t handle) = 0;
};

class MinimapFileLoader final {
public:
    [[nodiscard]] static MinimapLayerCollection& Load(
        MinimapLayerCollection& output, const std::string& folder,
        const std::string& scene, std::uint8_t incoming_header_byte,
        IMinimapFileRuntime& runtime);
};

}  // namespace pal4
