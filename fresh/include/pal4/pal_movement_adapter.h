#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PalMovementAdapterState {
    std::array<std::uint32_t, 5> words{};
};
static_assert(sizeof(PalMovementAdapterState) == 0x14u);

struct PalMovementAdapterGlobalState {
    std::uint8_t initialized = 0u;
    std::uint32_t instance = 0u;
};

struct PalMovementProbeScratch {
    std::uint32_t first{};
    float score{};
    std::uint32_t third{};
};
static_assert(sizeof(PalMovementProbeScratch) == 12u);

class IPalMovementAdapterRuntime {
public:
    virtual ~IPalMovementAdapterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual PalMovementAdapterState& StateAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructPrimary(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructSecondary(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) = 0;
    virtual void NoopDestroy(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PreDestroy(
        std::uint32_t address, PalMovementAdapterState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteChild(
        std::uint32_t address, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchPrimary(
        std::uint32_t address,
        const std::array<std::uint32_t, 7>& arguments) = 0;
    [[nodiscard]] virtual std::uint32_t ProbeChild(
        std::uint32_t child, std::uint32_t fixed_first,
        std::uint32_t fixed_second, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        PalMovementProbeScratch& scratch) = 0;
    [[nodiscard]] virtual std::uint32_t CanSwitch(
        std::uint32_t child, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t fourth) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateChild(
        std::uint32_t child, std::uint32_t first,
        std::uint32_t second, std::uint32_t third) = 0;
    virtual void DeactivateChild(std::uint32_t child) = 0;
    virtual void ActivateChild(std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t ProbeActive(
        std::uint32_t child, std::uint32_t fixed_first,
        std::uint32_t fixed_second,
        const std::array<std::uint32_t, 4>& arguments) = 0;
};

class PalMovementAdapterRuntimeStub : public IPalMovementAdapterRuntime {
public:
    std::uint32_t Allocate(std::uint32_t size) override;
    PalMovementAdapterState& StateAt(std::uint32_t address) override;
    std::uint32_t ConstructPrimary(std::uint32_t address) override;
    std::uint32_t ConstructSecondary(std::uint32_t address) override;
    std::uint32_t SingletonManager() override;
    std::uint32_t RegisterSingleton(std::uint32_t manager,
                                    std::string_view name, std::uint32_t id,
                                    std::uint32_t factory,
                                    std::uint32_t destroy) override;
    void NoopDestroy(std::uint32_t address) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint32_t PreDestroy(std::uint32_t address,
                             PalMovementAdapterState& state) override;
    std::uint32_t DeleteChild(std::uint32_t address,
                              std::uint32_t flags) override;
    std::uint32_t DispatchPrimary(
        std::uint32_t address,
        const std::array<std::uint32_t, 7>& arguments) override;
    std::uint32_t ProbeChild(std::uint32_t child,
                             std::uint32_t fixed_first,
                             std::uint32_t fixed_second,
                             std::uint32_t first, std::uint32_t second,
                             std::uint32_t third,
                             PalMovementProbeScratch& scratch) override;
    std::uint32_t CanSwitch(std::uint32_t child, std::uint32_t first,
                            std::uint32_t second, std::uint32_t third,
                            std::uint32_t fourth) override;
    std::uint32_t UpdateChild(std::uint32_t child,
                              std::uint32_t first, std::uint32_t second,
                              std::uint32_t third) override;
    void DeactivateChild(std::uint32_t child) override;
    void ActivateChild(std::uint32_t child) override;
    std::uint32_t ProbeActive(
        std::uint32_t child, std::uint32_t fixed_first,
        std::uint32_t fixed_second,
        const std::array<std::uint32_t, 4>& arguments) override;

private:
    PalMovementAdapterState empty_state_{};
};

class PalMovementAdapter final {
public:
    static constexpr std::uint32_t kSingletonId = 1017u;
    static constexpr std::uint32_t kFactoryAddress = 0x428ba0u;
    static constexpr std::uint32_t kDestroyAddress = 0x428c00u;
    static constexpr std::string_view kSingletonName = "palMovmentAdapter";

    [[nodiscard]] static std::uint32_t Initialize(
        PalMovementAdapterGlobalState& globals,
        IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static PalMovementAdapterState& Construct(
        PalMovementAdapterState& state,
        IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t address, std::uint32_t undefined_return,
        IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        PalMovementAdapterState& state, std::uint32_t undefined_return,
        IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardPrimary(
        const PalMovementAdapterState& state,
        std::array<std::uint32_t, 7> arguments,
        IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectAndUpdate(
        PalMovementAdapterState& state, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t fourth, IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static bool CheckActive(
        const PalMovementAdapterState& state,
        const std::array<std::uint32_t, 4>& arguments,
        IPalMovementAdapterRuntime& runtime);
    [[nodiscard]] static bool AnyChildMatches(
        const PalMovementAdapterState& state,
        const std::array<std::uint32_t, 4>& arguments,
        IPalMovementAdapterRuntime& runtime);
};

}  // namespace pal4
