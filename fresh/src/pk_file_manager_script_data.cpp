#include "pal4/pk_file_manager.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kPageBytes = 4096u;

}  // namespace

std::uint32_t PkFileManager::AddScriptData(
    const std::uint32_t object, const std::uint32_t value) {
    auto& memory = runtime_.Scripts();
    const auto file = memory.ReadWord(object);
    if (file == 0 || value == 0) return file;

    auto count = memory.ReadWord(object + 120u);
    std::uint32_t destination = 0;
    if (count != 0 && memory.ReadWord(object + 104u) !=
                          memory.ReadWord(object + 100u)) {
        destination = memory.ReadWord(object + 104u);
        memory.WriteWord(object + 104u, destination + 4u);
        if (destination + 4u == memory.ReadWord(object + 100u) &&
            AllocateScriptDataPage(object + 76u, memory) == 0u) {
            memory.WriteWord(object + 104u, destination);
            return 0;
        }
    } else {
        if (AllocateScriptDataPage(object + 76u, memory) == 0u) return 0;
        destination = memory.ReadWord(object + 104u);
        memory.WriteWord(object + 104u, destination + 4u);
    }
    memory.WriteWord(destination, value);
    memory.WriteWord(object + 120u, ++count);
    return count;
}

std::uint32_t PkFileManager::RemoveScriptData(
    const std::uint32_t object) {
    auto& memory = runtime_.Scripts();
    auto count = memory.ReadWord(object + 120u);
    if (count == 0) return 0;

    if (memory.ReadWord(object + 104u) ==
        memory.ReadWord(object + 96u)) {
        const auto old_cursor = memory.ReadWord(object + 108u);
        const auto cursor = old_cursor - 4u;
        memory.WriteWord(object + 108u, cursor);
        (void)memory.ReleaseMemory(memory.ReadWord(old_cursor));
        const auto page = memory.ReadWord(cursor);
        (void)SetScriptDataPageIterator(
            object + 96u, page + kPageBytes, cursor, memory);
    }
    memory.WriteWord(object + 104u,
                     memory.ReadWord(object + 104u) - 4u);
    memory.WriteWord(object + 120u, --count);
    if (count != 0) return count;

    const auto old_cursor = memory.ReadWord(object + 108u);
    const auto cursor = old_cursor - 4u;
    memory.WriteWord(object + 108u, cursor);
    (void)memory.ReleaseMemory(memory.ReadWord(old_cursor));
    const auto front_cursor = memory.ReadWord(object + 92u);
    if (front_cursor == cursor)
        (void)memory.ReleaseMemory(memory.ReadWord(front_cursor));
    for (std::uint32_t offset = 80u; offset <= 108u; offset += 4u)
        memory.WriteWord(object + offset, 0u);
    return memory.ReleaseMemory(memory.ReadWord(object + 112u));
}

}  // namespace pal4
