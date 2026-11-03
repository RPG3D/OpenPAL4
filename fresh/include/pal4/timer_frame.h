#pragma once

#include <cstdint>

namespace pal4 {

class ITimerFrameButtonsRuntime;

class ITimerFrameRuntime {
public:
    virtual ~ITimerFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEvent(
        std::uint32_t manager, std::uint32_t script_name,
        std::uint32_t create_function) = 0;
    virtual void LoadLayout(std::uint32_t object,
                            std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void ConstructMember(std::uint32_t member) = 0;
    virtual void CleanupMember(std::uint32_t member) = 0;
    virtual void ConstructVector2(std::uint32_t vector) = 0;
    virtual void InitializeButtons(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDialog(
        std::uint32_t object) = 0;
};

class TimerFrame final {
public:
    static constexpr std::uint32_t kSize = 0x78u;
    static constexpr std::uint32_t kScriptNameSource = 0x8b0660u;
    static constexpr std::uint32_t kScriptNameStorage = 0x8e7968u;
    static constexpr std::uint32_t kCreateFunction = 0x47d260u;
    static constexpr std::uint32_t kMemberBaseVtable = 0x843628u;
    static constexpr std::uint32_t kObjectVtable = 0x843b70u;
    static constexpr std::uint32_t kMemberVtable = 0x843b48u;

    [[nodiscard]] static std::uint32_t GetScriptName(
        ITimerFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeScriptName(
        ITimerFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, ITimerFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name, ITimerFrameRuntime& runtime,
        ITimerFrameButtonsRuntime& buttons_runtime);
    [[nodiscard]] static std::uint32_t GetFactory(
        ITimerFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Factory(
        ITimerFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t name,
        ITimerFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t name,
        ITimerFrameRuntime& runtime,
        ITimerFrameButtonsRuntime& buttons_runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, ITimerFrameRuntime& runtime);
};

}  // namespace pal4
