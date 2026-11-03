#include "pal4/world_path_planner.h"

#include <limits>

namespace pal4 {

std::uint32_t PathwayRecord::Load(const std::uint32_t record,
                                  const std::uint32_t stream,
                                  IWorldPathRuntime& runtime) {
    (void)runtime.Call(WorldPathCall::ReadString, {stream, record});
    if (runtime.ReadStreamWord(stream) == 0u) return 0u;

    const auto variant = runtime.ReadStreamWord(stream);
    const auto kind = runtime.ReadStreamByte(stream);
    const auto count = runtime.ReadStreamWord(stream);
    if (count > std::numeric_limits<std::uint32_t>::max() / 12u)
        return 0u;
    const auto records = runtime.Allocate(count * 12u);
    if (records == 0u && count != 0u) return 0u;
    std::uint32_t grid_storage = 0u;
    bool constructed = false;
    try {
        for (auto index = 0u; index < count; ++index) {
            const auto item = records + index * 12u;
            for (const auto offset : {0u, 4u, 8u})
                runtime.WriteWord(item + offset, 0u);
            runtime.ReadStreamRecord(stream, item);
        }

        grid_storage = runtime.Allocate(0x58u);
        if (grid_storage == 0u) {
            (void)runtime.Free(records);
            return 0u;
        }
        const auto grid = runtime.Call(WorldPathCall::ConstructGrid,
                                       {grid_storage, 1u});
        constructed = true;
        runtime.WriteWord(record + 12u, grid);
        (void)runtime.Call(WorldPathCall::InitializeGrid,
                           {grid, count, records, variant, kind});
    } catch (...) {
        if (constructed) (void)Release(record, runtime);
        else if (grid_storage != 0u) (void)runtime.Free(grid_storage);
        (void)runtime.Free(records);
        throw;
    }
    return runtime.Free(records);
}

std::uint32_t PathwayRecord::Release(
    const std::uint32_t record, IWorldPathRuntime& runtime) {
    const auto grid = runtime.ReadWord(record + 12u);
    const auto result = grid == 0u ? 0u :
        runtime.Call(WorldPathCall::DestroyGrid, {grid, 1u});
    runtime.WriteWord(record + 12u, 0u);
    return result;
}

std::uint32_t PathwayRecord::CleanupText(
    const std::uint32_t record, IWorldPathRuntime& runtime) {
    return runtime.Call(WorldPathCall::CleanupText, {record});
}

std::uint32_t WorldPathPlanner::Register(
    std::uint8_t& guard, std::uint32_t& registered,
    IWorldPathRuntime& runtime) {
    if ((guard & 1u) != 0u) return registered;
    guard |= 1u;
    registered = runtime.RegisterSingleton(kSingletonName, kSingletonId,
                                           0x522e30u, 0x522e90u);
    return registered;
}

std::uint32_t WorldPathPlanner::Create(IWorldPathRuntime& runtime) {
    const auto planner = runtime.Allocate(16u);
    if (planner == 0u) return 0u;
    runtime.WriteByte(planner, static_cast<std::uint8_t>(planner >> 24u));
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(planner + offset, 0u);
    return planner;
}

std::uint32_t WorldPathPlanner::Destroy(
    const std::uint32_t planner, IWorldPathRuntime& runtime) {
    if (planner == 0u) return 0u;
    (void)CleanupData(planner, runtime);
    return runtime.Free(planner);
}

std::uint32_t WorldPathPlanner::CleanupData(
    const std::uint32_t planner, IWorldPathRuntime& runtime) {
    (void)runtime.Free(runtime.ReadWord(planner + 4u));
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(planner + offset, 0u);
    return 0u;
}

std::uint32_t WorldPathPlanner::CleanupPathList(
    const std::uint32_t planner, IWorldPathRuntime& runtime) {
    for (auto cursor = runtime.ReadWord(planner + 4u);
         cursor != runtime.ReadWord(planner + 8u); cursor += 4u) {
        const auto record = runtime.ReadWord(cursor);
        if (record != 0u) {
            (void)PathwayRecord::Release(record, runtime);
            (void)PathwayRecord::CleanupText(record, runtime);
            (void)runtime.Free(record);
        }
    }
    const auto old_end = runtime.ReadWord(planner + 8u);
    runtime.WriteWord(planner + 8u, runtime.ReadWord(planner + 4u));
    return old_end;
}

std::uint32_t WorldPathPlanner::Find(
    const std::uint32_t planner, const std::uint32_t name,
    IWorldPathRuntime& runtime) {
    for (auto cursor = runtime.ReadWord(planner + 4u);
         cursor != runtime.ReadWord(planner + 8u); cursor += 4u) {
        const auto record = runtime.ReadWord(cursor);
        if (record == 0u) continue;
        const auto text = runtime.ReadWord(record);
        const auto length = runtime.ReadWord(record + 4u);
        auto offset = 0u;
        for (; offset <= length; ++offset) {
            if (runtime.ReadByte(text + offset) !=
                runtime.ReadByte(name + offset))
                break;
        }
        if (offset > length) return record;
    }
    return 0u;
}

bool WorldPathPlanner::LoadPathwayFile(
    const std::uint32_t planner, const std::string_view directory,
    const std::string_view name, IWorldPathRuntime& runtime) {
    const auto stream = runtime.OpenPathwayStream(directory, name);
    if (stream == 0u) return false;
    std::uint32_t pending = 0u;
    try {
        (void)CleanupPathList(planner, runtime);
        const auto count = runtime.ReadStreamWord(stream);
        for (auto index = 0u; index < count; ++index) {
            const auto record = runtime.Allocate(20u);
            pending = record;
            if (record == 0u) {
                runtime.ClosePathwayStream(stream);
                return false;
            }
            for (auto offset = 0u; offset < 20u; offset += 4u)
                runtime.WriteWord(record + offset, 0u);
            (void)runtime.Call(WorldPathCall::InitializeRecordText, {record});
            (void)PathwayRecord::Load(record, stream, runtime);

            auto begin = runtime.ReadWord(planner + 4u);
            auto end = runtime.ReadWord(planner + 8u);
            if (end == runtime.ReadWord(planner + 12u)) {
                const auto size = begin == 0u ? 0u : (end - begin) / 4u;
                const auto capacity = size + (size > 1u ? size : 1u);
                const auto replacement = runtime.Allocate(capacity * 4u);
                if (replacement == 0u) {
                    (void)PathwayRecord::Release(record, runtime);
                    (void)PathwayRecord::CleanupText(record, runtime);
                    (void)runtime.Free(record);
                    pending = 0u;
                    runtime.ClosePathwayStream(stream);
                    return false;
                }
                for (auto entry = 0u; entry < size; ++entry)
                    runtime.WriteWord(replacement + entry * 4u,
                                      runtime.ReadWord(begin + entry * 4u));
                (void)runtime.Free(begin);
                begin = replacement;
                end = replacement + size * 4u;
                runtime.WriteWord(planner + 4u, begin);
                runtime.WriteWord(planner + 12u,
                                  replacement + capacity * 4u);
            }
            runtime.WriteWord(end, record);
            runtime.WriteWord(planner + 8u, end + 4u);
            pending = 0u;
        }
        runtime.ClosePathwayStream(stream);
    } catch (...) {
        if (pending != 0u) {
            (void)PathwayRecord::Release(pending, runtime);
            (void)PathwayRecord::CleanupText(pending, runtime);
            (void)runtime.Free(pending);
        }
        // Preserve the read/allocation error if closing an incomplete stream fails.
        try { runtime.ClosePathwayStream(stream); } catch (...) {}
        throw;
    }
    return true;
}

}  // namespace pal4
