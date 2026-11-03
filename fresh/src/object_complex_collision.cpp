#include "pal4/object_complex_collision.h"

#include "pal4/vector3_operations.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float Dot(const BehaviorVector3& first, const BehaviorVector3& second) {
    return first[2] * second[2] + first[1] * second[1] +
           first[0] * second[0];
}

float Length(const BehaviorVector3& vector) {
    return std::sqrt(Dot(vector, vector));
}

BehaviorVector3 Add(const BehaviorVector3& first,
                    const BehaviorVector3& second) {
    BehaviorVector3 result{};
    Vector3Operations::Add(first, result, second);
    return result;
}

BehaviorVector3 Subtract(const BehaviorVector3& first,
                         const BehaviorVector3& second) {
    BehaviorVector3 result{};
    Vector3Operations::Subtract(first, result, second);
    return result;
}

BehaviorVector3 Scale(const BehaviorVector3& vector, float factor) {
    BehaviorVector3 result{};
    Vector3Operations::Scale(vector, result, factor);
    return result;
}

void Store(ObjectMovementState& state, std::size_t word,
           const BehaviorVector3& value) {
    for (std::size_t i = 0u; i != 3u; ++i)
        state.words[word + i] = std::bit_cast<std::uint32_t>(value[i]);
}

float DirectionSign(float dot) {
    return dot > 0.0f ? -1.0f : 1.0f;
}

}  // namespace

float ObjectComplexCollisionRuntimeStub::Radius(std::uint32_t) { return 0.0f; }
float ObjectComplexCollisionRuntimeStub::Speed(std::uint32_t) { return 0.0f; }
BehaviorVector3 ObjectComplexCollisionRuntimeStub::Position(std::uint32_t) {
    return {};
}
BehaviorVector3 ObjectComplexCollisionRuntimeStub::Offset(std::uint32_t) {
    return {};
}
BehaviorVector3 ObjectComplexCollisionRuntimeStub::Forward(std::uint32_t) {
    return {};
}
BehaviorVector3 ObjectComplexCollisionRuntimeStub::Direction(std::uint32_t) {
    return {};
}
BehaviorVector3 ObjectComplexCollisionRuntimeStub::GlobalZeroVector() {
    return {};
}
void ObjectComplexCollisionRuntimeStub::NotifyProximity(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void ObjectComplexCollisionRuntimeStub::NotifyCollision(
    std::uint32_t, std::uint32_t, float,
    const BehaviorVector3&, const BehaviorVector3&) {}

BehaviorVector3& ObjectComplexCollision::Process(
    ObjectMovementState& state, std::uint32_t self_address,
    BehaviorVector3& output, float maximum_time,
    std::span<const std::uint32_t> objects,
    IObjectComplexCollisionRuntime& runtime) {
    BehaviorVector3 first_response{};
    bool found_proximity = false;
    for (const auto other : objects) {
        if (other == self_address) continue;
        const float other_radius = runtime.Radius(other);
        const float self_radius = runtime.Radius(self_address);
        const float radius_sum = other_radius + self_radius;
        const auto self_position = runtime.Position(self_address);
        const auto other_position = runtime.Position(other);
        const auto separation = Subtract(other_position, self_position);
        if (Length(separation) < radius_sum) {
            runtime.NotifyProximity(self_address, other, 0u);
            const auto reversed = Scale(separation, -1.0f);
            const auto forward = runtime.Forward(self_address);
            Vector3Operations::SubtractProjection(
                reversed, first_response, forward);
            found_proximity = true;
            break;
        }
    }
    if (!found_proximity) first_response = runtime.GlobalZeroVector();
    if (!Vector3Operations::Equals(first_response,
                                   runtime.GlobalZeroVector())) {
        output = first_response;
        return output;
    }

    std::uint32_t selected = 0u;
    float best_time = maximum_time;
    float sign = 0.0f;
    BehaviorVector3 selected_self{};
    BehaviorVector3 selected_other{};
    for (const auto other : objects) {
        if (other == self_address) continue;
        const float diameter = runtime.Radius(self_address) * 2.0f;
        const auto self_offset = runtime.Offset(self_address);
        const auto other_offset = runtime.Offset(other);
        const auto separation = Subtract(other_offset, self_offset);
        const float distance = Length(separation);

        float time = 0.0f;
        if (distance != 0.0f) {
            const auto normal = Scale(separation, 1.0f / distance);
            const auto other_position = runtime.Position(other);
            const auto self_position = runtime.Position(self_address);
            const auto relative = Subtract(self_position, other_position);
            time = Dot(normal, relative) / distance;
            if (time < 0.0f) continue;
        }
        if (!(time < best_time)) continue;

        const float self_speed = runtime.Speed(self_address);
        const auto self_forward = runtime.Forward(self_address);
        const float other_speed = runtime.Speed(other);
        const auto other_forward = runtime.Forward(other);
        const auto projected_self = Add(
            runtime.Position(self_address),
            Scale(Scale(self_forward, self_speed), time));
        const auto projected_other = Add(
            runtime.Position(other),
            Scale(Scale(other_forward, other_speed), time));
        Store(state, 18u, projected_self);
        Store(state, 15u, projected_other);
        if (!(Length(Subtract(projected_self, projected_other)) < diameter))
            continue;

        selected = other;
        selected_self = projected_self;
        selected_other = projected_other;
        best_time = time;
    }

    if (selected != 0u) {
        const auto other_forward = runtime.Forward(selected);
        const auto self_forward = runtime.Forward(self_address);
        const float alignment = Dot(self_forward, other_forward);
        float selected_sign = -1.0f;
        if (alignment < -0.70700002f) {
            const auto self_position = runtime.Position(self_address);
            const auto direction = runtime.Direction(self_address);
            selected_sign = DirectionSign(Dot(
                Subtract(selected_other, self_position), direction));
        } else if (alignment < 0.70700002f) {
            const auto self_position = runtime.Position(self_address);
            const auto other_position = runtime.Position(selected);
            const auto direction = runtime.Direction(self_address);
            selected_sign = DirectionSign(Dot(
                Subtract(other_position, self_position), direction));
        } else {
            const float other_speed = runtime.Speed(selected);
            const float self_speed = runtime.Speed(self_address);
            if (!(self_speed < other_speed)) {
                const auto other_offset = runtime.Offset(selected);
                const auto direction = runtime.Direction(self_address);
                selected_sign = DirectionSign(Dot(other_offset, direction));
            }
        }
        sign = selected_sign;
        runtime.NotifyCollision(self_address, selected, sign,
                                selected_self, selected_other);
    }

    output = Scale(runtime.Direction(self_address), sign);
    return output;
}

}  // namespace pal4
