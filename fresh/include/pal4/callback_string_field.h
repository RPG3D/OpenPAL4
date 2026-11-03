#pragma once

#include "pal4/callback_state_initializer.h"

namespace pal4 {

class ICallbackStringRuntime {
public:
    virtual ~ICallbackStringRuntime() = default;

    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Length(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;

    virtual void CopyAnsiString(std::uint32_t destination,
                                std::uint32_t source) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class CallbackStringField final {
public:
    [[nodiscard]] static int Set(CallbackState::Storage& object,
                                 std::uint32_t source,
                                 ICallbackStringRuntime& runtime);
};

}  // namespace pal4
