#pragma once

#include <cstdint>

namespace pal4 {

class IInputMethodStaticRuntime {
public:
    virtual ~IInputMethodStaticRuntime() = default;
    virtual void BeginInputMethod(std::uint32_t object) = 0;
    virtual void ReleaseVirtualMemory(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t RegisterAtExit(
        std::uint32_t callback) = 0;
};

class InputMethodStaticRegistration final {
public:
    static constexpr std::uint32_t kObject = 0x9078f0u;
    static constexpr std::uint32_t kCleanupCallback = 0x5c4460u;

    // IDA 0x5c4430, 0x5c4440 and 0x5c4460.
    [[nodiscard]] static std::int32_t Register(IInputMethodStaticRuntime& runtime);
    static void Initialize(IInputMethodStaticRuntime& runtime);
    static void Cleanup(IInputMethodStaticRuntime& runtime);
};

}  // namespace pal4
