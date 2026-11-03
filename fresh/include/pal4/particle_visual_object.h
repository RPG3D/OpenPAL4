#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ParticleVisualState {
    std::array<std::uint8_t, 440u> bytes{};

    void WriteByte(std::size_t offset, std::uint8_t value);
    void WriteWord(std::size_t offset, std::uint32_t value);
    [[nodiscard]] std::uint8_t ReadByte(std::size_t offset) const;
    [[nodiscard]] std::uint32_t ReadWord(std::size_t offset) const;
};
static_assert(sizeof(ParticleVisualState) == 440u);

class IParticleVisualRuntime {
public:
    virtual ~IParticleVisualRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CurrentTime() = 0;
    virtual void SeedRandom(std::uint32_t seed) = 0;
};

struct ParticleVisualPoint {
    float x{};
    float y{};
    float z{};
};
static_assert(sizeof(ParticleVisualPoint) == 12u);

class IParticleFrameTransformRuntime {
public:
    virtual ~IParticleFrameTransformRuntime() = default;
    virtual void SetIdentity(std::uint32_t frame) = 0;
    virtual void Scale(std::uint32_t frame, ParticleVisualPoint scale,
                       std::uint32_t combine) = 0;
    virtual void Rotate(std::uint32_t frame, ParticleVisualPoint axis,
                        float angle, std::uint32_t combine) = 0;
    virtual void Translate(std::uint32_t frame,
                           ParticleVisualPoint position,
                           std::uint32_t combine) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateObjects(
        std::uint32_t frame) = 0;
};

class IParticleTankRuntime {
public:
    virtual ~IParticleTankRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateTank(
        std::uint32_t capacity, std::uint32_t flags,
        std::uint32_t data_flags) = 0;
    [[nodiscard]] virtual std::uint32_t PluginOffset() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetMaterialTexture(std::uint32_t material,
                                    std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetAtomicFrame(std::uint32_t atomic,
                                std::uint32_t frame) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual IParticleFrameTransformRuntime&
        FrameTransformRuntime() = 0;
};

class IParticleCollectionRuntime {
public:
    virtual ~IParticleCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void FreeBuffer(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void InitializeFromAsset(
        std::uint32_t element, std::uint32_t asset,
        std::array<float, 2u> scale, bool mode,
        std::uint8_t option, std::uint32_t enabled) = 0;
    virtual void InitializeFromObject(
        std::uint32_t element, std::array<std::uint32_t, 3u> source,
        std::uint32_t asset, std::array<float, 2u> scale,
        bool mode, std::uint8_t option,
        std::uint32_t enabled) = 0;
    virtual void AttachElementFrame(std::uint32_t frame,
                                    std::uint32_t child) = 0;
    [[nodiscard]] virtual IParticleFrameTransformRuntime&
        FrameTransformRuntime() = 0;
};

class IParticleVisualLifecycleRuntime {
public:
    virtual ~IParticleVisualLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadExternalWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual int Free(std::uint32_t address) = 0;
    virtual void RemoveFrameChild(std::uint32_t frame) = 0;
    [[nodiscard]] virtual int DestroyAtomic(
        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual int DestroyFrame(
        std::uint32_t frame) = 0;
    virtual void DeinitializeElement(std::uint32_t element) = 0;
    [[nodiscard]] virtual IParticleTankRuntime& TankRuntime() = 0;
    [[nodiscard]] virtual IParticleCollectionRuntime& CollectionRuntime() = 0;
    virtual void AddFrameChild(std::uint32_t parent,
                               std::uint32_t child) = 0;
};

class IParticleSkinRuntime {
public:
    virtual ~IParticleSkinRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SourceGeometry(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t Skin(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual int MorphTargetCount(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t VertexCount(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t Hierarchy(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual ParticleVisualPoint GeometryVertex(
        std::uint32_t geometry, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::array<std::uint8_t, 4u> BoneIndices(
        std::uint32_t skin, std::uint32_t vertex) = 0;
    [[nodiscard]] virtual std::array<float, 4u> BoneWeights(
        std::uint32_t skin, std::uint32_t vertex) = 0;
    [[nodiscard]] virtual ParticleVisualPoint TransformBoneVertex(
        std::uint32_t skin, std::uint32_t hierarchy,
        std::uint8_t bone, ParticleVisualPoint vertex) = 0;
    [[nodiscard]] virtual ParticleVisualPoint TransformFrameVertex(
        std::uint32_t frame, ParticleVisualPoint vertex) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePointBuffer(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual ParticleVisualPoint ReadPointBuffer(
        std::uint32_t address) = 0;
    virtual void WritePointBuffer(std::uint32_t address,
                                  ParticleVisualPoint point) = 0;
    virtual void FreePointBuffer(std::uint32_t address) = 0;
};

struct ParticleTankLock {
    std::uint32_t address{};
    std::uint32_t stride{};
};

class IParticleTankUpdateRuntime {
public:
    virtual ~IParticleTankUpdateRuntime() = default;
    [[nodiscard]] virtual ParticleTankLock Lock(
        std::uint32_t atomic, std::uint32_t stream,
        std::uint32_t access) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, ParticleVisualPoint position) = 0;
    [[nodiscard]] virtual int Unlock(std::uint32_t atomic) = 0;
};

class IParticleVisualRenderRuntime {
public:
    virtual ~IParticleVisualRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, ParticleVisualPoint position) = 0;
    virtual void InitializeTransform(std::uint32_t visual) = 0;
    virtual void SetColor(std::uint32_t property,
                          std::array<std::uint8_t, 4u> color) = 0;
    virtual void UpdateVisual(std::uint32_t visual,
                              std::uint32_t low,
                              std::uint32_t high,
                              std::uint32_t argument) = 0;
};

enum class ParticleRandomSite { period, count, properties, position, color, size, rotation };

class IParticleEmissionRuntime {
public:
    virtual ~IParticleEmissionRuntime() = default;
    [[nodiscard]] virtual std::int32_t Random() = 0;
    // Optional observational attribution; ordinary runtimes consume the same
    // one draw through Random(), with no separate generator or state.
    [[nodiscard]] virtual std::int32_t RandomFor(ParticleRandomSite) { return Random(); }
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void FreeBuffer(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual ParticleVisualPoint Normalize(
        ParticleVisualPoint point) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, ParticleVisualPoint point) = 0;
    [[nodiscard]] virtual std::uint32_t PluginOffset() = 0;
};

class IParticleRenderStateRuntime {
public:
    virtual ~IParticleRenderStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetState(
        std::uint32_t state) = 0;
    [[nodiscard]] virtual int SetState(std::uint32_t state,
                                       std::uint32_t value) = 0;
    virtual void RenderAtomic(std::uint32_t atomic) = 0;
};

class IParticleCollectionStateRuntime {
public:
    virtual ~IParticleCollectionStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetElementEnabled(std::uint32_t element,
                                   std::uint8_t enabled) = 0;
};

class IParticleVisualUpdateRuntime {
public:
    virtual ~IParticleVisualUpdateRuntime() = default;
    [[nodiscard]] virtual IParticleSkinRuntime& SkinRuntime() = 0;
    [[nodiscard]] virtual IParticleTankUpdateRuntime& TankUpdateRuntime() = 0;
    [[nodiscard]] virtual IParticleVisualRenderRuntime& VisualRenderRuntime() = 0;
    [[nodiscard]] virtual IParticleEmissionRuntime& EmissionRuntime() = 0;
    [[nodiscard]] virtual IParticleRenderStateRuntime& RenderStateRuntime() = 0;
    virtual void PrepareRender(double value) = 0;
    virtual void RenderCollectionElement(std::uint32_t element,
                                         std::uint32_t event,
                                         ParticleVisualState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ReadExternalWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadExternalByte(
        std::uint32_t address) = 0;
    virtual void WriteExternalWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    virtual void WriteExternalByte(std::uint32_t address,
                                   std::uint8_t value) = 0;
};

class IParticleSimulationRuntime {
public:
    virtual ~IParticleSimulationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual ParticleVisualPoint TransformVector(
        std::uint32_t matrix, ParticleVisualPoint vector) = 0;
    [[nodiscard]] virtual ParticleVisualPoint TransformPosition(
        std::uint32_t matrix, ParticleVisualPoint point) = 0;
    [[nodiscard]] virtual ParticleVisualPoint Normalize(
        ParticleVisualPoint point) = 0;
};

class ParticleVisualObject final {
public:
    // IDA 0x644c30; stack_byte preserves the three native stack-byte writes.
    [[nodiscard]] static ParticleVisualState* Construct(
        ParticleVisualState& state, std::uint8_t stack_byte,
        IParticleVisualRuntime& runtime);
    // IDA 0x644f60/0x644ff0/0x645030/0x6450a0/0x645120.
    [[nodiscard]] static int Destruct(
        ParticleVisualState& state,
        IParticleVisualLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetProperties(
        ParticleVisualState& state, std::uint8_t mode);
    [[nodiscard]] static std::uint8_t Initialize(
        ParticleVisualState& state, std::uint8_t byte52,
        std::uint32_t word48, std::uint32_t word128,
        std::uint32_t word132, std::uint32_t word136,
        std::uint32_t word140, std::uint8_t byte144,
        IParticleVisualLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t InitializeWithFrame(
        ParticleVisualState& state, std::uint8_t byte52,
        std::uint32_t word48, std::uint32_t external_frame,
        std::uint8_t byte144,
        IParticleVisualLifecycleRuntime& runtime);
    // IDA 0x646080.
    [[nodiscard]] static std::uint8_t InitializeRender(
        ParticleVisualState& state, IParticleTankRuntime& runtime);
    // IDA 0x646180.
    [[nodiscard]] static std::uint8_t InitializeCollection(
        ParticleVisualState& state, IParticleCollectionRuntime& runtime);
    // IDA 0x647380.
    [[nodiscard]] static std::uint8_t UpdateFrameTransform(
        ParticleVisualState& state,
        IParticleFrameTransformRuntime& runtime);
    static void Reset(ParticleVisualState& state,
                      IParticleVisualLifecycleRuntime& runtime);
    // IDA 0x646430/0x646470.
    [[nodiscard]] static int DestroyAtomicAndFrame(
        ParticleVisualState& state,
        IParticleVisualLifecycleRuntime& runtime);
    [[nodiscard]] static int DestroyCollectionElements(
        ParticleVisualState& state,
        IParticleVisualLifecycleRuntime& runtime);
    // IDA 0x645480/0x6454e0/0x6454f0.
    [[nodiscard]] static int Update(
        ParticleVisualState& state, double value,
        std::uint32_t argument, bool skip_simulation,
        IParticleVisualUpdateRuntime& runtime);
    [[nodiscard]] static int HandleEvent(
        ParticleVisualState& state, std::uint32_t event,
        IParticleVisualUpdateRuntime& runtime);
    [[nodiscard]] static std::int32_t Deactivate(
        ParticleVisualState& state,
        IParticleVisualUpdateRuntime& runtime);
    // IDA 0x645560.
    [[nodiscard]] static int UpdateSkinAndBones(
        ParticleVisualState& state, IParticleSkinRuntime& runtime);
    // IDA 0x645dc0.
    [[nodiscard]] static std::uint32_t SimulateParticles(
        ParticleVisualState& state, double delta,
        IParticleSimulationRuntime& runtime);
    // IDA 0x647ee0.
    static void UpdatePositionNormalized(
        ParticleVisualState& state, std::uint32_t position,
        double delta, IParticleSimulationRuntime& runtime);
    // IDA 0x645ff0.
    [[nodiscard]] static int DispatchUpdate(
        ParticleVisualState& state, double delta,
        IParticleVisualUpdateRuntime& runtime);
    // IDA 0x646b10.
    [[nodiscard]] static int EmitTankParticles(
        ParticleVisualState& state, double delta,
        IParticleEmissionRuntime& runtime);
    // IDA 0x646ec0.
    [[nodiscard]] static int EmitCollectionParticles(
        ParticleVisualState& state, double delta,
        IParticleEmissionRuntime& runtime);
    // IDA 0x647450.
    static void InitializeDynamicProperties(
        ParticleVisualState& state, std::uint32_t item,
        IParticleEmissionRuntime& runtime);
    // IDA 0x646020/0x646060.
    [[nodiscard]] static int DispatchRender(
        ParticleVisualState& state, std::uint32_t low,
        std::uint32_t high, std::uint32_t argument,
        IParticleVisualUpdateRuntime& runtime);
    // IDA 0x6464f0; the native renderer ignores the two forwarded words.
    [[nodiscard]] static int RenderTank(
        ParticleVisualState& state,
        IParticleTankUpdateRuntime& runtime);
    // IDA 0x646830.
    [[nodiscard]] static int RenderCollection(
        ParticleVisualState& state, std::uint32_t low,
        std::uint32_t high, std::uint32_t argument,
        IParticleVisualRenderRuntime& runtime);
    [[nodiscard]] static int DispatchEvent(
        ParticleVisualState& state, std::uint32_t event,
        IParticleVisualUpdateRuntime& runtime);
    // IDA 0x647260.
    [[nodiscard]] static int RenderState(
        ParticleVisualState& state, IParticleRenderStateRuntime& runtime);
    // IDA 0x648030.
    [[nodiscard]] static std::uint32_t SetCollectionEnabled(
        ParticleVisualState& state, std::uint8_t enabled,
        IParticleCollectionStateRuntime& runtime);

private:
    [[nodiscard]] static bool EmitParticles(
        ParticleVisualState& state, double delta,
        IParticleEmissionRuntime& runtime);
    static void AppendParticle(ParticleVisualState& state,
                               std::uint32_t item,
                               IParticleEmissionRuntime& runtime);
    static void AppendPoint(ParticleVisualState& state,
                            ParticleVisualPoint point,
                            IParticleSkinRuntime& runtime);
};

}  // namespace pal4
