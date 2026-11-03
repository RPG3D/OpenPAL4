#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataRoleRecordTextRuntime {
public:
    virtual ~IGameDataRoleRecordTextRuntime() = default;
    virtual void DestroyText(std::uint32_t address) = 0;
};

class GameDataRoleRecordLifecycle final {
public:
    // IDA 0x4ec060: destroy two arrays of 16-byte text records.
    static void Destroy(std::uint32_t record,
                        IGameDataRoleRecordTextRuntime& runtime);
};

}  // namespace pal4
