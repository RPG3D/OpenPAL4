#pragma once

#include "pal4/game_render_system.h"
#include "pal4/text_collection_record.h"

#include <array>
#include <cstdint>

namespace pal4 {

using DebugTextRecord = TextCollectionRecord;

struct DebugTextGlobals {
    std::uint8_t string_guard = 0;
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

class IDebugTextRuntime {
public:
    virtual ~IDebugTextRuntime() = default;
    virtual void InitializeTemporary(StringBufferState& text) = 0;
    virtual void AssignTemporary(StringBufferState& text, const char* source,
                                 std::uint32_t length) = 0;
    virtual void Insert(std::uint32_t collection,
                        const DebugTextRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporary(
        StringBufferState& text) = 0;
    [[nodiscard]] virtual std::int32_t GetAndClearProperty(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t ClearCollection(
        std::uint32_t collection) = 0;
    virtual void InitializeStaticString(std::uint32_t address) = 0;
    virtual void RegisterCleanup(std::uint32_t callback_address) = 0;
    [[nodiscard]] virtual std::uint32_t FirstElement(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceElement(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t ReadTextPointer(
        std::uint32_t element) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFontManager() = 0;
    virtual void DrawFontRectangle(std::uint32_t font,
                                   std::uint32_t text,
                                   const DebugTextGlobals& rectangle,
                                   std::uint32_t flags,
                                   std::uint32_t color,
                                   std::uint32_t size) = 0;
    virtual void CleanupStaticString(std::uint32_t address) = 0;
};

class DebugTextCollection final {
public:
    static constexpr std::uint32_t kStaticStringAddress = 0x8e3dccu;
    static constexpr std::uint32_t kCleanupCallbackAddress = 0x409fd0u;
    static constexpr std::uint32_t kStringGuardAddress = 0x8e3dd8u;

    DebugTextCollection(GameRenderState& game, std::uint32_t collection,
                        DebugTextGlobals& globals)
        : game_(game), collection_(collection), globals_(globals) {}

    [[nodiscard]] std::uint32_t Add(const char* text,
                                     std::uint32_t first,
                                     std::uint32_t second,
                                     std::uint32_t third,
                                     IDebugTextRuntime& runtime);
    [[nodiscard]] std::uint32_t ClearIfPending(IDebugTextRuntime& runtime);
    [[nodiscard]] std::uint32_t Draw(std::uint32_t ignored,
                                      IDebugTextRuntime& runtime);
    static void CleanupStaticString(IDebugTextRuntime& runtime);

private:
    GameRenderState& game_;
    std::uint32_t collection_;
    DebugTextGlobals& globals_;
};

}  // namespace pal4
