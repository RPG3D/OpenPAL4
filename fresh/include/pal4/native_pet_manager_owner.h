#pragma once

#include "pal4/base_object_lifecycle.h"
#include "pal4/pet_manager.h"

#include <array>
#include <cstddef>
#include <span>

namespace pal4 {
class IStringBufferStorageRuntime;
class IGameResourceManagerRuntime;
class IBaseObjectActivityRuntime;

// Application lifetime; scene-ready adapters borrow this same saveable image.
class NativePetManagerOwner final {
public:
    NativePetManagerOwner(IBaseObjectRuntime& members, IStringBufferStorageRuntime& memory,
                          IGameResourceManagerRuntime& transforms,
                          IBaseObjectActivityRuntime& activity);
    ~NativePetManagerOwner();
    NativePetManagerOwner(const NativePetManagerOwner&) = delete;
    NativePetManagerOwner& operator=(const NativePetManagerOwner&) = delete;

    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] std::span<std::byte> Image() noexcept;
    [[nodiscard]] BaseObjectState& Base() noexcept { return state_.base; }
    [[nodiscard]] std::uint32_t Source() const;
    void InitializeStoredData();
    void InitializeData(std::uint32_t source);
    // IDA43DFA0 (vtable+44), including GameObject_Reset at442730.
    [[nodiscard]] std::uint32_t Reset();
    class FrameLease final {
    public:
        FrameLease(const FrameLease&)=delete;
        FrameLease& operator=(const FrameLease&)=delete;
        ~FrameLease();
    private:
        friend class NativePetManagerOwner;
        explicit FrameLease(NativePetManagerOwner&);
        NativePetManagerOwner& owner_;
    };
    [[nodiscard]] FrameLease BorrowFrame();
    [[nodiscard]] bool FrameActive() const noexcept {return frame_active_;}


private:
    struct State {
        BaseObjectState base;
        std::array<std::uint32_t,
            (PetManager::kObjectBytes - BaseObjectState::kSize) / 4u> tail{};
    } state_;
    static_assert(sizeof(State) == PetManager::kObjectBytes);
    static_assert(offsetof(State, base) == 0u);
    class Runtime;
    bool frame_active_{};
    IBaseObjectRuntime& members_;
    IStringBufferStorageRuntime& memory_;
    IGameResourceManagerRuntime& transforms_;
    IBaseObjectActivityRuntime& activity_;
};
}  // namespace pal4
