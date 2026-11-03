#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataParameterRecordDestroyRuntime {
public:
    virtual ~IGameDataParameterRecordDestroyRuntime() = default;
    virtual void DestroyTextRecord(std::uint32_t address) = 0;
};

class GameDataParameterRecordLifecycle final {
public:
    // IDA 0x4e35f0: destroy six 16-byte texts from record +4 backward.
    static void Destroy(std::uint32_t record,
                        IGameDataParameterRecordDestroyRuntime& runtime);
};

}  // namespace pal4
