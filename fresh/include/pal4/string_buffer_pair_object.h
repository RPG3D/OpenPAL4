#pragma once

#include "pal4/string_buffer.h"

namespace pal4 {

class StringBufferPairObject final {
public:
    static void InitializeAt(std::uint32_t self_address,
                             std::uint32_t source_record,
                             IStringBufferRuntime& runtime);
    static void CleanupAt(std::uint32_t self_address,
                          IStringBufferRuntime& runtime);
    static void DestroyObject(std::uint32_t object_address,
                              IStringBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t AllocateAndInitializeObjects(
        IStringBufferRuntime& runtime);
};

}  // namespace pal4
