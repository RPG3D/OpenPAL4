#include "pal4/dialog_object.h"

#include <bit>
#include <cctype>

namespace pal4 {
namespace {
bool EqualAsciiCaseInsensitive(std::string_view left, std::string_view right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t i = 0u; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i])))
            return false;
    }
    return true;
}
}  // namespace

bool DialogObject::LoadConfig(DialogObjectState& state, std::uint32_t root,
                              IDialogObjectConfigRuntime& runtime) {
    if (root == 0u ||
        !EqualAsciiCaseInsensitive(runtime.NodeName(root), "Effect"))
        return false;

    const auto object_list = runtime.FindChild(root, "ObjectList");
    if (object_list != 0u) {
        for (auto node = runtime.FindChild(object_list, "Object"); node != 0u;
             node = runtime.NextChild(object_list, node, "Object")) {
            runtime.AppendObjectId(
                state, runtime.StringAttribute(node, "ObjectId").value_or(""));
        }
    }

    const auto task_list = runtime.FindChild(root, "TaskList");
    if (task_list != 0u) {
        for (auto node = runtime.FindChild(task_list, "TASK"); node != 0u;
             node = runtime.NextChild(task_list, node, "TASK")) {
            runtime.AppendTaskId(
                state, runtime.IntAttribute(node, "TaskId").value_or(0));
        }
    }

    const auto parameter_list = runtime.FindChild(root, "TaskParamList");
    if (parameter_list != 0u) {
        for (std::uint32_t i = 0u; i < runtime.ChildCount(parameter_list); ++i)
            runtime.AppendTaskParameter(
                state, runtime.CopyXmlNode(runtime.ChildAt(parameter_list, i)));
    }

    if (!runtime.LoadText(state.words[4u], runtime.FindChild(root, "OxnList")))
        return false;

    const auto target_list = runtime.FindChild(root, "TargetList");
    if (target_list != 0u) {
        const auto count = runtime.ChildCount(target_list);
        for (std::uint32_t i = 0u; i < count; ++i) {
            const auto node = runtime.ChildAt(target_list, i);
            const auto name = runtime.StringAttribute(node, "TargetName")
                                  .value_or("");
            const std::array<float, 3u> position{
                runtime.FloatAttribute(node, "TargetPosX").value_or(0.0f),
                runtime.FloatAttribute(node, "TargetPosY").value_or(0.0f),
                runtime.FloatAttribute(node, "TargetPosZ").value_or(0.0f)};
            runtime.InsertTarget(state, name, position);
        }
    }

    const auto distance_clip = runtime.IntAttribute(root, "DistanceClip")
                                   .value_or(1);
    auto& flags = state.words[20u];
    flags = (flags & ~(0xffu << 24u)) |
            (std::uint32_t{distance_clip > 0 ? 1u : 0u} << 24u);
    const auto camera_start = runtime.FloatAttribute(root, "CameraStart")
                                  .value_or(1.0f);
    state.words[21u] = std::bit_cast<std::uint32_t>(
        camera_start < 0.0f ? 0.0f : camera_start);
    return true;
}

}  // namespace pal4
