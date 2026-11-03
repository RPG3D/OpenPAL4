#pragma once

#include <cstdint>

namespace pal4 {

class ISystemSettingInterfaceRuntime;

class ISystemSettingFrameRuntime {
public:
    virtual ~ISystemSettingFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEvent(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t create_function) = 0;
    virtual void LoadLayout(std::uint32_t frame,
                            std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void ConstructMember(std::uint32_t member) = 0;
    virtual void CleanupMember(std::uint32_t member) = 0;
    virtual void SetupInterface(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDialog(
        std::uint32_t frame) = 0;
};

class SystemSettingFrame final {
public:
    static constexpr std::uint32_t kSize = 0x1fcu;
    static constexpr std::uint32_t kNameSource = 0x8b06acu;
    static constexpr std::uint32_t kNameStorage = 0x8e7974u;
    static constexpr std::uint32_t kCreateFunction = 0x47dc80u;
    static constexpr std::uint32_t kBaseMemberVtable = 0x843628u;
    static constexpr std::uint32_t kObjectVtable = 0x843bb8u;
    static constexpr std::uint32_t kMemberVtable = 0x843b90u;

    [[nodiscard]] static std::uint32_t GetName(
        ISystemSettingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        ISystemSettingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, ISystemSettingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, ISystemSettingFrameRuntime& runtime,
        ISystemSettingInterfaceRuntime& interface_runtime);
    [[nodiscard]] static std::uint32_t GetFactory(
        ISystemSettingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Factory(
        ISystemSettingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t frame, std::uint32_t name,
        ISystemSettingFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t frame, std::uint32_t name,
        ISystemSettingFrameRuntime& runtime,
        ISystemSettingInterfaceRuntime& interface_runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t frame, ISystemSettingFrameRuntime& runtime);
};

}  // namespace pal4
