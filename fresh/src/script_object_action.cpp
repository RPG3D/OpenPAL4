#include "pal4/script_object_action.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

char FoldAscii(char value) {
    return value >= 'A' && value <= 'Z' ?
        static_cast<char>(value + ('a' - 'A')) : value;
}

bool SameAnimation(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i)
        if (FoldAscii(left[i]) != FoldAscii(right[i])) return false;
    return true;
}

}  // namespace

std::uint32_t ScriptObjectAction::Create(
    std::string_view object_name, std::string_view animation_name,
    std::int32_t target_loops, std::uint8_t wait_mode) {
    const auto object = runtime_.FindByName(runtime_.ObjectManager(),
                                            object_name);
    if (object == 0u)
        throw std::invalid_argument("giObjectDoAction requires an existing object");
    const auto type = runtime_.ObjectType(object);
    if (type != kObjectType) return type;

    if (SameAnimation(runtime_.AnimationName(object), animation_name))
        runtime_.SetAnimationTime(object, 0.0f);
    else
        runtime_.SelectAnimation(object, animation_name);
    runtime_.ClearAnimationFlag(object, 0u);
    runtime_.SetAnimationFlag(object, 1u);

    const auto name = std::string(object_name) + "-OBJ-Action";
    auto* state = runtime_.AllocateWait(sizeof(ObjectActionWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->target_loops = target_loops;
        state->completed_loops = 0;
        state->object = object;
        state->vtable_address = kWaitVtable;
        state->phase = 4u;
        runtime_.AssignName(*state, name);
        runtime_.FlushWait(*state);
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    return runtime_.CleanupName();
}

void ScriptObjectAction::Update(ObjectActionWaitState& state) {
    if (state.target_loops > 0) {
        if (state.completed_loops >= state.target_loops) {
            state.completed_loops = 0;
            runtime_.SetAnimationTime(state.object, 0.0f);
            runtime_.ClearAnimationFlag(state.object, 1u);
            runtime_.CloseWait(state);
        } else if (runtime_.AnimationFlag(state.object)) {
            ++state.completed_loops;
        }
    } else if (state.target_loops == -1 &&
               runtime_.AnimationFlag(state.object)) {
        state.completed_loops = 0;
        runtime_.SetAnimationTime(state.object,
                                  runtime_.AnimationLength(state.object));
        runtime_.ClearAnimationFlag(state.object, 1u);
        runtime_.CloseWait(state);
    }
}

void ScriptObjectAction::Cleanup(ObjectActionWaitState& state) {
    state.completed_loops = 0;
    runtime_.CloseWait(state);
}

}  // namespace pal4
