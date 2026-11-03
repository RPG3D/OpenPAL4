#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IWorldMapFrameRuntime {
public:
    virtual ~IWorldMapFrameRuntime() = default;
    virtual void LoadLayout(std::uint32_t frame, std::uint32_t name) = 0;
    virtual void DestroyBase(std::uint32_t frame) = 0;
    virtual void ConstructMouse(std::uint32_t address) = 0;
    virtual void DestroyMouse(std::uint32_t address) = 0;
    virtual void ConstructNames(std::uint32_t address, std::uint32_t count) = 0;
    virtual void DestroyNames(std::uint32_t address, std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ClearNameFlag(std::uint32_t address) = 0;
    virtual void AssignText(std::uint32_t address, std::string_view value) = 0;
    virtual void DestroyText(std::uint32_t address) = 0;
    virtual void InitializeEventHandlers(std::uint32_t frame) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t address,
                                            std::uint32_t size) = 0;
    virtual void CopyText(std::uint32_t address, std::string_view value) = 0;
    virtual void SetTextSize(std::uint32_t address, std::uint32_t size) = 0;
};

class IWorldMapFrameDestructorRuntime {
public:
    virtual ~IWorldMapFrameDestructorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void DestroyNames(std::uint32_t address,
                              std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t frame) = 0;
};

class WorldMapFrame final {
public:
    static constexpr std::uint32_t kMouseOffset = 0x28u;
    static constexpr std::uint32_t kNamesOffset = 0x94u;
    static constexpr std::uint32_t kNameStride = 0x10u;
    static constexpr std::uint32_t kNameCount = 21u;
    static constexpr std::uint32_t kFlyNameOffset = 0x258u;
    static constexpr std::uint32_t kFrameVtable = 0x843ca8u;
    static constexpr std::uint32_t kMouseVtable = 0x843c80u;
    static constexpr std::uint32_t kFlySpeedBits = 0x3c23d70au;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t frame, std::uint32_t name,
        IWorldMapFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t frame, IWorldMapFrameDestructorRuntime& runtime);
};

}  // namespace pal4
