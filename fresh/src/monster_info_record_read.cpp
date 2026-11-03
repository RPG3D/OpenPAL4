#include "pal4/monster_info_manager.h"
#include <limits>

namespace pal4 {

void MonsterInfoRecord::Read(const std::uint32_t record,
                             const std::uint32_t stream,
                             INpcInfoRuntime& runtime) {
    runtime.ReadStreamString(stream, record);
    runtime.ReadStreamString(stream, record + 12u);
    runtime.ReadStreamData(stream, record + 24u, 12u);
    runtime.ReadStreamData(stream, record + 36u, 12u);
    for (auto offset = 48u; offset <= 64u; offset += 4u)
        runtime.ReadStreamData(stream, record + offset, 4u);

    const auto count = static_cast<std::int32_t>(runtime.ReadWord(record + 64u));
    if (count > 0 &&
        static_cast<std::uint32_t>(count) <=
            std::numeric_limits<std::uint32_t>::max() / 4u) {
        const auto old_array = runtime.ReadWord(record + 68u);
        if (old_array) {
            runtime.Free(old_array);
            runtime.WriteWord(record + 68u, 0);
        }
        const auto array = runtime.Allocate(static_cast<std::uint32_t>(count) * 4u);
        runtime.WriteWord(record + 68u, array);
        if (array) {
            for (auto index = 0; index < count; ++index)
                runtime.ReadStreamData(stream, array +
                    static_cast<std::uint32_t>(index) * 4u, 4u);
        }
    }
    const auto kind = runtime.StreamKind(stream);
    if (kind == 1u || kind == 2u) {
        const auto facet = runtime.CreateLocaleFacet(runtime.StreamLocale(stream));
        runtime.CheckXml(facet, runtime.ReadWord(record + 72u));
        runtime.CheckXml(facet, runtime.ReadWord(record + 76u));
        runtime.DestroyLocaleFacet(facet);
    }
}

} // namespace pal4
