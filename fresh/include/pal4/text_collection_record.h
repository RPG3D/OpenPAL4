#pragma once

#include "pal4/string_buffer.h"

#include <array>
#include <cstdint>
#include <optional>

namespace pal4 {

struct TextCollectionRecord {
    StringBufferState text{};
    std::array<std::uint32_t, 3> attributes{};
};

class ITextCollectionRecordRuntime {
public:
    virtual ~ITextCollectionRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual TextCollectionRecord& Access(
        std::uint32_t address) = 0;
    virtual void InitializeEmpty(StringBufferState& text) = 0;
    virtual void AssignText(StringBufferState& text, std::uint32_t source,
                            std::uint32_t length) = 0;
    virtual void CleanupText(StringBufferState& text) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class TextCollectionRecordLifecycle final {
public:
    [[nodiscard]] static std::uint32_t InitializeFromRecord(
        TextCollectionRecord& destination,
        const TextCollectionRecord& source,
        ITextCollectionRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        ITextCollectionRecordRuntime& runtime);
    [[nodiscard]] static std::optional<std::uint32_t> Destroy(
        std::uint32_t address, ITextCollectionRecordRuntime& runtime);
};

}  // namespace pal4
