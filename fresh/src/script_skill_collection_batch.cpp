#include "pal4/script_skill_collection.h"

#include <array>
#include <utility>

namespace pal4 {

std::uint32_t ScriptSkillCollection::ProcessSkillsBatch(
    std::uint32_t object, std::uint32_t requested_id,
    std::uint32_t target, IScriptSkillContextRuntime& runtime) {
    for (const auto [begin_offset, width] :
         std::array{std::pair{696u, 8u}, std::pair{680u, 12u}}) {
        for (auto entry = runtime.ReadWord(object + begin_offset);
             entry != runtime.ReadWord(object + begin_offset + 4u);
             entry += width) {
            const ScriptSkillContextState item{
                runtime.ReadWord(entry), runtime.ReadWord(entry + 4u)};
            static_cast<void>(ScriptSkillContext::Handle(
                item, requested_id, object, target, runtime));
        }
    }
    return runtime.ReadWord(object + 684u);
}

}  // namespace pal4
