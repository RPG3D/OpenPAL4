#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <functional>

namespace pal4 {

using UtilitiesRecordEquals =
    std::function<bool(std::uint32_t previous, std::uint32_t candidate)>;

class IUtilitiesRecordTextRuntime : public IStringAssignRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class UtilitiesRecordTextField final {
public:
    [[nodiscard]] static StringBufferState Read(
        std::uint32_t field, IUtilitiesRecordTextRuntime& runtime);
    static void Write(std::uint32_t field, const StringBufferState& text,
                      IUtilitiesRecordTextRuntime& runtime);
    static void Assign(std::uint32_t destination, std::uint32_t source,
                       IUtilitiesRecordTextRuntime& runtime);
    static void AssignInline(std::uint32_t destination,
                             std::uint32_t source,
                             IUtilitiesRecordTextRuntime& runtime,
                             bool finish_with_set_size = false);
};

}  // namespace pal4
