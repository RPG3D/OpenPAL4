#pragma once

#include <cstdint>

namespace pal4 {

class IGameObjectTextCleanupRuntime {
public:
    virtual ~IGameObjectTextCleanupRuntime() = default;
    virtual void DestroyTextRecord(std::uint32_t address) = 0;
};

class GameObjectTextCleanupRuntimeStub :
    public IGameObjectTextCleanupRuntime {
public:
    void DestroyTextRecord(std::uint32_t) override;
};

class GameObjectTextArrays final {
public:
    // IDA 0x440420.
    static void Cleanup(std::uint32_t object,
                        IGameObjectTextCleanupRuntime& runtime);
};

}  // namespace pal4
