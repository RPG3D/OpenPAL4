#include "pal4/monster_manager_finder.h"
#include "monster_manager_temporary_text.h"

namespace pal4 {

std::uint32_t MonsterManagerFindRuntimeStub::CStringLength(
    std::uint32_t) { return 0u; }
void MonsterManagerFindRuntimeStub::SetErrorFlag(
    ModeStringBufferRecord& text, std::uint8_t) { text.buffer = {}; }
bool MonsterManagerFindRuntimeStub::AllocateText(
    ModeStringBufferRecord&, std::uint32_t, std::uint8_t) { return false; }
void MonsterManagerFindRuntimeStub::CopyBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MonsterManagerFindRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
std::uint8_t MonsterManagerFindRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void MonsterManagerFindRuntimeStub::FreeTextHeader(std::uint32_t) {}
std::uint32_t MonsterManagerFindRuntimeStub::SearchNodeAlternative(
    std::uint32_t, const ModeStringBufferRecord&) { return 0u; }
std::int32_t MonsterManagerFindRuntimeStub::CompareName(
    const ModeStringBufferRecord&, std::uint32_t) { return 0; }
std::uint32_t MonsterManagerFindRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }

std::uint32_t MonsterManagerFinder::FindByName(
    std::uint32_t manager, std::uint32_t name_address,
    IMonsterManagerFindRuntime& runtime) {
    ModeStringBufferRecord name{};
    name.mode_word = name_address & 0xffu;
    runtime.SetErrorFlag(name, 0u);
    MonsterManagerTemporaryText name_owner{name, runtime};
    const auto length = runtime.CStringLength(name_address);
    if (runtime.AllocateText(name, length, 1u)) {
        runtime.CopyBytes(name.buffer.data, name_address, length);
        name.buffer.length = length;
        runtime.WriteByte(name.buffer.data + length, 0u);
    }

    const auto tree = manager + 0x28u;
    const auto candidate = runtime.SearchNodeAlternative(tree, name);
    const auto header = runtime.ReadDword(tree + 4u);
    const auto selected = candidate == header ||
                                  runtime.CompareName(name, candidate + 0x0cu) < 0
                              ? header : candidate;
    const auto result = selected == header ? 0u :
        runtime.ReadDword(selected + 0x1cu);
    name_owner.Release();
    return result;
}

}  // namespace pal4
