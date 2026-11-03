#include "pal4/gob_object_base.h"

#include <bit>
#include <array>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace pal4 {

std::uint32_t GobObjectBase::Construct(
    const std::uint32_t object, IGobObjectConstructRuntime& runtime) {
    runtime.BaseConstruct(object);
    try {
        runtime.InitializeEmptyString(object + 0x178u);
        try {
            runtime.ConstructNameNode(object + 0x1ccu);
        } catch (...) {
            runtime.CleanupString(object + 0x178u);
            throw;
        }
    } catch (...) {
        runtime.BaseDestruct(object);
        throw;
    }
    for (const auto& [offset, value] : {
             std::pair{0x1d4u, 0xffffffffu},
             std::pair{0x1d8u, 0u},
             std::pair{0x174u, 0u},
             std::pair{0x188u, 0u},
             std::pair{0x194u, 0u},
             std::pair{0x1b0u, 0u},
             std::pair{0x1b4u, 0xffffffffu},
             std::pair{0x1b8u, 0u},
             std::pair{0x1bcu, 0xffffffffu},
             std::pair{0x1c0u, 0u},
             std::pair{0x1c8u, 0u},
             std::pair{0x1c4u, 0u}})
        runtime.WriteWord(object + offset, value);
    runtime.WriteWord(object, kVtable);
    for (const auto offset : {0x170u, 0x184u, 0x18cu, 0x190u, 0x198u,
                              0x1a0u, 0x1a4u, 0x1a8u})
        runtime.WriteWord(object + offset, 1u);
    runtime.WriteWord(object + 0x19cu,
                      std::bit_cast<std::uint32_t>(20.0f));
    runtime.WriteWord(object + 0x1acu,
                      std::bit_cast<std::uint32_t>(1.0f));
    return object;
}

void GobObjectBase::Destruct(
    const std::uint32_t object, IGobObjectConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.ResetEffect(object + 0x1ccu);
    runtime.CleanupString(object + 0x178u);
    runtime.BaseDestruct(object);
}

std::uint32_t GobObjectBase::CleanupResources(
    const std::uint32_t object, const std::uint32_t event_context,
    IGobBaseCleanupRuntime& runtime) {
    if (runtime.EffectNeedsUpdate(object + 0x1ccu))
        runtime.HandleEffectEvent(object + 0x1ccu, event_context, 1u);
    if (runtime.EffectReadyToShow(object + 0x1d4u))
        (void)runtime.RemoveEffect(object + 0x1d4u);
    auto result = runtime.CleanupClump(object);
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document != 0u) {
        runtime.DestructXmlDocument(document);
        result = runtime.FreeMemory(document);
        runtime.WriteWord(object + 0x1c0u, 0u);
    }
    return result;
}

std::uint32_t GobObjectBase::RenderIfVisible(std::uint32_t object,std::uint32_t camera,IGobObjectBaseRuntime& runtime){
    class Render final : public IGobObjectRenderRuntime {
        IGobObjectBaseRuntime& runtime;
    public:
        explicit Render(IGobObjectBaseRuntime& r):runtime(r){}
        std::uint32_t ReadWord(std::uint32_t a)override{return runtime.ReadWord(a);}
        std::uint32_t RenderClump(std::uint32_t o,std::uint32_t c)override{return runtime.RenderClump(o,c);}
    } render(runtime);
    return RenderIfVisible(object,camera,render);
}
std::uint32_t GobObjectBase::RenderIfVisible(
    const std::uint32_t object, const std::uint32_t argument,
    IGobObjectRenderRuntime& runtime) {
    if (runtime.ReadWord(object + 0x184u) != 0u ||
        runtime.ReadWord(object + 0x1c4u) != 0u)
        return runtime.RenderClump(object, argument);
    return 0u;
}

std::uint32_t GobObjectBase::UpdateClumpIfVisible(
    const std::uint32_t object, const float delta_seconds,
    IGobObjectBaseRuntime& runtime) {
    if (runtime.ReadWord(object + 0x184u) != 0u ||
        runtime.ReadWord(object + 0x1c4u) != 0u)
        return runtime.UpdateClump(object, delta_seconds);
    return 0u;
}

std::uint32_t GobObjectBase::CreateParametersNode(
    const std::uint32_t object, IGobObjectDocumentRuntime& runtime) {
    const auto old_document = runtime.ReadWord(object + 0x1c0u);
    if (old_document != 0u) {
        runtime.DestructXmlDocument(old_document);
        (void)runtime.FreeMemory(old_document);
        runtime.WriteWord(object + 0x1c0u, 0u);
    }
    const auto memory = runtime.AllocateMemory(0x3cu);
    std::uint32_t document{};
    try {
        document = memory == 0u ? 0u :
            runtime.InitializeXmlDocument(memory, "PAL4-GameObject", 0u);
    } catch (...) {
        if(memory) (void)runtime.FreeMemory(memory);
        throw;
    }
    runtime.WriteWord(object + 0x1c0u, document);
    try {return runtime.CreateXmlNode(document,"PAL4-GameObject-Parameters");}
    catch (...) {
        if(document) {
            runtime.DestructXmlDocument(document);
            (void)runtime.FreeMemory(document);
            runtime.WriteWord(object+0x1c0u,0u);
        }
        throw;
    }
}

std::uint32_t GobObjectBase::LoadAndInitialize(
    const std::uint32_t object, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t third,
    const std::uint32_t variant_name,
    IGobObjectBaseRuntime& runtime) {
    const auto loaded = runtime.LoadClump(object, first, second, third);
    if (loaded == 0u) return loaded;
    runtime.HideUserDataAtomics(runtime.ReadWord(object + 0x14u));
    if (variant_name != 0u) {
        const auto variant = runtime.ReadCString(variant_name);
        if (!variant.empty()) {
            const auto plugin_context = runtime.ReadWord(object + 0x10u);
            if (plugin_context != 0u &&
                runtime.CanUseVariant(plugin_context, variant_name)) {
                const auto directory = runtime.ReadCString(
                    runtime.ReadWord(object + 8u));
                const auto texture = runtime.FindTexture(
                    directory + variant + ".png");
                if (texture != 0u)
                    runtime.LoadTexture(object, variant_name, texture);
            }
        }
    }
    (void)CreateParametersNode(object, runtime);
    return 1u;
}

void GobObjectBase::ApplyParameters(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return;

    class DocumentValues final : public IGobObjectParameterValueRuntime {
    public:
        DocumentValues(std::uint32_t document, IGobObjectBaseRuntime& runtime)
            : document_(document), runtime_(runtime) {}

        float ReadFloat(std::string_view name, float fallback) override {
            const auto attribute = runtime_.FindConfigAttribute(document_, name);
            return attribute == 0u ? fallback :
                runtime_.ConfigAttributeToFloat(attribute, fallback);
        }
        std::int32_t ReadInt(std::string_view name,
                             std::int32_t fallback) override {
            const auto attribute = runtime_.FindConfigAttribute(document_, name);
            return attribute == 0u ? fallback :
                runtime_.ConfigAttributeToInt(attribute, fallback);
        }
        std::int32_t FloatToInteger(float value) override {
            return runtime_.FloatToInteger(value);
        }
        void WriteWord(std::uint32_t address,
                       std::uint32_t value) override {
            runtime_.WriteWord(address, value);
        }
        void ApplyScale(std::uint32_t object, float scale,
                        std::uint32_t mode) override {
            runtime_.ApplyScale(object, scale, mode);
        }

    private:
        std::uint32_t document_;
        IGobObjectBaseRuntime& runtime_;
    } values(document, runtime);
    ApplyParameters(object, values);
}

void GobObjectBase::ApplyParameters(
    const std::uint32_t object, IGobObjectParameterValueRuntime& runtime) {
    const auto write_int = [&](const std::uint32_t offset,
                               const std::int32_t value) {
        runtime.WriteWord(object + offset,
                          static_cast<std::uint32_t>(value));
    };

    const auto scale = runtime.ReadFloat("PAL4-GameObject-object-scale", 1.0f);
    runtime.WriteWord(object + 0x1acu,
                      std::bit_cast<std::uint32_t>(scale));
    write_int(0x190u, runtime.ReadInt(
        "PAL4-GameObject-object-auto-disappear", 1));
    write_int(0x194u, runtime.ReadInt(
        "PAL4-GameObject-object-camera-collide", 0));
    write_int(0x198u, runtime.FloatToInteger(runtime.ReadFloat(
        "PAL4-GameObject-object-research-num", 1.0f)));
    write_int(0x1a0u, runtime.FloatToInteger(runtime.ReadFloat(
        "PAL4_GameObject-object-height-limit", 1.0f)));
    write_int(0x1a4u, runtime.FloatToInteger(runtime.ReadFloat(
        "PAL4-GameObject-object-store", 1.0f)));
    write_int(0x1a8u, runtime.ReadInt("PAL4-GameObject-object-clip", 1));
    write_int(0x1b0u, runtime.ReadInt("PAL4-GameObject-object-marker", 0));
    write_int(0x1b4u, runtime.ReadInt("PAL4-GameObject-object-validfor", -1));
    write_int(0x1b8u, runtime.ReadInt("PAL4-GameObject-object-hide", 0));
    write_int(0x1bcu, runtime.ReadInt("PAL4-GameObject-object-hint", -1));
    runtime.ApplyScale(object, scale, 0u);
}

std::uint32_t GobObjectBase::LoadRecord(
    const std::uint32_t object, const std::uint32_t context,
    const std::uint32_t stream, IGobObjectBaseRuntime& runtime) {
    if (stream == 0u) return 0u;

    std::array<std::uint32_t, 5> buffers{};
    for (std::size_t i = 0; i < 4u; ++i)
        buffers[i] = runtime.ReadStreamBuffer(stream);

    (void)runtime.InitializeFromRecord(
        object, buffers[0], buffers[1], buffers[2], buffers[3], context);
    runtime.ReadStreamBytes(stream, object + 0xacu, 0xcu);
    runtime.ReadStreamBytes(stream, object + 0xa0u, 0xcu);
    buffers[4] = runtime.ReadStreamBuffer(stream);
    runtime.AssignScriptName(object + 0x178u,
                             runtime.ReadCString(buffers[4]));
    for (const auto offset : {0x184u, 0x188u, 0x18cu, 0x19cu})
        runtime.ReadStreamBytes(stream, object + offset, 4u);
    const auto type = runtime.ReadStreamType(stream);
    if (type == 1u || type == 2u)
        runtime.CheckRecordXml(stream,
                               runtime.ReadWord(object + 0x1c0u));
    runtime.FinalizeRecord(object);
    runtime.ApplyPosition(object, object + 0xacu, 0u);
    runtime.ApplyRotation(object, object + 0xa0u, 0u);
    runtime.ApplyRecordVisibility(
        object, runtime.ReadWord(object + 0x184u));
    for (std::size_t i = 0; i < 4u; ++i)
        runtime.FreeStreamBuffer(buffers[i]);
    runtime.FreeStreamBuffer(0u);
    return buffers[4];
}

std::uint32_t GobObjectBase::ProcessMarkerIfSet(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return ProcessMarkerIfSet(object, static_cast<IGobResearchUseRuntime&>(runtime));
}

std::uint32_t GobObjectBase::ProcessMarkerIfSet(
    const std::uint32_t object, IGobResearchUseRuntime& runtime) {
    const auto marker = runtime.ReadWord(object + 0x1b0u);
    return marker == 0u ? marker : runtime.ProcessMarker();
}

bool GobObjectBase::IsWithinInteractionBounds(
    const std::uint32_t object, const Position& position,
    const float vertical_radius, float* horizontal_distance,
    IGobInteractionBoundsRuntime& runtime) {
    const auto object_x = std::bit_cast<float>(
        runtime.ReadWord(object + 0xacu));
    const auto object_z = std::bit_cast<float>(
        runtime.ReadWord(object + 0xb4u));
    const auto dx = position.x - object_x;
    const auto dz = position.z - object_z;
    // IDA59A7FF calls668530, stores the returned approximate length as float,
    // then compares the virtual +144 range. The output is a length too.
    const auto distance = static_cast<float>(runtime.PlanarMagnitude(dx, dz));
    const auto threshold = runtime.InteractionThreshold(object);
    bool within = !std::isunordered(threshold, distance) &&
                  threshold >= distance;
    if (within && runtime.ReadWord(object + 0x1a0u) != 0u) {
        const auto object_y = std::bit_cast<float>(
            runtime.ReadWord(object + 0xb0u));
        within = position.y + vertical_radius > object_y &&
                 position.y - vertical_radius < object_y;
    }
    if (horizontal_distance != nullptr)
        *horizontal_distance = distance;
    return within;
}

bool GobObjectBase::IsResearchVisibleCandidate(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return runtime.ReadWord(object + 0x18cu) != 0u &&
           runtime.ReadWord(object + 0x198u) != 0u;
}

std::uint8_t GobObjectBase::SetVisible(
    const std::uint32_t object, const std::uint32_t visible,
    IGobObjectVisibilityRuntime& runtime) {
    runtime.WriteWord(object + 0x184u, visible);
    if (visible != 0u) return static_cast<std::uint8_t>(visible);
    if (runtime.DialogNeedsUpdate(object + 0x1ccu))
        runtime.UpdateDialogContent(object + 0x1ccu);
    if (!runtime.EffectReadyToShow(object + 0x1d4u)) return 0u;
    return static_cast<std::uint8_t>(
        runtime.RemoveEffect(object + 0x1d4u));
}

bool GobObjectBase::IsMarkedVisibleWithUses(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return runtime.ReadWord(object + 0x1b0u) != 0u &&
           runtime.ReadWord(object + 0x184u) != 0u &&
           runtime.ReadWord(object + 0x198u) != 0u;
}

std::uint32_t GobObjectBase::ConsumeResearchUse(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return ConsumeResearchUse(object, static_cast<IGobResearchUseRuntime&>(runtime));
}

std::uint32_t GobObjectBase::ConsumeResearchUse(
    const std::uint32_t object, IGobResearchUseRuntime& runtime) {
    auto remaining = static_cast<std::int32_t>(
        runtime.ReadWord(object + 0x198u));
    if (remaining > 0) {
        --remaining;
        runtime.WriteWord(object + 0x198u,
                          static_cast<std::uint32_t>(remaining));
    }
    if (remaining != 0)
        return static_cast<std::uint32_t>(remaining);
    if (runtime.ReadWord(object + 0x190u) != 0u)
        runtime.AddActiveSceneObject(object, 1.0f);
    return ProcessMarkerIfSet(object, runtime);
}

std::uint32_t GobObjectBase::AlwaysOne() { return 1u; }

std::uint32_t GobObjectBase::AutoDisappear(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return runtime.ReadWord(object + 0x190u);
}

std::uint32_t GobObjectBase::CameraCollide(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return runtime.ReadWord(object + 0x194u);
}

float GobObjectBase::InteractionRangeThreshold(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(object + 0x19cu));
}

std::uint32_t GobObjectBase::ConfigNode(
    const std::uint32_t object, IGobObjectBaseRuntime& runtime) {
    return runtime.ReadWord(object + 0x1c0u);
}

}  // namespace pal4
