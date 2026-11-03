#include "pal4/base_object_motion.h"
#include "pal4/collision_actor.h"
#include "pal4/game_vector_math.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

static_assert(sizeof(pal4::BaseObjectState) == 0x60cu);

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float value, float expected) {
    return std::fabs(value - expected) < 0.0001f;
}

void PutFloat(pal4::BaseObjectState& state,
              std::uint32_t offset, float value) {
    state.Write(offset, std::bit_cast<std::uint32_t>(value));
}

struct Runtime final : pal4::IBaseObjectMotionRuntime {
    std::vector<char> calls;
    pal4::BaseObjectVector3 flat{};
    pal4::BaseObjectVector3 pose{};
    pal4::BaseObjectVector3 direction{};
    float cosine{};

    void UpdateAndRenderClump(float elapsed) override {
        Check(Near(elapsed, 0.25f), "clump update receives elapsed time");
        calls.push_back('R');
    }
    void UpdateInterpolator(pal4::BaseObjectState&,
                            std::uint32_t offset, float elapsed) override {
        Check(offset == 0x170u && Near(elapsed, 0.25f),
              "animation update receives embedded state and elapsed time");
        calls.push_back('I');
    }
    void UpdateCharacter(pal4::BaseObjectState&,
                         std::uint32_t offset, std::uint32_t argument,
                         float elapsed, float scale) override {
        Check(offset == 0x564u && argument == 0x77u &&
                  Near(elapsed, 0.25f) && scale == 1.0f,
              "character virtual slot receives original four arguments");
        calls.push_back('C');
    }
    void NormalizeAndClamp(pal4::BaseObjectVector3& value) override {
        calls.push_back('N');
        Check(value.y == 0.0f,
              "direction loses its vertical component before normalization");
        flat = value;
        const float length = std::hypot(value.x, value.z);
        value.x /= length;
        value.z /= length;
    }
    double AngleFromCosine(float value) override {
        calls.push_back('A');
        cosine = value;
        return 1.0;
    }
    void SetPose(pal4::BaseObjectState&,
                 const pal4::BaseObjectVector3& value,
                 std::uint32_t mode) override {
        Check(mode == 0u, "pose virtual slot receives zero mode");
        calls.push_back('P');
        pose = value;
    }
    std::uint32_t SetDirection(pal4::BaseObjectState&,
                               const pal4::BaseObjectVector3& value) override {
        calls.push_back('D');
        direction = value;
        return 0xabcdef12u;
    }
};

void Seed(pal4::BaseObjectState& state, float x, float y, float z) {
    PutFloat(state, 0x570u, 11.0f);
    PutFloat(state, 0x574u, 12.0f);
    PutFloat(state, 0x578u, 13.0f);
    PutFloat(state, 0x57cu, x);
    PutFloat(state, 0x580u, y);
    PutFloat(state, 0x584u, z);
    PutFloat(state, 0xa0u, 21.0f);
    PutFloat(state, 0xa4u, 22.0f);
    PutFloat(state, 0xa8u, 23.0f);
}

void CheckGates() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    Seed(state, 3.0f, 4.0f, 5.0f);
    Check(pal4::BaseObjectMotion::UpdatePosition(
              state, 0x12345678u, 0.25f, 0x77u, runtime) == 0u &&
              runtime.calls.empty(),
          "inactive object skips every update");

    state.Write(0x1f0u, 1u);
    state.Write(0x5f4u, 0xfedcba98u);
    Check(pal4::BaseObjectMotion::UpdatePosition(
              state, 0x12345678u, 0.25f, 0x77u, runtime) ==
              0xfedcba98u && runtime.calls == std::vector<char>{'R', 'I', 'C'},
          "position hold returns its word after three updates");

    runtime.calls.clear();
    state.Write(0x5f4u, 0u);
    Seed(state, -0.0f, 5.0f, 0.0f);
    Check(pal4::BaseObjectMotion::UpdatePosition(
              state, 0x12345678u, 0.25f, 0x77u, runtime) ==
              0x12344000u && runtime.calls == std::vector<char>{'R', 'I', 'C'},
          "zero horizontal direction preserves x87 AX return artifact");
}

void CheckRotation(bool reverse) {
    pal4::BaseObjectState state{};
    state.Write(0x1f0u, 1u);
    state.Write(0x604u, reverse ? 1u : 0u);
    Seed(state, 3.0f, 99.0f, 4.0f);
    Runtime runtime;
    Check(pal4::BaseObjectMotion::UpdatePosition(
              state, 0x12345678u, 0.25f, 0x77u, runtime) ==
              0xabcdef12u &&
              runtime.calls ==
                  std::vector<char>{'R', 'I', 'C', 'N', 'A', 'P', 'D'},
          "position update preserves complete helper and virtual-call order");
    Check(Near(runtime.flat.x, reverse ? -3.0f : 3.0f) &&
              runtime.flat.y == 0.0f &&
              Near(runtime.flat.z, reverse ? -4.0f : 4.0f) &&
              Near(runtime.cosine, reverse ? -0.8f : 0.8f),
          "reverse flag negates direction before normalization");
    Check(Near(runtime.pose.x, 21.0f) && Near(runtime.pose.z, 23.0f) &&
              Near(runtime.pose.y,
                   static_cast<float>((reverse ?
                       6.283185307179586 - 1.0 : 1.0) * 57.295780181884766)),
          "pose keeps X/Z and stores heading in degrees");
    Check(Near(runtime.direction.x, 11.0f) &&
              Near(runtime.direction.y, 12.0f) &&
              Near(runtime.direction.z, 13.0f),
          "final virtual slot receives the original character vector");
}

void CheckNegativeHeading() {
    pal4::BaseObjectState state{};
    state.Write(0x1f0u, 1u);
    Seed(state, -3.0f, 0.0f, 4.0f);
    Runtime runtime;
    (void)pal4::BaseObjectMotion::UpdatePosition(
        state, 0x12345678u, 0.25f, 0x77u, runtime);
    Check(Near(runtime.pose.y,
               static_cast<float>((6.283185307179586 - 1.0) *
                                  57.295780181884766)),
          "negative horizontal X selects the full-turn complement");
}

void CheckCapturedM11Heading() {
    // Pair133 after42861 +55 frames: complete path/cursor/position/speed and
    // velocity match. Original yaw433CBD3E versus native433CBD3F is the first
    // remaining field divergence. Use the production shared normalize/acos.
    struct CapturedRuntime final : pal4::IBaseObjectMotionRuntime {
        pal4::GameMathTables tables;
        float yaw{};
        CapturedRuntime() { (void)tables.CalculateSquareRootTable(); }
        void NormalizeAndClamp(pal4::BaseObjectVector3& value) override {
            pal4::GameVector3 vector{value.x, value.y, value.z};
            pal4::GameVectorMath::NormalizeAndClamp(vector, tables);
            value = {vector[0], vector[1], vector[2]};
        }
        double AngleFromCosine(float value) override {
            return tables.ApproximateArccosine(value);
        }
        void SetPose(pal4::BaseObjectState&, const pal4::BaseObjectVector3& value,
                     std::uint32_t) override { yaw = value.y; }
    } runtime;
    pal4::BaseObjectState state{};
    state.Write(0x1f0u, 1u);
    Seed(state, std::bit_cast<float>(3199820186u), 0.0f,
                std::bit_cast<float>(3222703116u));
    (void)pal4::BaseObjectMotion::UpdatePosition(state, 0x10000u, 0.017f, 0u, runtime);
    Check(std::bit_cast<std::uint32_t>(runtime.yaw) == 1128054078u,
          "captured M11 negative-X velocity yields exact original yaw through shared game math");
}

void CheckFinalPathSnapKeepsLastVisiblePosition() {
    // Live211 M06/2D: same path target/collision position, different last
    // visible per-axis remainders. Exercise both original shared stages.
    constexpr pal4::BaseObjectVector3 target{
        -647.0471801757812f, 0.7000000476837158f, 269.08331298828125f};
    struct Path final : pal4::ICollisionActorMovementRuntime {
        void WriteWord(std::uint32_t, std::uint32_t) override {
            Check(false, "final snap does not modify path nodes");
        }
        std::uint32_t Allocate(std::uint32_t) override {
            Check(false, "final snap does not allocate"); return 0u;
        }
        void Free(std::uint32_t) override { Check(false, "final snap does not free"); }
        std::uint32_t Trace(std::uint32_t, const pal4::D3dLine2State&,
                            std::uint32_t&) override {
            Check(false, "final node has no following face"); return 0u;
        }
        void ClampFacePoint(std::uint32_t, pal4::D3dTriangleVector3&) override {
            Check(false, "zero step does not clamp");
        }
        bool TestClearance(std::uint32_t, const pal4::D3dTriangleVector3&,
                           const pal4::D3dTriangleVector3&, std::uint32_t) override {
            Check(false, "zero step does not test clearance"); return false;
        }
        std::uint32_t AdvanceAcrossFaces(std::uint32_t,
            const pal4::D3dTriangleVector3&, std::uint32_t,
            pal4::D3dTriangleVector3&, std::uint32_t&) override {
            Check(false, "zero step does not advance faces"); return 0u;
        }
        std::uint32_t ReadWord(std::uint32_t address) override {
            if (address == 0x2000u) return 0x3000u;
            if (address == 0x2008u) return std::bit_cast<std::uint32_t>(-647.0471801757812f);
            if (address == 0x200cu) return std::bit_cast<std::uint32_t>(0.7000000476837158f);
            if (address == 0x2010u) return std::bit_cast<std::uint32_t>(269.08331298828125f);
            Check(false, "final node only reads its target and sentinel");
            return 0u;
        }
    };
    struct FinalStep final : pal4::IBaseObjectMotionRuntime {
        Path path;
        unsigned placements{};
        void UpdateCharacter(pal4::BaseObjectState& base, std::uint32_t offset,
                             std::uint32_t mode, float elapsed, float) override {
            pal4::CollisionActorState movement;
            for (unsigned i = 0; i < movement.words.size(); ++i)
                movement.words[i] = base.Read(offset + i * 4u);
            (void)pal4::CollisionActor::UpdateMovement(
                movement, 0x1000u, mode, elapsed, path);
            for (unsigned i = 0; i < movement.words.size(); ++i)
                base.Write(offset + i * 4u, movement.words[i]);
        }
        std::uint32_t SetDirection(pal4::BaseObjectState&,
                                  const pal4::BaseObjectVector3&) override {
            ++placements;
            return 0u;
        }
    };
    for (const auto before : {
             pal4::BaseObjectVector3{target.x, target.y, 269.0856018066406f},
             pal4::BaseObjectVector3{-647.0459594726562f, target.y, target.z}}) {
        pal4::BaseObjectState base{};
        base.Write(0x1f0u, 1u);
        for (const auto offset : {0xacu, 0x570u}) {
            PutFloat(base, offset, before.x);
            PutFloat(base, offset + 4u, before.y);
            PutFloat(base, offset + 8u, before.z);
        }
        base.Write(0x568u, 1u);  // CollisionActor tree
        PutFloat(base, 0x588u, 45.0f);
        base.Write(0x58cu, 1u); // path enabled
        base.Write(0x5b8u, 0x3000u); // path sentinel
        base.Write(0x5c0u, 0x2000u); // current target node
        FinalStep runtime;
        (void)pal4::BaseObjectMotion::UpdatePosition(
            base, 0x10000u, 0.017f, 0u, runtime);
        Check(base.Read(0x570u) == std::bit_cast<std::uint32_t>(target.x) &&
                  base.Read(0x578u) == std::bit_cast<std::uint32_t>(target.z) &&
                  (base.Read(0x58cu) & 0xffu) == 0u &&
                  base.Read(0x57cu) == 0u && base.Read(0x584u) == 0u,
              "final sub-threshold path step snaps internal target and stops");
        Check(runtime.placements == 0u &&
                  base.Read(0xacu) == std::bit_cast<std::uint32_t>(before.x) &&
                  base.Read(0xb4u) == std::bit_cast<std::uint32_t>(before.z),
              "zero velocity preserves last visible position after internal snap");
    }
}

}  // namespace

int main() {
    CheckGates();
    CheckRotation(false);
    CheckRotation(true);
    CheckNegativeHeading();
    CheckCapturedM11Heading();
    CheckFinalPathSnapKeepsLastVisiblePosition();
}
