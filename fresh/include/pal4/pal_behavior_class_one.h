#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct PalBehaviorClassOneState {
    std::array<std::uint32_t, 17> words{};
};
static_assert(sizeof(PalBehaviorClassOneState) == 0x44u);

class IPalBehaviorClassOneRuntime {
public:
    virtual ~IPalBehaviorClassOneRuntime() = default;
    virtual void ConstructBase(PalBehaviorClassOneState& state) = 0;
    virtual void DestructBase(PalBehaviorClassOneState& state) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void ReleaseOwned(std::uint32_t owner,
                              std::uint32_t object) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Bind(
        std::uint32_t binder, std::uint32_t& initial_value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeVectorObject(
        std::uint32_t address) = 0;
    virtual void OnCreated(std::uint32_t owner,
                           std::uint32_t object) = 0;
    [[nodiscard]] virtual PalBehaviorClassOneState& Access(
        std::uint32_t address) = 0;
};

class PalBehaviorClassOneRuntimeStub final
    : public IPalBehaviorClassOneRuntime {
public:
    void ConstructBase(PalBehaviorClassOneState& state) override;
    void DestructBase(PalBehaviorClassOneState& state) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void ReleaseOwned(std::uint32_t owner, std::uint32_t object) override;
    void Free(std::uint32_t address) override;
    std::uint32_t Bind(std::uint32_t binder,
                       std::uint32_t& initial_value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t InitializeVectorObject(std::uint32_t address) override;
    void OnCreated(std::uint32_t owner, std::uint32_t object) override;
    PalBehaviorClassOneState& Access(std::uint32_t address) override;

private:
    PalBehaviorClassOneState state_{};
};

class PalBehaviorClassOne final {
public:
    static constexpr std::uint32_t kVtable = 0x8427d4u;
    static constexpr std::uint32_t kBaseVtable = 0x8427f4u;

    [[nodiscard]] static PalBehaviorClassOneState& Construct(
        PalBehaviorClassOneState& state,
        IPalBehaviorClassOneRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        PalBehaviorClassOneState& state, std::uint32_t address,
        std::uint32_t flags, IPalBehaviorClassOneRuntime& runtime);
    static void Destruct(PalBehaviorClassOneState& state,
                         IPalBehaviorClassOneRuntime& runtime);
    [[nodiscard]] static std::uint32_t BindOwner(
        PalBehaviorClassOneState& state, std::uint32_t binder,
        IPalBehaviorClassOneRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateOwned(
        PalBehaviorClassOneState& state,
        IPalBehaviorClassOneRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IPalBehaviorClassOneRuntime& runtime);
};

}  // namespace pal4
