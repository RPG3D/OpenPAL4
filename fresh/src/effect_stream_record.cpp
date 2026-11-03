#include "pal4/effect_stream_record.h"

namespace pal4 {

void EffectStreamRecordReader::Read(
    EffectStreamRecord& record, const std::uint32_t context,
    IEffectStreamRecordRuntime& runtime) {
    if (context == 0u) return;
    runtime.ReadData(context, 0u, 4u, record);
    runtime.ReadString(context, 1u, record);
    runtime.ReadData(context, 4u, 4u, record);
    runtime.ReadData(context, 5u, 4u, record);
    runtime.ReadString(context, 6u, record);
    runtime.ReadString(context, 9u, record);
    runtime.ReadString(context, 12u, record);
}

}  // namespace pal4
