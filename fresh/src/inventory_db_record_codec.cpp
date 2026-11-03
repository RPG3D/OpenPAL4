#include "pal4/inventory_db_record_codec.h"

namespace pal4 {

bool InventoryDbRecordCodec::Read(
    const std::uint32_t record, const std::uint32_t stream,
    IInventoryDbRecordRuntime& runtime) {
    runtime.WriteWord(record + 4u, runtime.ReadStreamWord(stream));
    runtime.WriteWord(record + 8u, runtime.ReadStreamWord(stream));
    const auto id = runtime.ReadStreamWord(stream);
    const auto game_data = runtime.FindGameData(id);
    runtime.WriteWord(record, game_data);
    return game_data != 0u;
}

std::uint32_t InventoryDbRecordCodec::Write(
    const std::uint32_t record, const std::uint32_t stream,
    IInventoryDbRecordRuntime& runtime) {
    (void)runtime.WriteStreamWord(stream, runtime.ReadWord(record + 4u));
    (void)runtime.WriteStreamWord(stream, runtime.ReadWord(record + 8u));
    const auto game_data = runtime.ReadWord(record);
    return runtime.WriteStreamWord(stream, runtime.ReadWord(game_data));
}

}  // namespace pal4
