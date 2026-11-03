#include "pal4/camera_task_object.h"

namespace pal4 {

std::uint32_t CameraTaskFactory::Create(
    ICameraTaskFactoryRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x98u);
    if (memory == 0u) return 0u;
    try {
        return CameraTaskObject::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

std::uint32_t CameraTaskObject::Construct(
    const std::uint32_t object, ICameraTaskObjectRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8486a0u);
    runtime.WriteByte(object + 0x3cu, 0u);
    for (std::uint32_t offset = 0x40u; offset <= 0x4cu; offset += 4u)
        runtime.WriteWord(object + offset, 0u);
    return object;
}

std::uint32_t CameraTaskObject::Destruct(
    const std::uint32_t object, ICameraTaskObjectRuntime& runtime) {
    runtime.WriteWord(object, 0x8486a0u);
    return runtime.BaseDestruct(object);
}

std::uint32_t CameraTaskObject::Configure(
    const std::uint32_t object, const std::uint32_t property,
    const std::uint32_t camera_record, ICameraTaskObjectRuntime& runtime) {
    runtime.WriteWord(object + 0x48u, property);
    const auto record_field13 = runtime.ReadWord(camera_record + 0x34u);
    runtime.WriteWord(object + 0x4cu, camera_record);
    runtime.WriteWord(object + 0x44u, record_field13);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    return 0u;
}

}  // namespace pal4
