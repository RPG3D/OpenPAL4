#include "pal4/game_database_data.h"

#include <array>

namespace pal4 {
namespace {

struct HelperCall {
    std::uint32_t offset;
    std::uint32_t function;
};

constexpr std::array<HelperCall, 11> kRegister{{
    {0u, 0x4e56a0u}, {12u, 0x4f3b40u}, {32u, 0x4e99a0u},
    {52u, 0x5013a0u}, {88u, 0x4d5400u}, {128u, 0x4e2990u},
    {140u, 0x4db190u}, {148u, 0x4fae20u}, {164u, 0x4deb00u},
    {172u, 0x4cbef0u}, {184u, 0x4cfc90u},
}};

constexpr std::array<HelperCall, 11> kVerify{{
    {0u, 0x4e56c0u}, {12u, 0x4f3b80u}, {32u, 0x4e99e0u},
    {52u, 0x501440u}, {88u, 0x4d5430u}, {128u, 0x4e29b0u},
    {140u, 0x4db1b0u}, {148u, 0x4fae60u}, {164u, 0x4deb20u},
    {172u, 0x4cbf20u}, {184u, 0x4cfcd0u},
}};

}  // namespace

std::uint8_t GameDatabaseData::InitializeFromDatabase(
    const std::uint32_t object, const std::uint32_t path,
    IGameDatabaseDataRuntime& runtime) {
    runtime.ResetManager(runtime.ReadWord(object + 324u));
    for (const auto& call : kRegister)
        runtime.CallHelper(call.function, object + call.offset);
    const auto loaded = runtime.LoadDatabase(
        runtime.ReadWord(object + 324u), path, kCallbackTable);
    if (loaded == 0u) return loaded;
    for (const auto& call : kVerify)
        runtime.CallHelper(call.function, object + call.offset);
    static_cast<void>(InitializeShopData(object, kShopPath, runtime));
    return 1u;
}

std::uint8_t GameDatabaseData::InitializeShopData(
    const std::uint32_t object, const std::uint32_t path,
    IGameDatabaseDataRuntime& runtime) {
    runtime.ResetManager(runtime.ReadWord(object + 328u));
    runtime.CallHelper(0x4eef50u, object + 108u);
    const auto loaded = runtime.LoadDatabase(
        runtime.ReadWord(object + 328u), path, kCallbackTable);
    if (loaded != 0u) {
        runtime.CallHelper(0x4eef80u, object + 108u);
        return 1u;
    }
    return loaded;
}

std::uint32_t GameDatabaseData::Cleanup(
    const std::uint32_t object, IGameDatabaseDataRuntime& runtime) {
    runtime.ResetManager(runtime.ReadWord(object + 324u));
    runtime.ResetManager(runtime.ReadWord(object + 328u));
    for (const auto offset : {112u, 116u, 120u, 124u})
        runtime.WriteWord(object + offset, 0u);
    return 0u;
}

std::uint32_t GameDatabaseData::FindStringValue(
    const std::uint32_t object, const std::uint32_t text,
    IGameDatabaseDataRuntime& runtime) {
    return runtime.FindStringValue(runtime.ReadWord(object + 324u), text);
}

}  // namespace pal4
