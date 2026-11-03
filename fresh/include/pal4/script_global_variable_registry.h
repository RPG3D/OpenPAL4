#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ScriptGlobalVariableDescriptor {
    std::uint32_t description{};
    std::uint32_t description_size{};
    std::uint32_t signature{};
    std::uint32_t signature_size{};
    std::array<std::uint32_t, 8> reserved{};
    std::uint32_t variable_address{};
    std::uint32_t trailing_word{};
};
static_assert(sizeof(ScriptGlobalVariableDescriptor) == 56u);
static_assert(offsetof(ScriptGlobalVariableDescriptor, variable_address) == 0x30u);

enum class ScriptNamedVariable : std::uint8_t {
    Shu, Xia, Wu, Feng, Zi, Mo, Ming, Yan, Cha, Star,
    NpcA, NpcB, NpcC, NpcD, NpcE, NpcF, NpcG, Zhu,
    Shop031, Shop010, Shop032, Shop021, Shop012, Shop034,
    Shop037, Shop004, Shop024, Shop025, Shop005, Shop035,
    Shop026, Shop006, Shop027, Shop007, Shop029, Shop009, Shop030
};

class IScriptGlobalVariableRuntime {
public:
    virtual ~IScriptGlobalVariableRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size,
                                                  std::uint32_t allocation_type,
                                                  std::uint32_t protection) = 0;
    virtual void CopyString(std::uint32_t destination,
                            std::string_view source_with_nul) = 0;
    virtual void Release(std::uint32_t address, std::uint32_t size,
                         std::uint32_t free_type) = 0;
    virtual void RegisterVariable(
        std::uint32_t descriptor_address,
        const ScriptGlobalVariableDescriptor& descriptor) = 0;
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback_address) = 0;
};

class ScriptGlobalVariableRuntimeStub : public IScriptGlobalVariableRuntime {
public:
    std::uint32_t Allocate(std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
    void CopyString(std::uint32_t, std::string_view) override;
    void Release(std::uint32_t, std::uint32_t, std::uint32_t) override;
    void RegisterVariable(std::uint32_t,
                          const ScriptGlobalVariableDescriptor&) override;
    int RegisterAtExit(std::uint32_t) override;
};

class ScriptGlobalVariableRegistry final {
public:
    static constexpr std::uint32_t kGlobalDescriptor = 0x8e7230u;
    static constexpr std::uint32_t kMazeOneDescriptor = 0x8e71f8u;
    static constexpr std::uint32_t kMazeTwoDescriptor = 0x8e71c0u;
    static constexpr std::uint32_t kGlobalVariable = 0x90dec0u;
    static constexpr std::uint32_t kMazeOneVariable = 0x90dec4u;
    static constexpr std::uint32_t kMazeTwoVariable = 0x90dec8u;
    static constexpr std::uint32_t kGlobalCleanup = 0x45d270u;
    static constexpr std::uint32_t kMazeOneCleanup = 0x45d3b0u;
    static constexpr std::uint32_t kMazeTwoCleanup = 0x45d4f0u;
    static constexpr std::array<std::uint32_t, 8> kAdditionalMazeDescriptors{
        0x8e7188u, 0x8e7150u, 0x8e7118u, 0x8e70e0u,
        0x8e70a8u, 0x8e7070u, 0x8e7038u, 0x8e7000u};
    static constexpr std::array<std::uint32_t, 8> kAdditionalMazeVariables{
        0x90deccu, 0x90ded0u, 0x90ded4u, 0x90ded8u,
        0x90dedcu, 0x90dee0u, 0x90dee4u, 0x90dee8u};
    static constexpr std::array<std::uint32_t, 8> kAdditionalMazeCleanups{
        0x45d630u, 0x45d770u, 0x45d8b0u, 0x45d9f0u,
        0x45db30u, 0x45dc70u, 0x45ddb0u, 0x45def0u};
    static constexpr std::array<std::uint32_t, 37> kNamedDescriptors{
        0x8e6fc8u, 0x8e6f90u, 0x8e6f58u, 0x8e6f20u,
        0x8e6ee8u, 0x8e6eb0u, 0x8e6e78u, 0x8e6e40u,
        0x8e6e08u, 0x8e6dd0u, 0x8e6d98u, 0x8e6d60u,
        0x8e6d28u, 0x8e6cf0u, 0x8e6cb8u, 0x8e6c80u,
        0x8e6c48u, 0x8e6c10u, 0x8e6bd8u, 0x8e6ba0u,
        0x8e6b68u, 0x8e6b30u, 0x8e6af8u, 0x8e6ac0u,
        0x8e6a88u, 0x8e6a50u, 0x8e6a18u, 0x8e69e0u,
        0x8e69a8u, 0x8e6970u, 0x8e6938u, 0x8e6900u,
        0x8e68c8u, 0x8e6890u, 0x8e6858u, 0x8e6820u,
        0x8e67e8u};
    static constexpr std::array<std::uint32_t, 37> kNamedVariables{
        0x90deecu, 0x90def0u, 0x90def4u, 0x90def8u,
        0x90defcu, 0x90df00u, 0x90df04u, 0x90df08u,
        0x90df0cu, 0x90df10u, 0x90df14u, 0x90df18u,
        0x90df1cu, 0x90df20u, 0x90df24u, 0x90df28u,
        0x90df2cu, 0x90df30u, 0x90df34u, 0x90df38u,
        0x90df3cu, 0x90df40u, 0x90df44u, 0x90df48u,
        0x90df4cu, 0x90df50u, 0x90df54u, 0x90df58u,
        0x90df5cu, 0x90df60u, 0x90df64u, 0x90df68u,
        0x90df6cu, 0x90df70u, 0x90df74u, 0x90df78u,
        0x90df7cu};
    static constexpr std::array<std::uint32_t, 37> kNamedCleanups{
        0x45e030u, 0x45e170u, 0x45e2b0u, 0x45e3f0u,
        0x45e530u, 0x45e670u, 0x45e7b0u, 0x45e8f0u,
        0x45ea30u, 0x45eb70u, 0x45ecb0u, 0x45edf0u,
        0x45ef30u, 0x45f070u, 0x45f1b0u, 0x45f2f0u,
        0x45f430u, 0x45f570u, 0x45f6b0u, 0x45f7f0u,
        0x45f930u, 0x45fa70u, 0x45fbb0u, 0x45fcf0u,
        0x45fe30u, 0x45ff70u, 0x4600b0u, 0x4601f0u,
        0x460330u, 0x460470u, 0x4605b0u, 0x4606f0u,
        0x460830u, 0x460970u, 0x460ab0u, 0x460bf0u,
        0x460d30u};

    // Exact existing signature metadata without its descriptor terminator.
    static std::string_view NamedSignature(ScriptNamedVariable variable);

    explicit ScriptGlobalVariableRegistry(IScriptGlobalVariableRuntime& runtime)
        : runtime_(runtime) {}

    [[nodiscard]] int InitializeGlobal();
    void RegisterGlobal();
    void CleanupGlobal();
    [[nodiscard]] int InitializeMazeOne();
    void RegisterMazeOne();
    void CleanupMazeOne();
    [[nodiscard]] int InitializeMazeTwo();
    void RegisterMazeTwo();
    void CleanupMazeTwo();
    [[nodiscard]] int InitializeMazeNumber(std::uint32_t number);
    void RegisterMazeNumber(std::uint32_t number);
    void CleanupMazeNumber(std::uint32_t number);
    [[nodiscard]] int InitializeNamed(ScriptNamedVariable variable);
    void RegisterNamed(ScriptNamedVariable variable);
    void CleanupNamed(ScriptNamedVariable variable);

    [[nodiscard]] const ScriptGlobalVariableDescriptor& Global() const noexcept {
        return global_;
    }
    [[nodiscard]] const ScriptGlobalVariableDescriptor& MazeOne() const noexcept {
        return maze_one_;
    }
    [[nodiscard]] const ScriptGlobalVariableDescriptor& MazeTwo() const noexcept {
        return maze_two_;
    }
    [[nodiscard]] const ScriptGlobalVariableDescriptor& MazeNumber(
        std::uint32_t number) const;
    [[nodiscard]] const ScriptGlobalVariableDescriptor& Named(
        ScriptNamedVariable variable) const;

private:
    void Register(ScriptGlobalVariableDescriptor& descriptor,
                  std::uint32_t descriptor_address,
                  std::string_view description, std::string_view signature,
                  std::uint32_t variable_address);
    void Cleanup(ScriptGlobalVariableDescriptor& descriptor);

    IScriptGlobalVariableRuntime& runtime_;
    ScriptGlobalVariableDescriptor global_{};
    ScriptGlobalVariableDescriptor maze_one_{};
    ScriptGlobalVariableDescriptor maze_two_{};
    std::array<ScriptGlobalVariableDescriptor, 8> additional_maze_{};
    std::array<ScriptGlobalVariableDescriptor, 37> named_{};
};

}  // namespace pal4
