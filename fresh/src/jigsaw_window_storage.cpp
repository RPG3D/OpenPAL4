#include "pal4/jigsaw_window_storage.h"

#include <bit>
#include <initializer_list>

namespace pal4 {
namespace {

void ClearBoardRecords(const std::uint32_t puzzle,
                       IJigsawWindowStorageRuntime& runtime) {
    runtime.DestroyString(puzzle + 0x98u);
    const auto begin = runtime.ReadWord(puzzle + 0x1cu);
    const auto end = runtime.ReadWord(puzzle + 0x20u);
    for (auto record = begin; record != end; record += 0xc8u)
        runtime.DestroyString(record + 0x10u);
    (void)runtime.FreeZero(begin);
    for (const auto offset : {0x1cu, 0x20u, 0x24u})
        runtime.WriteWord(puzzle + offset, 0u);
}

void ClearGroupWords(const std::uint32_t group,
                     IJigsawWindowStorageRuntime& runtime) {
    for (const auto offset : {4u, 8u, 0xcu})
        runtime.WriteWord(group + offset, 0u);
}

void ClearVectorWords(const std::uint32_t vector,
                      IJigsawWindowStorageRuntime& runtime) {
    for (const auto offset : {4u, 8u, 0xcu})
        runtime.WriteWord(vector + offset, 0u);
}

}  // namespace

std::uint32_t JigsawWindowStorage::Cleanup(
    const std::uint32_t puzzle,
    IJigsawWindowStorageRuntime& runtime) {
    ClearBoardRecords(puzzle, runtime);
    const auto begin = runtime.ReadWord(puzzle + 4u);
    const auto end = runtime.ReadWord(puzzle + 8u);
    for (auto group = begin; group != end; group += 0x10u) {
        const auto cells = runtime.ReadWord(group + 4u);
        const auto cells_end = runtime.ReadWord(group + 8u);
        for (auto cell = cells; cell != cells_end; cell += 4u)
            runtime.TouchCell(group, cell);
        runtime.FreeRaw(cells);
        ClearGroupWords(group, runtime);
    }
    const auto result = runtime.FreeZero(begin);
    ClearVectorWords(puzzle, runtime);
    return result;
}

std::uint32_t JigsawWindowStorage::Destruct(
    const std::uint32_t frame,
    IJigsawWindowStorageRuntime& runtime) {
    const auto puzzle = frame + 0x2cu;
    ClearBoardRecords(puzzle, runtime);
    const auto begin = runtime.ReadWord(puzzle + 4u);
    const auto end = runtime.ReadWord(puzzle + 8u);
    for (auto group = begin; group != end; group += 0x10u) {
        const auto cells = runtime.ReadWord(group + 4u);
        const auto cells_end = runtime.ReadWord(group + 8u);
        const auto capacity = runtime.ReadWord(group + 0xcu);
        runtime.TouchRange(cells, cells_end);
        runtime.FreeSized(cells,
                          std::bit_cast<std::int32_t>(capacity - cells) >> 2);
        ClearGroupWords(group, runtime);
    }
    (void)runtime.FreeZero(begin);
    ClearVectorWords(puzzle, runtime);
    return runtime.DestroyBase(frame);
}

std::uint32_t JigsawWindowStorage::ClearGroups(
    const std::uint32_t vector,
    IJigsawWindowStorageRuntime& runtime) {
    const auto begin = runtime.ReadWord(vector + 4u);
    const auto end = runtime.ReadWord(vector + 8u);
    for (auto group = begin; group != end; group += 0x10u) {
        const auto cells = runtime.ReadWord(group + 4u);
        const auto cells_end = runtime.ReadWord(group + 8u);
        for (auto cell = cells; cell != cells_end; cell += 4u)
            runtime.TouchNoop();
        (void)runtime.FreeZero(cells);
        ClearGroupWords(group, runtime);
    }
    (void)runtime.FreeZero(begin);
    ClearVectorWords(vector, runtime);
    return 0u;
}

}  // namespace pal4
