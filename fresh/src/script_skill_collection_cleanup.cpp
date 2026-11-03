#include "pal4/script_skill_collection.h"

#include <array>
#include <utility>

namespace pal4 {
namespace {

template <typename Runtime>
std::uint32_t CleanupObjectLists(std::uint32_t object, Runtime& runtime) {
    for (const auto [begin_offset, width] :
         std::array{std::pair{680u, 12u}, std::pair{696u, 8u}}) {
        auto result = runtime.ReadWord(object + begin_offset + 4u);
        const auto begin = runtime.ReadWord(object + begin_offset);
        while (begin != runtime.ReadWord(object + begin_offset + 4u)) {
            const auto record = runtime.ResolveSkillRecord(
                runtime.ReadWord(begin));
            const auto end = runtime.ReadWord(object + begin_offset + 4u);
            for (auto source = begin + width; source != end; source += width)
                for (std::uint32_t offset = 0u; offset < width; offset += 4u)
                    runtime.WriteWord(source - width + offset,
                                      runtime.ReadWord(source + offset));
            runtime.WriteWord(object + begin_offset + 4u, end - width);
            const std::array arguments{record};
            result = runtime.InvokeVirtual(object, 304u, arguments);
        }
        if (begin_offset == 696u) return result;
    }
    return 0u;
}

}  // namespace

std::uint32_t ScriptSkillCollection::CleanupObjectLists(
    std::uint32_t object, IScriptSkillCollectionRuntime& runtime) {
    return pal4::CleanupObjectLists(object, runtime);
}

std::uint32_t ScriptSkillCollection::CleanupObjectLists(
    std::uint32_t object, IScriptSkillListCleanupRuntime& runtime) {
    return pal4::CleanupObjectLists(object, runtime);
}

}  // namespace pal4
