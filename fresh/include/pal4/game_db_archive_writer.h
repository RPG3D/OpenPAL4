#pragma once

#include "pal4/archive_text_writer.h"
#include "pal4/game_db_flag_static.h"
#include "pal4/game_db_record_index.h"

#include <cstdint>

namespace pal4 {

class IGameDbArchiveRuntime : public IArchiveStreamRuntime,
                              public IArchiveTextRuntime {
public:
    virtual ~IGameDbArchiveRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Open(const char* filename,
                                             const char* mode) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFileStream(
        std::uint32_t file) = 0;
    [[nodiscard]] virtual ArchiveTextRecordState TextRecord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyEntry(
        std::uint32_t property, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectProperty(
        std::uint32_t object, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t IntegerProperty(
        std::uint32_t property, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t TextProperty(
        std::uint32_t property, std::uint32_t index) = 0;
    [[nodiscard]] virtual float FloatProperty(
        std::uint32_t property, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FirstProperty(
        const GameDbRecordIndexState& index, std::uint32_t& cursor) = 0;
    [[nodiscard]] virtual std::uint32_t NextProperty(
        const GameDbRecordIndexState& index, std::uint32_t& cursor) = 0;
    [[nodiscard]] virtual bool WriteProperty(
        std::uint32_t property, const ArchiveStreamAdapterState& stream) = 0;
    [[nodiscard]] virtual bool WriteObject(
        std::uint32_t object, const ArchiveStreamAdapterState& stream) = 0;
    virtual void Flush(std::uint32_t file) = 0;
    virtual void Close(std::uint32_t file) = 0;
    virtual void FinishAdapter(const ArchiveStreamAdapterState& stream) = 0;
};

class GameDbArchiveWriter final {
public:
    static constexpr std::uint32_t kFileVersion = 20060228u;
    static constexpr std::uint32_t kPropertyStart = 20060303u;
    static constexpr std::uint32_t kPropertyEnd = 20060304u;
    static constexpr std::uint32_t kObjectStart = 20060225u;
    static constexpr std::uint32_t kObjectEnd = 20060226u;
    static constexpr std::uint32_t kObjectRowStart = 20060223u;
    static constexpr std::uint32_t kObjectRowEnd = 20060224u;
    // IDA 0x651cd0: write the named property tree and object record array.
    [[nodiscard]] static bool WriteToFile(
        const char* filename, const GameDbRecordIndexState& index,
        const GameDbFlagStaticState& flag,
        IGameDbArchiveRuntime& runtime);
    // IDA 0x651e90: write a property's marker, name, entries and terminator.
    [[nodiscard]] static bool WritePropertyRecord(
        std::uint32_t property, const ArchiveStreamAdapterState& stream,
        IGameDbArchiveRuntime& runtime);
    // IDA 0x6521d0: write typed object property rows.
    [[nodiscard]] static bool WriteObjectRecord(
        std::uint32_t object, const ArchiveStreamAdapterState& stream,
        IGameDbArchiveRuntime& runtime);
};

}  // namespace pal4
