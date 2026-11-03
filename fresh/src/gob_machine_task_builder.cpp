#include "pal4/gob_machine.h"
#include "pal4/gob_machine_task_runtime.h"

#include <array>

namespace pal4 {
namespace {

void AddInt(GobMachineTaskDefinition& task,
            const std::uint32_t node, const std::string_view key,
            const std::int32_t fallback,
            IGobMachineTaskRuntime& runtime) {
    task.parameters.push_back({std::string(key),
        runtime.ReadXmlInt(node, key, fallback)});
}

void AddFloat(GobMachineTaskDefinition& task,
              const std::uint32_t node, const std::string_view key,
              const float fallback, IGobMachineTaskRuntime& runtime) {
    task.parameters.push_back({std::string(key),
        runtime.ReadXmlFloat(node, key, fallback)});
}

void AddString(GobMachineTaskDefinition& task,
               const std::uint32_t node, const std::string_view key,
               IGobMachineTaskRuntime& runtime) {
    task.parameters.push_back({std::string(key),
        runtime.ReadXmlString(node, key, "")});
}

bool ReadTaskParameters(GobMachineTaskDefinition& task,
                        const std::uint32_t node,
                        IGobMachineTaskRuntime& runtime) {
    const auto i = [&](std::string_view key, std::int32_t fallback = 0) {
        AddInt(task, node, key, fallback, runtime);
    };
    const auto f = [&](std::string_view key, float fallback = 0.0f) {
        AddFloat(task, node, key, fallback, runtime);
    };
    const auto s = [&](std::string_view key) {
        AddString(task, node, key, runtime);
    };
    switch (task.type) {
    case 0:
        f("PAL4_GOMTask_move_speed");
        f("PAL4_GOMTask_move_target_X");
        f("PAL4_GOMTask_move_target_Y");
        f("PAL4_GOMTask_move_target_Z");
        s("PAL4_GOMTask_move_sound_name");
        i("PAL4_GOMTask_move_sound_time");
        s("PAL4_GOMTask_move_effect_name");
        i("PAL4_GOMTask_move_effect_time");
        return true;
    case 1:
        f("PAL4_GOMTask_rotate_speed");
        f("PAL4_GOMTask_rotate_target_X");
        f("PAL4_GOMTask_rotate_target_Y");
        f("PAL4_GOMTask_rotate_target_Z");
        s("PAL4_GOMTask_rotate_sound_name");
        i("PAL4_GOMTask_rotate_sound_time");
        s("PAL4_GOMTask_rotate_effect_name");
        i("PAL4_GOMTask_rotate_effect_time");
        return true;
    case 2:
        i("PAL4_GOMTask_script_wait");
        s("PAL4_GOMTask_script_func");
        return true;
    case 3:
        i("PAL4_GOMTask_action_time", 1);
        s("PAL4_GOMTask_action_name");
        return true;
    case 6:
        return true;
    case 7:
    case 8:
        i("PAL4_GOMTask_getitem_id");
        i("PAL4_GOMTask_getitem_num");
        return true;
    case 9:
        i("PAL4_GOMTask_getitem_money");
        return true;
    case 10:
        f("PAL4_GOMTask_movment_tPosX");
        f("PAL4_GOMTask_movment_tPosY");
        f("PAL4_GOMTask_movment_tPosZ");
        f("PAL4_GOMTask_movment_tRotX");
        f("PAL4_GOMTask_movment_tRotY");
        f("PAL4_GOMTask_movment_tRotZ");
        s("PAL4_GOMTask_movment_sound_name");
        i("PAL4_GOMTask_movment_sound_time");
        s("PAL4_GOMTask_movment_effect_name");
        i("PAL4_GOMTask_movment_effect_time");
        return true;
    case 11:
        i("PAL4_GOMTask_camera_autoseek");
        s("PAL4_GOMTask_camera_filename");
        return true;
    case 12:
    case 13:
    case 14:
        return true;
    case 15:
        i("PAL4_GOMTask_active_object_ctrl");
        s("PAL4_GOMTask_active_object_name");
        return true;
    case 16:
        f("PAL4_GOMTask_start_posX");
        f("PAL4_GOMTask_start_posY");
        f("PAL4_GOMTask_start_posZ");
        f("PAL4_GOMTask_start_rotX");
        f("PAL4_GOMTask_start_rotY");
        f("PAL4_GOMTask_start_rotZ");
        return true;
    case 17:
        i("PAL4_GOMTask_touch_other_wait");
        s("PAL4_GOMTask_touch_other_name");
        return true;
    case 18:
        s("PAL4_GOMTask_even_rotate_sound_name");
        i("PAL4_GOMTask_even_rotate_sound_time");
        s("PAL4_GOMTask_even_rotate_effect_name");
        i("PAL4_GOMTask_even_rotate_effect_time");
        f("PAL4_GOMTask_even_rotate_speedY", 10.0f);
        return true;
    case 19: {
        const float minimum = runtime.ReadXmlFloat(
            node, "PAL4_GOMTask_time_wait_minval", 0.0f);
        const float maximum = runtime.ReadXmlFloat(
            node, "PAL4_GOMTask_time_wait_maxval", 1.0f);
        task.parameters.push_back(
            {"PAL4_GOMTask_time_wait_minval", minimum});
        task.parameters.push_back(
            {"PAL4_GOMTask_time_wait_maxval", maximum});
        return true;
    }
    case 20:
        f("PAL4_GOMTask_camera_quake_frequency", 1.0f);
        f("PAL4_GOMTask_camera_quake_amplitude", 1.0f);
        f("PAL4_GOMTask_camera_quake_time", 1.0f);
        return true;
    case 21:
        return true;
    case 22:
        f("CSTR_GOMTask_moveAndSelfRot_target_X");
        f("CSTR_GOMTask_moveAndSelfRot_target_Y");
        f("CSTR_GOMTask_moveAndSelfRot_target_Z");
        f("CSTR_GOMTask_moveAndSelfRot_speed_X");
        f("CSTR_GOMTask_moveAndSelfRot_speed_Y");
        f("CSTR_GOMTask_moveAndSelfRot_speed_Z");
        s("PAL4_GOMTask_moveAndSelfRot_sound_name");
        i("CSTR_GOMTask_moveAndSelfRot_sound_time");
        s("CSTR_GOMTask_moveAndSelfRot_effect_name");
        i("CSTR_GOMTask_moveAndSelfRot_effect_time");
        i("CSTR_GOMTask_moveAndSelfRot_direct");
        f("CSTR_GOMTask_moveAndSelfRot_directDist");
        return true;
    default:
        return false;
    }
}

bool CreateBeforeParameters(const std::int32_t type) {
    switch (type) {
    case 2:
    case 6:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 17:
    case 20:
    case 21:
        return true;
    default:
        return false;
    }
}

void RegisterCreatedTask(const std::uint32_t object,
                         const std::uint32_t task,
                         const GobMachineTaskDefinition& definition,
                         IGobMachineTaskRuntime& runtime) {
    if (task != 0u)
        runtime.RegisterTask(object + GobMachine::kPrimaryTasksOffset,
                             task, definition);
}

void SaveTransform(const std::uint32_t object,
                   IGobMachineTaskRuntime& runtime) {
    for (std::uint32_t axis = 0; axis < 3u; ++axis)
        runtime.WriteWord(object + 0x288u + axis * 4u,
                          runtime.ReadWord(object + 0xacu + axis * 4u));
    for (std::uint32_t axis = 0; axis < 3u; ++axis)
        runtime.WriteWord(object + 0x294u + axis * 4u,
                          runtime.ReadWord(object + 0xa0u + axis * 4u));
}

void SaveStartTransform(const std::uint32_t object,
                        const GobMachineTaskDefinition& task,
                        IGobMachineTaskRuntime& runtime) {
    for (std::uint32_t index = 0; index < 6u; ++index) {
        const auto value = std::get<float>(task.parameters[index].value);
        runtime.WriteFloat(object + 0x288u + index * 4u, value);
    }
}

}  // namespace

std::uint32_t GobMachine::BuildAndStartTaskRun(
    const std::uint32_t object, IGobMachineTaskRuntime& runtime) {
    SaveTransform(object, runtime);
    runtime.ClearPrimaryTasks(object + kPrimaryTasksOffset);
    runtime.EraseTaskPointerRange(object + 0x2a0u,
                                 runtime.ReadWord(object + 0x2a4u),
                                 runtime.ReadWord(object + 0x2a8u));
    const auto document = runtime.GetMachineConfigNode(object);
    if (document == 0u) return 0u;
    const auto tasks = runtime.FindXmlChild(
        document, "PAL4_GameObject-machine-tasks");
    if (tasks == 0u) return 0u;
    const auto count = runtime.XmlChildCount(tasks);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto node = runtime.XmlChildAt(tasks, index);
        if (node == 0u) return 0u;
        GobMachineTaskDefinition definition;
        definition.type = runtime.ReadXmlInt(
            node, "PAL4_GOMTask_type", -1);
        definition.wait_mode = runtime.ReadXmlInt(
            node, "PAL4_GOMTask_wait_mode", 0);
        definition.wait_time = runtime.ReadXmlFloat(
            node, "PAL4_GOMTask_wait_time", 0.0f);
        definition.expect_time = runtime.ReadXmlFloat(
            node, "PAL4_GOMTask_expt_time", 0.0f);
        const bool early_factory = CreateBeforeParameters(definition.type);
        std::uint32_t task = 0u;
        if (early_factory) {
            task = runtime.CreateTask(definition.type);
            RegisterCreatedTask(object, task, definition, runtime);
        }
        if (!ReadTaskParameters(definition, node, runtime)) continue;
        if (!early_factory) {
            task = runtime.CreateTask(definition.type);
            RegisterCreatedTask(object, task, definition, runtime);
        }
        if (definition.type == 19) {
            const auto minimum = std::get<float>(
                definition.parameters[0].value);
            const auto maximum = std::get<float>(
                definition.parameters[1].value);
            definition.parameters.push_back(
                {"generated_wait_duration",
                 runtime.RandomWaitDuration(minimum, maximum)});
        }
        runtime.InitializeTask(task, object,
                               object + kContextTasksOffset, definition);
        if (definition.type == 16)
            SaveStartTransform(object, definition, runtime);
        runtime.AppendTaskPointer(object + 0x2a0u, task);
    }
    return 1u;
}

}  // namespace pal4
