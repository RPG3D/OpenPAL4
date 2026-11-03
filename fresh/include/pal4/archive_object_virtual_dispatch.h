#pragma once

#include <cstdint>

namespace pal4 {

class IArchiveObjectVirtualRuntime {
public:
    virtual ~IArchiveObjectVirtualRuntime() = default;
    [[nodiscard]] virtual std::int32_t Invoke(
        std::uint32_t object, std::uint32_t slot_offset) = 0;
};

class ArchiveObjectVirtualRuntimeStub final : public IArchiveObjectVirtualRuntime {
public:
    [[nodiscard]] std::int32_t Invoke(std::uint32_t,
                                      std::uint32_t) override { return 0; }
};

class ArchiveObjectVirtualDispatch final {
public:
    explicit ArchiveObjectVirtualDispatch(IArchiveObjectVirtualRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ec7a0 and 0x5ec7b0 are distinct vtable tail jumps.
    [[nodiscard]] std::int32_t ReleaseSlot12(std::uint32_t object);
    [[nodiscard]] std::int32_t ReleaseSlot8(std::uint32_t object);

private:
    IArchiveObjectVirtualRuntime& runtime_;
};

}  // namespace pal4
