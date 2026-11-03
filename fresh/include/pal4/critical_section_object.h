#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CriticalSectionState final {
    std::uint32_t vtable = 0u;
    std::array<std::uint8_t, 24> native_section{};
    std::uint8_t available = 0u;
    std::uint8_t running = 0u;
    std::array<std::uint8_t, 2> reserved{};
    std::uint32_t handle = 0u;
    std::uint32_t trailing = 0u;
};

static_assert(sizeof(CriticalSectionState) == 40);
static_assert(offsetof(CriticalSectionState, native_section) == 4);
static_assert(offsetof(CriticalSectionState, available) == 28);
static_assert(offsetof(CriticalSectionState, running) == 29);
static_assert(offsetof(CriticalSectionState, handle) == 32);

class ICriticalSectionObjectRuntime {
public:
    virtual ~ICriticalSectionObjectRuntime() = default;
    virtual void InitializeNative(CriticalSectionState& state) = 0;
    virtual void DeleteNative(CriticalSectionState& state) = 0;
    virtual void WaitForSingleObject(std::uint32_t handle,
                                     std::uint32_t milliseconds) = 0;
    virtual void CloseHandle(std::uint32_t handle) = 0;
    virtual void LeaveNative(CriticalSectionState& state) = 0;
    [[nodiscard]] virtual std::uint32_t InitialReturnWord() = 0;
};

class CriticalSectionObjectRuntimeStub final
    : public ICriticalSectionObjectRuntime {
public:
    void InitializeNative(CriticalSectionState&) override {}
    void DeleteNative(CriticalSectionState&) override {}
    void WaitForSingleObject(std::uint32_t, std::uint32_t) override {}
    void CloseHandle(std::uint32_t) override {}
    void LeaveNative(CriticalSectionState&) override {}
    [[nodiscard]] std::uint32_t InitialReturnWord() override {
        return 0u;
    }
};

class CriticalSectionObject final {
public:
    static constexpr std::uint32_t kVtable = 0x84a014u;

    // IDA 0x5fc230.
    [[nodiscard]] static CriticalSectionState& Initialize(
        CriticalSectionState& state,
        ICriticalSectionObjectRuntime& runtime);
    // IDA 0x5fc280.
    static void Destroy(CriticalSectionState& state,
                        ICriticalSectionObjectRuntime& runtime);
    // IDA 0x5fc2b0.
    [[nodiscard]] static std::uint32_t WaitAndRelease(
        CriticalSectionState& state, std::uint32_t milliseconds,
        ICriticalSectionObjectRuntime& runtime);
};

}  // namespace pal4
