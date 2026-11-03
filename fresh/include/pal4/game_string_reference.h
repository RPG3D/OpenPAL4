#pragma once

#include <cstdint>

namespace pal4 {

class IGameStringReferenceRuntime {
public:
    virtual ~IGameStringReferenceRuntime() = default;
    virtual std::uint32_t GameDbInstance() = 0;
    virtual std::uint32_t FindGameString(std::uint32_t manager,
                                         std::uint32_t key) = 0;
};

class GameStringReference final {
public:
    explicit GameStringReference(std::uint32_t key) : key_(key) {}

    // IDA 0x500b20: this+4 is the key; GameDB+52 owns the utilities manager.
    [[nodiscard]] std::uint32_t FindInGameDB(
        IGameStringReferenceRuntime& runtime) const;

private:
    std::uint32_t key_;
};

}  // namespace pal4
