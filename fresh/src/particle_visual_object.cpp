#include "pal4/particle_visual_object.h"

#include <cstring>
#include <bit>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace pal4 {

namespace {

std::int32_t TruncateInt(double number) {
    if (!std::isfinite(number) || number < -2147483648.0 ||
        number >= 2147483648.0)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(number);
}

std::uint8_t ColorByte(float value) {
    const auto scaled = static_cast<float>(
        static_cast<double>(value) * 255.0 + 0.5);
    return static_cast<std::uint8_t>(TruncateInt(scaled));
}

float StateFloat(const ParticleVisualState& state, std::size_t offset) {
    return std::bit_cast<float>(state.ReadWord(offset));
}

template <typename Runtime>
float ExternalFloat(Runtime& runtime, std::uint32_t address) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

template <typename Runtime>
void WriteExternalFloat(Runtime& runtime,
                        std::uint32_t address, float value) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

float ScaleRandomSample(std::int32_t sample, float center, float radius) {
    // 64748C..6474AA and646B6A..646B90: each arithmetic instruction runs
    // under game PC24. Keep the original operation order and float constant.
    const float width = radius - (-radius);
    const float product = static_cast<float>(sample) * width;
    const float fraction = product * std::bit_cast<float>(0x38000100u);
    const float shifted = fraction + center;
    return shifted - radius;
}

float RandomRange(IParticleEmissionRuntime& runtime, float center,
                  float radius, ParticleRandomSite site = ParticleRandomSite::properties) {
    return ScaleRandomSample(runtime.RandomFor(site),center,radius);
}

float RandomSphereCoordinate(IParticleEmissionRuntime& runtime) {
    // 6478FB..64793F: PC24 rounds FMUL before subtracting one.
    const float scaled = static_cast<float>(runtime.RandomFor(
        ParticleRandomSite::position)) * std::bit_cast<float>(0x38800100u);
    return scaled - 1.f;
}

}  // namespace

void ParticleVisualState::WriteByte(std::size_t offset,
                                    std::uint8_t value) {
    bytes[offset] = value;
}

void ParticleVisualState::WriteWord(std::size_t offset,
                                    std::uint32_t value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

std::uint8_t ParticleVisualState::ReadByte(std::size_t offset) const {
    return bytes[offset];
}

std::uint32_t ParticleVisualState::ReadWord(std::size_t offset) const {
    std::uint32_t value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

ParticleVisualState* ParticleVisualObject::Construct(
    ParticleVisualState& state, std::uint8_t stack_byte,
    IParticleVisualRuntime& runtime) {
    const std::pair<std::size_t, std::uint32_t> early_bytes[]{
        {0u, 0u}, {1u, 1u}, {2u, 1u}, {3u, 0u},
        {4u, 0u}, {5u, 0u}, {16u, 0u}, {52u, 1u},
        {54u, 1u}, {55u, 0u}, {60u, stack_byte},
        {144u, 1u}, {156u, stack_byte}, {172u, stack_byte},
        {188u, 1u}, {316u, 0u}, {384u, 0u}, {420u, 0u}};
    for (const auto& [offset, value] : early_bytes)
        state.WriteByte(offset, static_cast<std::uint8_t>(value));

    constexpr std::pair<std::size_t, std::uint32_t> early_words[]{
        {8u, 0u}, {12u, 0u}, {20u, 0u},
        {40u, 0u}, {44u, 0u}, {48u, 0u}, {56u, 0u},
        {64u, 0u}, {68u, 0u}, {72u, 0u}, {76u, 0u},
        {80u, 0u}, {84u, 0u}, {88u, 0u}, {92u, 0u},
        {96u, 0u}, {100u, 0x3f800000u},
        {104u, 0x3f800000u}, {108u, 0x3f800000u},
        {112u, 0u}, {116u, 0u}, {120u, 0u}, {124u, 0u},
        {148u, 0u}, {152u, 0u}, {160u, 0u},
        {164u, 0u}, {168u, 0u}, {176u, 0u},
        {180u, 0u}, {184u, 0u}, {192u, 5u},
        {196u, 2u}, {200u, 100u}, {204u, 0u},
        {208u, 0u}, {228u, 0x3f800000u},
        {232u, 0x3f800000u}, {236u, 0u},
        {240u, 0u}, {244u, 0u}, {248u, 0u},
        {252u, 0u}, {256u, 0u}, {260u, 20u},
        {264u, 5u}, {268u, 0u}, {272u, 0u},
        {276u, 0x3f800000u}, {280u, 0x3f000000u},
        {284u, 0x3f800000u}, {288u, 0u},
        {292u, 0u}, {296u, 0x3f800000u},
        {300u, 0u}, {304u, 0u}, {308u, 0u},
        {312u, 0u}, {388u, 0x3f800000u},
        {392u, 0x3f800000u}, {396u, 0u},
        {400u, 0u}, {404u, 0u}, {408u, 0u},
        {412u, 0u}, {416u, 0u}, {424u, 1097859072u},
        {428u, 0u}, {432u, 0u}, {436u, 0u}};
    for (const auto& [offset, value] : early_words)
        state.WriteWord(offset, value);

    runtime.SeedRandom(runtime.CurrentTime());

    for (std::size_t offset = 320u; offset <= 332u; offset += 4u)
        state.WriteWord(offset, 0x3f800000u);
    for (std::size_t offset = 336u; offset <= 380u; offset += 4u)
        state.WriteWord(offset, 0u);
    for (std::size_t offset = 212u; offset <= 224u; offset += 4u)
        state.WriteWord(offset, 0x3f800000u);
    return &state;
}

int ParticleVisualObject::Destruct(
    ParticleVisualState& state,
    IParticleVisualLifecycleRuntime& runtime) {
    Reset(state, runtime);
    (void)runtime.Free(state.ReadWord(176u));
    for (std::size_t offset = 176u; offset <= 184u; offset += 4u)
        state.WriteWord(offset, 0u);
    (void)runtime.Free(state.ReadWord(160u));
    for (std::size_t offset = 160u; offset <= 168u; offset += 4u)
        state.WriteWord(offset, 0u);
    const auto result = runtime.Free(state.ReadWord(64u));
    for (std::size_t offset = 64u; offset <= 72u; offset += 4u)
        state.WriteWord(offset, 0u);
    return result;
}

std::uint8_t ParticleVisualObject::SetProperties(
    ParticleVisualState& state, std::uint8_t mode) {
    state.WriteWord(192u, 5u);
    state.WriteByte(188u, 1u);
    state.WriteWord(196u, mode != 0u ? 2u : 6u);
    return mode;
}

std::uint8_t ParticleVisualObject::Initialize(
    ParticleVisualState& state, std::uint8_t byte52,
    std::uint32_t word48, std::uint32_t word128,
    std::uint32_t word132, std::uint32_t word136,
    std::uint32_t word140, std::uint8_t byte144,
    IParticleVisualLifecycleRuntime& runtime) {
    Reset(state, runtime);
    state.WriteWord(128u, word128);
    state.WriteByte(0u, 1u);
    state.WriteWord(132u, word132);
    state.WriteWord(136u, word136);
    state.WriteWord(48u, word48);
    const auto type = state.ReadWord(124u);
    state.WriteWord(140u, word140);
    state.WriteByte(52u, byte52);
    state.WriteByte(144u, byte144);
    if (type == 0u) return InitializeRender(state, runtime.TankRuntime());
    if (type == 1u)
        return InitializeCollection(state, runtime.CollectionRuntime());
    return 0u;
}

std::uint8_t ParticleVisualObject::InitializeWithFrame(
    ParticleVisualState& state, std::uint8_t byte52,
    std::uint32_t word48, std::uint32_t external_frame,
    std::uint8_t byte144,
    IParticleVisualLifecycleRuntime& runtime) {
    Reset(state, runtime);
    state.WriteByte(52u, byte52);
    const auto type = state.ReadWord(124u);
    state.WriteWord(48u, word48);
    state.WriteByte(55u, 1u);
    state.WriteWord(56u, external_frame);
    state.WriteByte(144u, byte144);
    std::uint8_t result{};
    if (type == 0u)
        result = InitializeRender(state, runtime.TankRuntime());
    else if (type == 1u)
        result = InitializeCollection(state, runtime.CollectionRuntime());
    else {
        state.WriteByte(0u, 0u);
        return 0u;
    }
    if (result != 0u)
        runtime.AddFrameChild(
            runtime.ReadExternalWord(external_frame + 4u),
            state.ReadWord(44u));
    state.WriteByte(0u, result);
    return result;
}

std::uint8_t ParticleVisualObject::InitializeRender(
    ParticleVisualState& state, IParticleTankRuntime& runtime) {
    const auto atomic = runtime.CreateTank(state.ReadWord(200u),
                                           0x100000a7u, 1u);
    state.WriteWord(40u, atomic);
    const auto plugin = runtime.ReadWord(atomic + runtime.PluginOffset());
    runtime.WriteWord(plugin + 64u,
                      runtime.ReadWord(plugin + 64u) | 0x00800000u);
    runtime.WriteWord(plugin + 4u, 0u);
    const auto texture = state.ReadWord(48u);
    if (texture != 0u) {
        const auto geometry = runtime.ReadWord(atomic + 24u);
        const auto materials = runtime.ReadWord(geometry + 32u);
        runtime.SetMaterialTexture(runtime.ReadWord(materials), texture);
    }
    const auto frame = runtime.CreateFrame();
    state.WriteWord(44u, frame);
    runtime.SetAtomicFrame(atomic, frame);
    runtime.SetFrameIdentity(frame);
    runtime.UpdateFrameObjects(frame);
    runtime.WriteWord(plugin + 172u, state.ReadByte(188u));
    runtime.WriteWord(plugin + 164u, state.ReadWord(192u));
    runtime.WriteWord(plugin + 168u, state.ReadWord(196u));
    runtime.WriteWord(plugin + 64u,
                      runtime.ReadWord(plugin + 64u) | 0x10000000u);
    (void)UpdateFrameTransform(state, runtime.FrameTransformRuntime());
    return 1u;
}

std::uint8_t ParticleVisualObject::InitializeCollection(
    ParticleVisualState& state, IParticleCollectionRuntime& runtime) {
    const auto frame = runtime.CreateFrame();
    state.WriteWord(44u, frame);
    runtime.SetFrameIdentity(frame);
    runtime.UpdateFrameObjects(frame);

    const bool mode = state.ReadWord(196u) != 6u;
    constexpr std::array<float, 2u> scale{1.f, 1.f};
    for (std::int32_t index = 0;
         index < static_cast<std::int32_t>(state.ReadWord(200u)); ++index) {
        const auto element = runtime.Allocate(52u);
        if (element != 0u) {
            runtime.WriteByte(element + 12u, 0u);
            runtime.WriteByte(element + 13u, 1u);
            runtime.WriteByte(element + 14u, 0u);
            runtime.WriteByte(element + 15u, 1u);
            runtime.WriteByte(element + 16u, 0u);
            runtime.WriteWord(element + 20u, 0u);
            runtime.WriteWord(element + 24u, 0u);
            runtime.WriteWord(element + 48u, 0u);
        }
        runtime.WriteByte(element + 14u, state.ReadByte(53u));
        const auto asset = state.ReadWord(48u);
        if (state.ReadByte(52u) != 0u) {
            runtime.InitializeFromAsset(element, asset, scale, mode,
                                        state.ReadByte(144u), 1u);
        } else {
            const std::array<std::uint32_t, 3u> source{
                state.ReadWord(112u), state.ReadWord(116u),
                state.ReadWord(120u)};
            runtime.InitializeFromObject(element, source, asset, scale,
                                         mode, state.ReadByte(144u), 1u);
            for (std::uint32_t word = 0u; word < 3u; ++word)
                runtime.WriteWord(element + word * 4u, source[word]);
        }
        if (state.ReadByte(53u) != 0u)
            runtime.AttachElementFrame(frame,
                                       runtime.ReadWord(element + 20u));

        const auto begin = state.ReadWord(176u);
        const auto end = state.ReadWord(180u);
        const auto capacity_end = state.ReadWord(184u);
        if ((capacity_end - end) / 4u != 0u) {
            runtime.WriteWord(end, element);
            state.WriteWord(180u, end + 4u);
        } else {
            const auto size = begin == 0u ? 0u : (end - begin) / 4u;
            const auto capacity = size + std::max(size, 1u);
            const auto new_begin = runtime.Allocate(capacity * 4u);
            for (std::uint32_t word = 0u; word < size; ++word)
                runtime.WriteWord(new_begin + word * 4u,
                                  runtime.ReadWord(begin + word * 4u));
            runtime.WriteWord(new_begin + size * 4u, element);
            runtime.FreeBuffer(begin);
            state.WriteWord(176u, new_begin);
            state.WriteWord(180u, new_begin + (size + 1u) * 4u);
            state.WriteWord(184u, new_begin + capacity * 4u);
        }
    }
    (void)UpdateFrameTransform(state, runtime.FrameTransformRuntime());
    return 1u;
}

std::uint8_t ParticleVisualObject::UpdateFrameTransform(
    ParticleVisualState& state,
    IParticleFrameTransformRuntime& runtime) {
    const auto active = state.ReadByte(0u);
    if (active == 0u) return active;
    const auto frame = state.ReadWord(44u);
    const auto read_float = [&state](std::size_t offset) {
        return std::bit_cast<float>(state.ReadWord(offset));
    };
    runtime.SetIdentity(frame);
    runtime.Scale(frame,
                  {read_float(100u), read_float(104u), read_float(108u)},
                  0u);
    runtime.Rotate(frame, {1.f, 0.f, 0.f}, read_float(88u), 2u);
    runtime.Rotate(frame, {0.f, 1.f, 0.f}, read_float(92u), 2u);
    runtime.Rotate(frame, {0.f, 0.f, 1.f}, read_float(96u), 2u);
    runtime.Translate(frame,
                      {read_float(112u), read_float(116u),
                       read_float(120u)}, 2u);
    return runtime.UpdateObjects(frame);
}

void ParticleVisualObject::Reset(
    ParticleVisualState& state,
    IParticleVisualLifecycleRuntime& runtime) {
    if (state.ReadByte(0u) == 0u) return;
    state.WriteByte(0u, 0u);
    const auto begin = state.ReadWord(160u);
    const auto end = state.ReadWord(164u);
    if (begin != 0u) {
        for (auto entry = begin; entry != end; entry += 4u) {
            const auto item = runtime.ReadExternalWord(entry);
            (void)runtime.Free(runtime.ReadExternalWord(item + 80u));
            (void)runtime.Free(runtime.ReadExternalWord(item + 84u));
            (void)runtime.Free(runtime.ReadExternalWord(item + 88u));
            (void)runtime.Free(item);
        }
    }
    state.WriteWord(164u, begin);
    const auto frame = state.ReadWord(44u);
    if (frame != 0u && runtime.ReadExternalWord(frame + 4u) != 0u)
        runtime.RemoveFrameChild(frame);
    const auto type = state.ReadWord(124u);
    if (type == 0u)
        (void)DestroyAtomicAndFrame(state, runtime);
    else if (type == 1u)
        (void)DestroyCollectionElements(state, runtime);

    constexpr std::pair<std::size_t, std::uint32_t> zero_words[]{
        {88u, 0u}, {92u, 0u}, {96u, 0u},
        {112u, 0u}, {116u, 0u}, {120u, 0u},
        {204u, 0u}, {208u, 0u}, {148u, 0u},
        {152u, 0u}, {268u, 0u}, {272u, 0u},
        {288u, 0u}, {236u, 0u}, {240u, 0u},
        {292u, 0u}, {300u, 0u}, {304u, 0u},
        {308u, 0u}, {244u, 0u}, {312u, 0u},
        {248u, 0u}, {252u, 0u}, {256u, 0u},
        {336u, 0u}, {340u, 0u}, {344u, 0u},
        {348u, 0u}, {352u, 0u}, {356u, 0u},
        {360u, 0u}, {364u, 0u}, {368u, 0u},
        {372u, 0u}, {376u, 0u}, {380u, 0u},
        {396u, 0u}, {400u, 0u}, {404u, 0u},
        {408u, 0u}, {428u, 0u}, {432u, 0u},
        {436u, 0u}, {124u, 0u}, {48u, 0u},
        {76u, 0u}, {80u, 0u}, {84u, 0u}};
    for (const auto& [offset, value] : zero_words)
        state.WriteWord(offset, value);
    constexpr std::size_t unit_words[]{
        100u, 104u, 108u, 212u, 216u, 220u,
        224u, 228u, 232u, 276u, 284u, 296u,
        320u, 324u, 328u, 332u, 388u, 392u};
    for (const auto offset : unit_words)
        state.WriteWord(offset, 0x3f800000u);
    state.WriteWord(192u, 5u);
    state.WriteWord(196u, 2u);
    state.WriteWord(200u, 100u);
    state.WriteWord(260u, 20u);
    state.WriteWord(264u, 5u);
    state.WriteWord(280u, 0x3f000000u);
    state.WriteWord(424u, 1097859072u);
    state.WriteByte(188u, 1u);
    state.WriteByte(2u, 1u);
    state.WriteByte(3u, 0u);
    state.WriteByte(4u, 0u);
    state.WriteByte(54u, 1u);
    state.WriteByte(316u, 0u);
    state.WriteByte(384u, 0u);
    state.WriteByte(420u, 0u);
    state.WriteByte(144u, 1u);
}

int ParticleVisualObject::Update(
    ParticleVisualState& state, double value,
    std::uint32_t argument, bool skip_simulation,
    IParticleVisualUpdateRuntime& runtime) {
    if (state.ReadByte(0u) == 0u) return 0;
    const auto bits = std::bit_cast<std::uint64_t>(value);
    const auto low = static_cast<std::uint32_t>(bits);
    const auto high = static_cast<std::uint32_t>(bits >> 32u);
    if (!skip_simulation) {
        if (state.ReadByte(55u) != 0u)
            (void)UpdateSkinAndBones(state, runtime.SkinRuntime());
        if (state.ReadByte(1u) != 0u)
            (void)DispatchUpdate(state, value, runtime);
    }
    runtime.PrepareRender(value);
    return DispatchRender(state, low, high, argument, runtime);
}

int ParticleVisualObject::HandleEvent(
    ParticleVisualState& state, std::uint32_t event,
    IParticleVisualUpdateRuntime& runtime) {
    return DispatchEvent(state, event, runtime);
}

std::int32_t ParticleVisualObject::Deactivate(
    ParticleVisualState& state,
    IParticleVisualUpdateRuntime& runtime) {
    const auto active = state.ReadByte(0u);
    if (active == 0u || state.ReadByte(55u) != 0u)
        return active;
    std::int32_t result = static_cast<std::int32_t>(state.ReadWord(208u));
    state.WriteByte(16u, 0u);
    state.WriteWord(20u, 0u);
    state.WriteByte(5u, 0u);
    if (result > 0) {
        result = static_cast<std::int32_t>(state.ReadWord(204u));
        state.WriteWord(208u, 0u);
        for (std::int32_t index = 0; index < result; ++index) {
            const auto item = runtime.ReadExternalWord(
                state.ReadWord(160u) + 4u *
                    static_cast<std::uint32_t>(index));
            if (runtime.ReadExternalByte(item) != 0u) {
                runtime.WriteExternalWord(item + 4u, 0u);
                runtime.WriteExternalByte(item, 0u);
                runtime.WriteExternalWord(item + 40u, 0u);
            }
            result = static_cast<std::int32_t>(state.ReadWord(204u));
        }
    }
    state.WriteByte(1u, 1u);
    state.WriteByte(3u, 0u);
    state.WriteWord(148u, state.ReadWord(152u));
    return result;
}

int ParticleVisualObject::DestroyAtomicAndFrame(
    ParticleVisualState& state,
    IParticleVisualLifecycleRuntime& runtime) {
    const auto atomic = state.ReadWord(40u);
    if (atomic != 0u) {
        (void)runtime.DestroyAtomic(atomic);
        state.WriteWord(40u, 0u);
    }
    const auto frame = state.ReadWord(44u);
    if (frame == 0u) return 0;
    const auto result = runtime.DestroyFrame(frame);
    state.WriteWord(44u, 0u);
    return result;
}

int ParticleVisualObject::DestroyCollectionElements(
    ParticleVisualState& state,
    IParticleVisualLifecycleRuntime& runtime) {
    for (auto entry = state.ReadWord(176u);
         entry != state.ReadWord(180u); entry += 4u) {
        const auto element = runtime.ReadExternalWord(entry);
        if (element != 0u) {
            runtime.DeinitializeElement(element);
            (void)runtime.Free(element);
        }
    }
    state.WriteWord(180u, state.ReadWord(176u));
    const auto frame = state.ReadWord(44u);
    if (frame == 0u) return 0;
    const auto result = runtime.DestroyFrame(frame);
    state.WriteWord(44u, 0u);
    return result;
}

void ParticleVisualObject::AppendPoint(
    ParticleVisualState& state, ParticleVisualPoint point,
    IParticleSkinRuntime& runtime) {
    const auto begin = state.ReadWord(64u);
    const auto end = state.ReadWord(68u);
    const auto capacity_end = state.ReadWord(72u);
    if ((capacity_end - end) / 12u != 0u) {
        runtime.WritePointBuffer(end, point);
        state.WriteWord(68u, end + 12u);
        return;
    }
    const auto old_size = begin == 0u ? 0u : (end - begin) / 12u;
    const auto new_capacity = old_size + std::max(old_size, 1u);
    const auto new_begin = runtime.AllocatePointBuffer(new_capacity * 12u);
    for (std::uint32_t index = 0u; index < old_size; ++index)
        runtime.WritePointBuffer(
            new_begin + index * 12u,
            runtime.ReadPointBuffer(begin + index * 12u));
    runtime.WritePointBuffer(new_begin + old_size * 12u, point);
    runtime.FreePointBuffer(begin);
    state.WriteWord(64u, new_begin);
    state.WriteWord(68u, new_begin + (old_size + 1u) * 12u);
    state.WriteWord(72u, new_begin + new_capacity * 12u);
}

int ParticleVisualObject::UpdateSkinAndBones(
    ParticleVisualState& state, IParticleSkinRuntime& runtime) {
    const auto source = state.ReadWord(56u);
    const auto geometry = runtime.SourceGeometry(source);
    const auto skin = runtime.Skin(geometry);
    // IDA645581 reads geometry+24 (numMorphTargets), not numTriangles at+16.
    const auto morph_target_count = runtime.MorphTargetCount(geometry);
    if (morph_target_count > 1) return morph_target_count;
    const auto vertex_count = runtime.VertexCount(geometry);
    const auto hierarchy = runtime.Hierarchy(source);
    state.WriteWord(68u, state.ReadWord(64u));
    for (std::uint32_t index = 0u; index < vertex_count; ++index) {
        const auto vertex = runtime.GeometryVertex(geometry, index);
        ParticleVisualPoint point{};
        if (skin == 0u) {
            point = vertex;
            if ((state.ReadWord(124u) != 1u ||
                 state.ReadByte(53u) == 0u) &&
                state.ReadByte(4u) == 0u)
                point = runtime.TransformFrameVertex(
                    state.ReadWord(44u), point);
        } else {
            const auto indices = runtime.BoneIndices(skin, index);
            const auto weights = runtime.BoneWeights(skin, index);
            const int highest = indices[3] != 0u ? 3
                              : indices[2] != 0u ? 2
                              : indices[1] != 0u ? 1 : 0;
            for (int bone = highest; bone >= 0; --bone) {
                const auto transformed = runtime.TransformBoneVertex(
                    skin, hierarchy, indices[bone], vertex);
                point.x += weights[bone] * transformed.x;
                point.y += weights[bone] * transformed.y;
                point.z += weights[bone] * transformed.z;
            }
        }
        AppendPoint(state, point, runtime);
    }
    return static_cast<int>(vertex_count);
}

std::uint32_t ParticleVisualObject::SimulateParticles(
    ParticleVisualState& state, double delta,
    IParticleSimulationRuntime& runtime) {
    const auto frame = state.ReadWord(44u);
    const auto object_matrix = runtime.ObjectTransform(frame);
    const auto begin = state.ReadWord(160u);
    if (begin == 0u) return object_matrix;

    const auto read_float = [&runtime](std::uint32_t address) {
        return std::bit_cast<float>(runtime.ReadWord(address));
    };
    const auto write_float = [&runtime](std::uint32_t address,
                                        float value) {
        runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
    };
    std::uint32_t index = 0u;
    for (;;) {
        const auto current = state.ReadWord(160u);
        if (current == 0u) return object_matrix;
        const auto count = (state.ReadWord(164u) - current) / 4u;
        if (index >= count) return count;
        const auto item = runtime.ReadWord(current + index * 4u);
        ++index;
        if (runtime.ReadByte(item) == 0u) continue;

        const float age = static_cast<float>(static_cast<double>(read_float(item + 4u)) + delta);
        write_float(item + 4u, age);
        if (age >= static_cast<double>(read_float(item + 8u))) {
            runtime.WriteByte(item, 0u);
            state.WriteWord(208u, state.ReadWord(208u) - 1u);
            continue;
        }

        ParticleVisualPoint movement{
            static_cast<float>(static_cast<double>(read_float(item + 44u)) * delta),
            static_cast<float>(static_cast<double>(read_float(item + 48u)) * delta),
            static_cast<float>(static_cast<double>(read_float(item + 52u)) * delta)};
        if (state.ReadWord(124u) == 1u && state.ReadByte(53u) != 0u) {
            if (state.ReadByte(54u) == 0u)
                movement = runtime.TransformVector(frame + 16u, movement);
        } else if (state.ReadByte(4u) == 0u) {
            movement = runtime.TransformVector(object_matrix, movement);
        }
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            const auto offset = item + 68u + axis * 4u;
            const auto component = axis == 0u ? movement.x
                                 : axis == 1u ? movement.y : movement.z;
            write_float(offset, static_cast<float>(
                static_cast<double>(read_float(offset)) + component));
        }
        write_float(item + 48u, static_cast<float>(
            static_cast<double>(read_float(item + 48u)) -
            static_cast<double>(std::bit_cast<float>(state.ReadWord(240u))) * delta));

        const float normalized_rate =
            std::bit_cast<float>(state.ReadWord(244u));
        constexpr float epsilon = 0.000099999997f;
        if (std::isnan(normalized_rate) || normalized_rate <= -epsilon ||
            normalized_rate >= epsilon)
            UpdatePositionNormalized(state, item + 68u, delta, runtime);

        if (state.ReadByte(420u) != 0u) {
            const auto data = runtime.ReadWord(item + 88u);
            write_float(item + 16u, static_cast<float>(
                static_cast<double>(read_float(data + 4u)) * delta +
                read_float(item + 16u)));
        }
        if (state.ReadByte(3u) != 0u) {
            for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
                runtime.WriteWord(item + 28u + offset,
                                  state.ReadWord(212u + offset));
        } else if (state.ReadByte(316u) != 0u) {
            const auto data = runtime.ReadWord(item + 80u);
            const auto stored_age = read_float(item + 4u);
            for (std::uint32_t component = 0u; component < 4u; ++component) {
                const auto offset = component * 4u;
                write_float(item + 28u + offset, static_cast<float>(
                    static_cast<double>(read_float(data + offset)) +
                    static_cast<double>(read_float(data + 16u + offset)) *
                        stored_age));
            }
        }
        if (state.ReadByte(384u) != 0u) {
            const auto data = runtime.ReadWord(item + 84u);
            const auto stored_age = read_float(item + 4u);
            for (std::uint32_t component = 0u; component < 2u; ++component) {
                const auto offset = component * 4u;
                write_float(item + 20u + offset, static_cast<float>(
                    static_cast<double>(read_float(data + offset)) +
                    static_cast<double>(read_float(data + 8u + offset)) *
                        stored_age));
            }
        } else {
            runtime.WriteWord(item + 20u, state.ReadWord(228u));
            runtime.WriteWord(item + 24u, state.ReadWord(232u));
        }
    }
}

void ParticleVisualObject::UpdatePositionNormalized(
    ParticleVisualState& state, std::uint32_t position,
    double delta, IParticleSimulationRuntime& runtime) {
    const ParticleVisualPoint local{StateFloat(state, 248u),
                                    StateFloat(state, 252u),
                                    StateFloat(state, 256u)};
    ParticleVisualPoint target = local;
    if (state.ReadWord(124u) == 1u && state.ReadByte(53u) != 0u) {
        if (state.ReadByte(54u) == 0u)
            target = runtime.TransformPosition(state.ReadWord(44u) + 16u,
                                               local);
    } else if (state.ReadByte(4u) == 0u) {
        target = runtime.TransformPosition(
            runtime.ObjectTransform(state.ReadWord(44u)), local);
    }
    const ParticleVisualPoint current{
        ExternalFloat(runtime, position),
        ExternalFloat(runtime, position + 4u),
        ExternalFloat(runtime, position + 8u)};
    const auto direction = runtime.Normalize({target.x - current.x,
                                              target.y - current.y,
                                              target.z - current.z});
    const auto distance = static_cast<double>(StateFloat(state, 244u)) * delta;
    WriteExternalFloat(runtime, position,
        static_cast<float>(static_cast<double>(current.x) +
                           static_cast<double>(direction.x) * distance));
    WriteExternalFloat(runtime, position + 4u,
        static_cast<float>(static_cast<double>(current.y) +
                           static_cast<double>(direction.y) * distance));
    WriteExternalFloat(runtime, position + 8u,
        static_cast<float>(static_cast<double>(current.z) +
                           static_cast<double>(direction.z) * distance));
}

std::uint32_t ParticleVisualObject::SetCollectionEnabled(
    ParticleVisualState& state, std::uint8_t enabled,
    IParticleCollectionStateRuntime& runtime) {
    if (state.ReadWord(124u) != 1u) return 0u;
    auto cursor = state.ReadWord(176u);
    auto end = state.ReadWord(180u);
    while (cursor != end) {
        runtime.SetElementEnabled(runtime.ReadWord(cursor), enabled);
        cursor += 4u;
        end = state.ReadWord(180u);
    }
    return end;
}

int ParticleVisualObject::DispatchUpdate(
    ParticleVisualState& state, double delta,
    IParticleVisualUpdateRuntime& runtime) {
    const auto type = state.ReadWord(124u);
    if (type == 0u)
        return EmitTankParticles(state, delta, runtime.EmissionRuntime());
    if (type == 1u)
        return EmitCollectionParticles(state, delta,
                                       runtime.EmissionRuntime());
    return static_cast<int>(type - 1u);
}

void ParticleVisualObject::AppendParticle(
    ParticleVisualState& state, std::uint32_t item,
    IParticleEmissionRuntime& runtime) {
    const auto begin = state.ReadWord(160u);
    const auto end = state.ReadWord(164u);
    const auto capacity_end = state.ReadWord(168u);
    if ((capacity_end - end) / 4u != 0u) {
        runtime.WriteWord(end, item);
        state.WriteWord(164u, end + 4u);
        return;
    }
    const auto size = begin == 0u ? 0u : (end - begin) / 4u;
    const auto capacity = size + std::max(size, 1u);
    const auto new_begin = runtime.Allocate(capacity * 4u);
    for (std::uint32_t index = 0u; index < size; ++index)
        runtime.WriteWord(new_begin + index * 4u,
                          runtime.ReadWord(begin + index * 4u));
    runtime.WriteWord(new_begin + size * 4u, item);
    runtime.FreeBuffer(begin);
    state.WriteWord(160u, new_begin);
    state.WriteWord(164u, new_begin + (size + 1u) * 4u);
    state.WriteWord(168u, new_begin + capacity * 4u);
}

bool ParticleVisualObject::EmitParticles(
    ParticleVisualState& state, double delta,
    IParticleEmissionRuntime& runtime) {
    // 646B1E/646ECE: game x87 PC24 rounds FADD before FCOMP.
    const auto elapsed = static_cast<float>(static_cast<double>(
        std::bit_cast<float>(state.ReadWord(148u))) + delta);
    state.WriteWord(148u,
                    std::bit_cast<std::uint32_t>(static_cast<float>(elapsed)));
    if (!(elapsed >= static_cast<double>(
              std::bit_cast<float>(state.ReadWord(152u)))))
        return false;

    const auto spread = std::bit_cast<float>(state.ReadWord(272u));
    state.WriteWord(148u, 0u);
    const auto sample = runtime.RandomFor(ParticleRandomSite::period);
    const auto next_period = ScaleRandomSample(sample,StateFloat(state,268u),spread);
    state.WriteWord(152u, std::bit_cast<std::uint32_t>(
                              static_cast<float>(next_period)));

    std::int32_t limit = static_cast<std::int32_t>(state.ReadWord(200u));
    if (state.ReadByte(5u) != 0u) {
        limit = static_cast<std::int32_t>(state.ReadWord(12u));
        const float consumed = static_cast<float>(
            static_cast<double>(static_cast<std::int32_t>(state.ReadWord(8u))) * delta);
        const float balance = static_cast<float>(static_cast<double>(limit) - consumed);
        const auto remaining = TruncateInt(balance);
        state.WriteWord(12u, static_cast<std::uint32_t>(
                                 std::max(remaining, 0)));
    }
    const auto active = static_cast<std::int32_t>(state.ReadWord(208u));
    if (active >= limit) return true;

    const auto variation = static_cast<std::int32_t>(state.ReadWord(264u));
    std::int32_t jitter = 0;
    if (variation != 0) {
        const auto magnitude = std::abs(static_cast<std::int64_t>(variation));
        jitter = static_cast<std::int32_t>(
            static_cast<std::int64_t>(runtime.RandomFor(ParticleRandomSite::count)) %
                (2 * magnitude + 1) - magnitude);
    }
    auto requested = static_cast<std::int32_t>(state.ReadWord(260u)) + jitter;
    if (static_cast<std::int64_t>(active) + requested > limit)
        requested = limit - active;
    std::int32_t reused = 0;
    for (auto entry = state.ReadWord(160u);
         entry != state.ReadWord(164u);
         entry += 4u) {
        const auto item = runtime.ReadWord(entry);
        if (runtime.ReadByte(item) == 0u) {
            InitializeDynamicProperties(state, item, runtime);
            ++reused;
            state.WriteWord(208u, state.ReadWord(208u) + 1u);
            // 646C39/646FE9 test the count only after reviving a slot,
            // including when jitter made the requested count nonpositive.
            if (reused >= requested) return true;
        }
    }
    for (auto remaining = requested - reused; remaining > 0; --remaining) {
        const auto item = runtime.Allocate(92u);
        runtime.WriteWord(item + 80u, 0u);
        runtime.WriteWord(item + 84u, 0u);
        runtime.WriteWord(item + 88u, 0u);
        if (state.ReadByte(316u) != 0u)
            runtime.WriteWord(item + 80u, runtime.Allocate(32u));
        if (state.ReadByte(384u) != 0u)
            runtime.WriteWord(item + 84u, runtime.Allocate(16u));
        if (state.ReadByte(420u) != 0u)
            runtime.WriteWord(item + 88u, runtime.Allocate(8u));
        InitializeDynamicProperties(state, item, runtime);
        AppendParticle(state, item, runtime);
        state.WriteWord(204u, state.ReadWord(204u) + 1u);
        state.WriteWord(208u, state.ReadWord(208u) + 1u);
    }
    return true;
}

int ParticleVisualObject::EmitTankParticles(
    ParticleVisualState& state, double delta,
    IParticleEmissionRuntime& runtime) {
    (void)EmitParticles(state, delta, runtime);
    const auto atomic = state.ReadWord(40u);
    auto plugin = runtime.ReadWord(atomic + runtime.PluginOffset());
    runtime.WriteWord(plugin + 64u,
                      runtime.ReadWord(plugin + 64u) | 0x00800000u);
    plugin = runtime.ReadWord(atomic + runtime.PluginOffset());
    runtime.WriteWord(plugin + 4u, state.ReadWord(204u));
    return static_cast<int>(plugin);
}

int ParticleVisualObject::EmitCollectionParticles(
    ParticleVisualState& state, double delta,
    IParticleEmissionRuntime& runtime) {
    return EmitParticles(state, delta, runtime) ? 0 : 0x100;
}

void ParticleVisualObject::InitializeDynamicProperties(
    ParticleVisualState& state, std::uint32_t item,
    IParticleEmissionRuntime& runtime) {
    if (item == 0u) return;

    runtime.WriteByte(item, 1u);
    runtime.WriteWord(item + 4u, 0u);
    const float lifetime = RandomRange(runtime, StateFloat(state, 276u),
                                       StateFloat(state, 280u));
    const float speed = RandomRange(runtime, StateFloat(state, 284u),
                                    StateFloat(state, 288u));
    WriteExternalFloat(runtime, item + 8u, lifetime);
    WriteExternalFloat(runtime, item + 12u, speed);
    runtime.WriteWord(item + 16u, state.ReadWord(236u));

    auto direction = runtime.Normalize({
        RandomRange(runtime, StateFloat(state, 292u), StateFloat(state, 304u)),
        RandomRange(runtime, StateFloat(state, 296u), StateFloat(state, 308u)),
        RandomRange(runtime, StateFloat(state, 300u), StateFloat(state, 312u))});
    WriteExternalFloat(runtime, item + 56u, direction.x);
    WriteExternalFloat(runtime, item + 60u, direction.y);
    WriteExternalFloat(runtime, item + 64u, direction.z);
    for (std::uint32_t offset = 20u; offset <= 40u; offset += 4u)
        runtime.WriteWord(item + offset, 0u);
    WriteExternalFloat(runtime, item + 44u, speed * direction.x);
    WriteExternalFloat(runtime, item + 48u, speed * direction.y);
    WriteExternalFloat(runtime, item + 52u, speed * direction.z);

    ParticleVisualPoint position{};
    if (state.ReadByte(55u) != 0u) {
        const auto begin = state.ReadWord(64u);
        const auto count = begin == 0u ? 0 :
            static_cast<std::int32_t>((state.ReadWord(68u) - begin) / 12u);
        const auto last = count - 1;
        std::int32_t index = 0;
        if (last > 0)
            index = runtime.RandomFor(ParticleRandomSite::position) % (last + 1);
        else if (last < 0)
            index = last + runtime.RandomFor(ParticleRandomSite::position) % (1 - last);
        const auto vertex = begin + static_cast<std::uint32_t>(index * 12);
        position = {ExternalFloat(runtime, vertex),
                    ExternalFloat(runtime, vertex + 4u),
                    ExternalFloat(runtime, vertex + 8u)};
        position.x = RandomRange(runtime, position.x, StateFloat(state, 76u), ParticleRandomSite::position);
        position.y = RandomRange(runtime, position.y, StateFloat(state, 80u), ParticleRandomSite::position);
        position.z = RandomRange(runtime, position.z, StateFloat(state, 84u), ParticleRandomSite::position);
    } else if (state.ReadWord(128u) == 0u) {
        position.x = RandomRange(runtime, 0.f, StateFloat(state, 132u) * 0.5f, ParticleRandomSite::position);
        position.z = RandomRange(runtime, 0.f, StateFloat(state, 136u) * 0.5f, ParticleRandomSite::position);
        position.y = RandomRange(runtime, 0.f, StateFloat(state, 140u) * 0.5f, ParticleRandomSite::position);
    } else if (state.ReadWord(128u) == 1u) {
        constexpr double angle_scale = 0.00019175343913957477;
        const auto angle = static_cast<float>(
            static_cast<double>(runtime.RandomFor(ParticleRandomSite::position)) * angle_scale);
        const auto inner = StateFloat(state, 132u) * 0.5f;
        const auto outer = StateFloat(state, 136u) * 0.5f;
        position.x = static_cast<float>(
            (static_cast<double>(runtime.RandomFor(ParticleRandomSite::position)) *
                 (static_cast<double>(outer) - inner) *
                 3.0518509447574615e-05 + inner) * std::sin(angle));
        position.z = static_cast<float>(
            (static_cast<double>(runtime.RandomFor(ParticleRandomSite::position)) *
                 (static_cast<double>(outer) - inner) *
                 3.0518509447574615e-05 + inner) * std::cos(angle));
        position.y = RandomRange(runtime, 0.f, StateFloat(state, 140u) * 0.5f, ParticleRandomSite::position);
    } else {
        float x, y, z, squared;
        do {
            x = RandomSphereCoordinate(runtime);
            y = RandomSphereCoordinate(runtime);
            z = RandomSphereCoordinate(runtime);
            // 647945..647968: Z^2+Y^2 first, then X^2. Equality is accepted.
            const float z_squared = z * z;
            const float y_squared = y * y;
            const float zy_squared = z_squared + y_squared;
            const float x_squared = x * x;
            squared = zy_squared + x_squared;
        } while (squared > 1.f);
        const auto diameter = StateFloat(state, 132u);
        const auto scale_position = [diameter](float coordinate) {
            const float scaled = coordinate * diameter;
            return scaled * 0.5f;
        };
        position = {scale_position(x),scale_position(y),scale_position(z)};
    }
    if (state.ReadByte(55u) == 0u && state.ReadByte(4u) == 0u) {
        const auto frame = state.ReadWord(44u);
        const auto matrix = state.ReadWord(124u) == 1u &&
                                    state.ReadByte(53u) != 0u &&
                                    state.ReadByte(54u) == 0u
                                ? frame + 16u : runtime.ObjectTransform(frame);
        position = runtime.TransformPosition(matrix, position);
    }
    WriteExternalFloat(runtime, item + 68u, position.x);
    WriteExternalFloat(runtime, item + 72u, position.y);
    WriteExternalFloat(runtime, item + 76u, position.z);

    if (state.ReadByte(3u) != 0u || state.ReadByte(316u) == 0u) {
        for (std::uint32_t channel = 0; channel < 4u; ++channel)
            runtime.WriteWord(item + 28u + channel * 4u,
                              state.ReadWord(212u + channel * 4u));
    } else {
        const auto curve = runtime.ReadWord(item + 80u);
        const double inv_lifetime = 1.0 / lifetime;
        for (std::uint32_t channel = 0; channel < 4u; ++channel) {
            const auto start = RandomRange(runtime,
                StateFloat(state, 320u + channel * 4u),
                StateFloat(state, 336u + channel * 4u), ParticleRandomSite::color);
            WriteExternalFloat(runtime, curve + channel * 4u, start);
            // The native code samples all four starts before any end value.
        }
        for (std::uint32_t channel = 0; channel < 4u; ++channel) {
            const auto end = RandomRange(runtime,
                StateFloat(state, 352u + channel * 4u),
                StateFloat(state, 368u + channel * 4u), ParticleRandomSite::color);
            WriteExternalFloat(runtime, curve + 16u + channel * 4u,
                static_cast<float>(
                    (static_cast<double>(end) -
                     static_cast<double>(ExternalFloat(
                         runtime, curve + channel * 4u))) * inv_lifetime));
        }
    }
    if (state.ReadByte(384u) != 0u) {
        const auto curve = runtime.ReadWord(item + 84u);
        const auto start_x = RandomRange(runtime, StateFloat(state, 388u),
                                         StateFloat(state, 396u), ParticleRandomSite::size);
        const auto start_y = RandomRange(runtime, StateFloat(state, 392u),
                                         StateFloat(state, 400u), ParticleRandomSite::size);
        const auto end_x = RandomRange(runtime, StateFloat(state, 404u),
                                       StateFloat(state, 412u), ParticleRandomSite::size);
        const auto end_y = RandomRange(runtime, StateFloat(state, 408u),
                                       StateFloat(state, 416u), ParticleRandomSite::size);
        const auto inv_lifetime = 1.0 / lifetime;
        WriteExternalFloat(runtime, curve, start_x);
        WriteExternalFloat(runtime, curve + 4u, start_y);
        WriteExternalFloat(runtime, curve + 8u,
                           static_cast<float>((static_cast<double>(end_x) -
                               start_x) * inv_lifetime));
        WriteExternalFloat(runtime, curve + 12u,
                           static_cast<float>((static_cast<double>(end_y) -
                               start_y) * inv_lifetime));
    }
    if (state.ReadByte(420u) != 0u) {
        const auto curve = runtime.ReadWord(item + 88u);
        const auto start = RandomRange(runtime, StateFloat(state, 424u),
                                       StateFloat(state, 428u), ParticleRandomSite::rotation);
        const auto end = RandomRange(runtime, StateFloat(state, 432u),
                                     StateFloat(state, 436u), ParticleRandomSite::rotation);
        WriteExternalFloat(runtime, curve, start);
        WriteExternalFloat(runtime, curve + 4u, end - start);
        WriteExternalFloat(runtime, item + 16u, start);
    }
}

int ParticleVisualObject::DispatchRender(
    ParticleVisualState& state, std::uint32_t low,
    std::uint32_t high, std::uint32_t argument,
    IParticleVisualUpdateRuntime& runtime) {
    const auto type = state.ReadWord(124u);
    if (type == 0u) return RenderTank(state, runtime.TankUpdateRuntime());
    if (type == 1u)
        return RenderCollection(state, low, high, argument,
                                runtime.VisualRenderRuntime());
    return static_cast<int>(type - 1u);
}

int ParticleVisualObject::RenderTank(
    ParticleVisualState& state, IParticleTankUpdateRuntime& runtime) {
    const auto atomic = state.ReadWord(40u);
    constexpr std::uint32_t write_access = 0x40000000u;
    auto position = runtime.Lock(atomic, 1u, write_access);
    auto color = runtime.Lock(atomic, 2u, write_access);
    auto size = runtime.Lock(atomic, 4u, write_access);
    auto uv = runtime.Lock(atomic, 128u, write_access);
    auto rotation = runtime.Lock(atomic, 32u, write_access);

    const auto read_float = [&runtime](std::uint32_t address) {
        return std::bit_cast<float>(runtime.ReadWord(address));
    };
    const auto write_float = [&runtime](std::uint32_t address, float value) {
        runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
    };

    for (std::int32_t index = 0;
         index < static_cast<std::int32_t>(state.ReadWord(204u)); ++index) {
        const auto item = runtime.ReadWord(
            state.ReadWord(160u) + static_cast<std::uint32_t>(index) * 4u);
        if (runtime.ReadByte(item) == 0u) {
            for (std::uint32_t channel = 0u; channel < 4u; ++channel)
                runtime.WriteByte(color.address + channel, 0u);
        } else {
            const auto fade = static_cast<float>(
                1.0 - static_cast<double>(read_float(item + 4u)) /
                          static_cast<double>(read_float(item + 8u)));
            ParticleVisualPoint point{
                read_float(item + 68u), read_float(item + 72u),
                read_float(item + 76u)};
            if (state.ReadByte(4u) != 0u)
                point = runtime.TransformPosition(
                    runtime.ObjectTransform(state.ReadWord(44u)), point);
            write_float(position.address, point.x);
            write_float(position.address + 4u, point.y);
            write_float(position.address + 8u, point.z);

            runtime.WriteByte(color.address,
                              ColorByte(read_float(item + 28u)));
            runtime.WriteByte(color.address + 1u,
                              ColorByte(read_float(item + 32u)));
            runtime.WriteByte(color.address + 2u,
                              ColorByte(read_float(item + 36u)));
            const auto alpha = ColorByte(read_float(item + 40u));
            const auto multiplier = state.ReadByte(2u) != 0u ? fade : 1.f;
            runtime.WriteByte(color.address + 3u,
                static_cast<std::uint8_t>(TruncateInt(
                    static_cast<float>(alpha * multiplier))));

            runtime.WriteWord(size.address, runtime.ReadWord(item + 20u));
            runtime.WriteWord(size.address + 4u,
                              runtime.ReadWord(item + 24u));
            runtime.WriteWord(uv.address, 0u);
            runtime.WriteWord(uv.address + 4u, 0u);
            runtime.WriteWord(uv.address + 8u, 0x3f800000u);
            runtime.WriteWord(uv.address + 12u, 0x3f800000u);
            runtime.WriteWord(rotation.address,
                              runtime.ReadWord(item + 16u));
        }
        position.address += position.stride;
        color.address += color.stride;
        size.address += size.stride;
        uv.address += uv.stride;
        rotation.address += rotation.stride;
    }
    return runtime.Unlock(atomic);
}

int ParticleVisualObject::RenderCollection(
    ParticleVisualState& state, std::uint32_t low,
    std::uint32_t high, std::uint32_t argument,
    IParticleVisualRenderRuntime& runtime) {
    const auto frame = state.ReadWord(44u);
    const auto matrix = runtime.ObjectTransform(frame);
    const std::array<std::uint32_t, 3u> origin{
        runtime.ReadWord(matrix + 48u), runtime.ReadWord(matrix + 52u),
        runtime.ReadWord(matrix + 56u)};
    const auto read_float = [&runtime](std::uint32_t address) {
        return std::bit_cast<float>(runtime.ReadWord(address));
    };
    const auto write_float = [&runtime](std::uint32_t address, float value) {
        runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
    };

    for (std::int32_t index = 0;
         index < static_cast<std::int32_t>(state.ReadWord(204u)); ++index) {
        const auto offset = static_cast<std::uint32_t>(index) * 4u;
        const auto item = runtime.ReadWord(state.ReadWord(160u) + offset);
        const auto visual = runtime.ReadWord(state.ReadWord(176u) + offset);
        if (runtime.ReadByte(item) == 0u) {
            runtime.SetColor(runtime.ReadWord(visual + 48u), {});
        } else {
            const auto fade = static_cast<float>(
                1.0 - static_cast<double>(read_float(item + 4u)) /
                          static_cast<double>(read_float(item + 8u)));
            ParticleVisualPoint point{
                read_float(item + 68u), read_float(item + 72u),
                read_float(item + 76u)};
            if ((state.ReadWord(124u) != 1u ||
                 state.ReadByte(53u) == 0u) &&
                state.ReadByte(4u) != 0u)
                point = runtime.TransformPosition(
                    runtime.ObjectTransform(frame), point);
            write_float(visual + 28u, point.x);
            write_float(visual + 32u, point.y);
            write_float(visual + 36u, point.z);
            runtime.InitializeTransform(visual);

            const auto alpha = ColorByte(read_float(item + 40u));
            const auto multiplier = state.ReadByte(2u) != 0u ? fade : 1.f;
            const std::array<std::uint8_t, 4u> color{
                ColorByte(read_float(item + 28u)),
                ColorByte(read_float(item + 32u)),
                ColorByte(read_float(item + 36u)),
                static_cast<std::uint8_t>(TruncateInt(
                    static_cast<float>(alpha * multiplier)))};
            runtime.SetColor(runtime.ReadWord(visual + 48u), color);

            const auto property = runtime.ReadWord(visual + 48u);
            const auto width = read_float(property + 20u);
            const auto height = read_float(property + 24u);
            const auto width_divisor = width <= 0.f ? 1.f : width;
            const auto height_divisor = height <= 0.f ? 1.f : height;
            write_float(visual + 40u, static_cast<float>(
                static_cast<double>(read_float(item + 20u)) /
                width_divisor));
            write_float(visual + 44u, static_cast<float>(
                static_cast<double>(read_float(item + 24u)) /
                height_divisor));
            runtime.WriteWord(visual + 24u, runtime.ReadWord(item + 16u));
            for (std::uint32_t word = 0u; word < 3u; ++word)
                runtime.WriteWord(visual + word * 4u, origin[word]);
        }
        runtime.UpdateVisual(visual, low, high, argument);
    }
    return static_cast<std::int32_t>(state.ReadWord(204u));
}

int ParticleVisualObject::DispatchEvent(
    ParticleVisualState& state, std::uint32_t event,
    IParticleVisualUpdateRuntime& runtime) {
    const auto type = state.ReadWord(124u);
    if (type == 0u) return RenderState(state, runtime.RenderStateRuntime());
    if (type != 1u) return static_cast<int>(type - 1u);

    std::int32_t count = static_cast<std::int32_t>(state.ReadWord(204u));
    for (std::int32_t index = 0; index < count; ++index) {
        const auto element = runtime.ReadExternalWord(
            state.ReadWord(176u) + static_cast<std::uint32_t>(index) * 4u);
        const auto visual = runtime.ReadExternalWord(element + 48u);
        if (runtime.ReadExternalByte(visual + 31u) != 0u)
            runtime.RenderCollectionElement(element, event, state);
        count = static_cast<std::int32_t>(state.ReadWord(204u));
    }
    return count;
}

int ParticleVisualObject::RenderState(
    ParticleVisualState& state, IParticleRenderStateRuntime& runtime) {
    if (state.ReadByte(0u) == 0u) return 0;
    const auto old_27 = runtime.GetState(27u);
    const auto old_19 = runtime.GetState(19u);
    const auto old_20 = runtime.GetState(20u);
    (void)runtime.SetState(27u, state.ReadByte(188u));
    (void)runtime.SetState(19u, state.ReadWord(192u));
    (void)runtime.SetState(20u, state.ReadWord(196u));
    const auto old_7 = runtime.GetState(7u);
    const auto old_14 = runtime.GetState(14u);
    (void)runtime.SetState(7u, 1u);
    (void)runtime.SetState(14u, 0u);
    runtime.RenderAtomic(state.ReadWord(40u));
    (void)runtime.SetState(7u, old_7);
    (void)runtime.SetState(14u, old_14);
    (void)runtime.SetState(27u, old_27);
    (void)runtime.SetState(19u, old_19);
    return runtime.SetState(20u, old_20);
}

}  // namespace pal4
