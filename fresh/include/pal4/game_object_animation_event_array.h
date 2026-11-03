#pragma once

#include <cstdint>

namespace pal4 {

struct GameObjectAnimationEventArrayState final {
    std::uint32_t reserved = 0;
    std::uint32_t begin = 0;
    std::uint32_t end = 0;
    std::uint32_t capacity = 0;
};
static_assert(sizeof(GameObjectAnimationEventArrayState) == 16);

class IGameObjectAnimationEventArrayRuntime {
public:
    virtual ~IGameObjectAnimationEventArrayRuntime() = default;
    virtual void DestroyEvent(std::uint32_t address) = 0;
};

class GameObjectAnimationEventArrayRuntimeStub final
    : public IGameObjectAnimationEventArrayRuntime {
public:
    void DestroyEvent(std::uint32_t) override {}
};

class IGameObjectAnimationEventCopyRuntime
    : public IGameObjectAnimationEventArrayRuntime {
public:
    virtual void AssignEvent(std::uint32_t destination,
                             std::uint32_t source) = 0;
};

class GameObjectAnimationEventCopyRuntimeStub final
    : public IGameObjectAnimationEventCopyRuntime {
public:
    void DestroyEvent(std::uint32_t) override {}
    void AssignEvent(std::uint32_t, std::uint32_t) override {}
};

class IGameObjectAnimationEventInsertRuntime
    : public IGameObjectAnimationEventCopyRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyRaw(std::uint32_t destination,
                         std::uint32_t source) = 0;
};

class GameObjectAnimationEventArray final {
public:
    static constexpr std::uint32_t kEventStride = 0x14cu;

    static void CleanupRange(
        std::uint32_t begin, std::uint32_t end,
        IGameObjectAnimationEventArrayRuntime& runtime);
    [[nodiscard]] static int Release(
        GameObjectAnimationEventArrayState& state,
        IGameObjectAnimationEventInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyRangeForward(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination_begin,
        IGameObjectAnimationEventCopyRuntime& runtime);
    static void FillRange(
        std::uint32_t destination_begin, std::uint32_t destination_end,
        std::uint32_t source_event,
        IGameObjectAnimationEventCopyRuntime& runtime);
    [[nodiscard]] static std::uint32_t CopyRangeBackward(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination_end,
        IGameObjectAnimationEventCopyRuntime& runtime);
    [[nodiscard]] static std::uint32_t Insert(
        GameObjectAnimationEventArrayState& state,
        std::uint32_t position, std::uint32_t count,
        std::uint32_t source_event,
        IGameObjectAnimationEventInsertRuntime& runtime);
};

}  // namespace pal4
