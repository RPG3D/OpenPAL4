#pragma once

#include <cstdint>

namespace pal4 {

class IGameManagerComponentCallbackRuntime {
public:
    virtual ~IGameManagerComponentCallbackRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class GameManagerComponentCallback final {
public:
    // IDA 0x5a9b70, embedded by GameManager::InitializeComponents.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object,
        IGameManagerComponentCallbackRuntime& runtime);
};

}  // namespace pal4
