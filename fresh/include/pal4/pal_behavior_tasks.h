#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct PalBehaviorTaskState {
    std::array<std::uint32_t, 17> words{};
};
static_assert(sizeof(PalBehaviorTaskState) == 0x44u);

enum class PalBehaviorKind { ClassZero, ClassTwo, ClassThree, ClassFour };

class IPalBehaviorTaskExecutionRuntime {
public:
    virtual ~IPalBehaviorTaskExecutionRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void ReleaseOwned(std::uint32_t owner, std::uint32_t object) = 0;
    virtual void UpdateOwned(std::uint32_t owner, std::uint32_t object) = 0;
};

class IPalBehaviorTaskRuntime : public IPalBehaviorTaskExecutionRuntime {
public:
    virtual ~IPalBehaviorTaskRuntime() = default;
    virtual void ConstructBase(PalBehaviorTaskState& state) = 0;
    virtual void DestructBase(PalBehaviorTaskState& state) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Bind(std::uint32_t binder,
                                              std::uint32_t& type_id) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructVectorObject(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructExtendedObject(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructSmallObject(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructRandomObject(
        std::uint32_t address) = 0;
    virtual void OnCreated(std::uint32_t owner, std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t IsOwnedComplete(
        std::uint32_t owner, std::uint32_t object) = 0;
    [[nodiscard]] virtual PalBehaviorTaskState& Access(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsDurationComplete(
        PalBehaviorTaskState& state) = 0;
};

class PalBehaviorTaskRuntimeStub final : public IPalBehaviorTaskRuntime {
public:
    void ConstructBase(PalBehaviorTaskState& state) override;
    void DestructBase(PalBehaviorTaskState& state) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void ReleaseOwned(std::uint32_t owner, std::uint32_t object) override;
    void Free(std::uint32_t address) override;
    std::uint32_t Bind(std::uint32_t binder,
                       std::uint32_t& type_id) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ConstructVectorObject(std::uint32_t address) override;
    std::uint32_t ConstructExtendedObject(std::uint32_t address) override;
    std::uint32_t ConstructSmallObject(std::uint32_t address) override;
    std::uint32_t ConstructRandomObject(std::uint32_t address) override;
    void OnCreated(std::uint32_t owner, std::uint32_t object) override;
    void UpdateOwned(std::uint32_t owner, std::uint32_t object) override;
    std::uint32_t IsOwnedComplete(std::uint32_t owner,
                                  std::uint32_t object) override;
    PalBehaviorTaskState& Access(std::uint32_t address) override;
    bool IsDurationComplete(PalBehaviorTaskState& state) override;

private:
    PalBehaviorTaskState state_{};
};

class PalBehaviorTaskBase final {
public:
    static constexpr std::uint32_t kVtable = 0x8427f4u;

    static void Destruct(PalBehaviorTaskState& state,
                         IPalBehaviorTaskRuntime& runtime);
    [[nodiscard]] static bool Finish(PalBehaviorTaskState& state,
                                     IPalBehaviorTaskExecutionRuntime& runtime);
    [[nodiscard]] static bool Run(PalBehaviorTaskState& state,
                                  IPalBehaviorTaskExecutionRuntime& runtime);
    [[nodiscard]] static bool IsDurationComplete(
        PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime);
};

template <PalBehaviorKind Kind>
class PalBehaviorClass final {
public:
    static constexpr std::uint32_t kVtable =
        Kind == PalBehaviorKind::ClassZero ? 0x842814u :
        Kind == PalBehaviorKind::ClassTwo ? 0x842854u :
        Kind == PalBehaviorKind::ClassThree ? 0x842874u : 0x842834u;
    static constexpr std::uint32_t kTypeId =
        Kind == PalBehaviorKind::ClassZero ? 0u :
        Kind == PalBehaviorKind::ClassTwo ? 2u :
        Kind == PalBehaviorKind::ClassThree ? 3u : 4u;
    static constexpr std::uint32_t kOwnedBytes =
        Kind == PalBehaviorKind::ClassTwo ? 0xbcu :
        Kind == PalBehaviorKind::ClassFour ? 0xc0u : 0xe0u;

    [[nodiscard]] static PalBehaviorTaskState& Construct(
        PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        PalBehaviorTaskState& state, std::uint32_t address,
        std::uint32_t flags, IPalBehaviorTaskRuntime& runtime);
    static void Destruct(PalBehaviorTaskState& state,
                         IPalBehaviorTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t BindOwner(
        PalBehaviorTaskState& state, std::uint32_t binder,
        IPalBehaviorTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateOwned(
        PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IPalBehaviorTaskRuntime& runtime);
    [[nodiscard]] static bool IsComplete(
        PalBehaviorTaskState& state, IPalBehaviorTaskRuntime& runtime);
};

using PalBehaviorClassZero = PalBehaviorClass<PalBehaviorKind::ClassZero>;
using PalBehaviorClassTwo = PalBehaviorClass<PalBehaviorKind::ClassTwo>;
using PalBehaviorClassThree = PalBehaviorClass<PalBehaviorKind::ClassThree>;
using PalBehaviorClassFour = PalBehaviorClass<PalBehaviorKind::ClassFour>;

}  // namespace pal4
