#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PalActorControlState {
    std::array<std::uint32_t, 281> words{};
};
static_assert(sizeof(PalActorControlState) == 0x464u);

class IPalActorControlRuntime {
public:
    virtual ~IPalActorControlRuntime() = default;
    virtual void ConstructGameComponent(std::uint32_t* storage) = 0;
    virtual void ConstructDatabase(std::uint32_t* storage) = 0;
    virtual void ConstructMotion(std::uint32_t* storage) = 0;
    virtual void ConstructDNameNode(std::uint32_t* storage) = 0;
    virtual void ConstructNamedNode(std::uint32_t* storage,
                                    std::string_view name) = 0;
    virtual void DestructDatabase(std::uint32_t* storage) = 0;
    virtual void DestructDNameNode(std::uint32_t* storage) = 0;
    virtual void ResetMemberVtable(std::uint32_t* storage) = 0;
    virtual void DestructNamedNode(std::uint32_t* storage) = 0;
    virtual void SetNamedNodeErrorFlag(std::uint32_t* field,
                                       std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t GameConfig(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t Logger() = 0;
    [[nodiscard]] virtual std::uint32_t Log(
        std::uint32_t logger, std::string_view format,
        std::string_view source, std::uint32_t line) = 0;
    virtual void PrepareScripts() = 0;
    virtual void CleanupScripts() = 0;
    virtual void InitializeComponent(std::uint32_t* storage) = 0;
    virtual void InitializeDatabase(std::uint32_t* storage) = 0;
    virtual void InitializeMotion(std::uint32_t* storage) = 0;
    virtual void ShutdownComponent(std::uint32_t* storage) = 0;
    virtual void ShutdownDatabase(std::uint32_t* storage,
                                  std::uint32_t event_context) = 0;
    virtual void ShutdownMotion(std::uint32_t* storage) = 0;
    [[nodiscard]] virtual std::uint8_t ResetController(
        PalActorControlState& state) = 0;
    virtual void UpdateComponent(std::uint32_t* storage) = 0;
    virtual void UpdateDatabase(std::uint32_t* storage) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateMotion(
        std::uint32_t* storage) = 0;
    virtual void ProcessComponentInput(std::uint32_t* storage,
                                       std::uint32_t actor) = 0;
    virtual void ProcessDatabaseInput(std::uint32_t* storage,
                                      std::uint32_t actor,
                                      std::uint32_t event_context) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessMotionInput(
        std::uint32_t* storage, std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SetActorAnimationFlag(
        std::uint32_t actor, std::uint32_t value) = 0;
    virtual void ProcessComponentInputWithUi(std::uint32_t* storage,
                                             std::uint32_t actor) = 0;
    virtual void ProcessDatabaseInputWithUi(std::uint32_t* storage,
                                            std::uint32_t actor,
                                            std::uint32_t event_context) = 0;
    virtual void ProcessMotionInputWithUi(std::uint32_t* storage,
                                          std::uint32_t actor) = 0;
};

class PalActorControlRuntimeStub : public IPalActorControlRuntime {
public:
    void ConstructGameComponent(std::uint32_t*) override;
    void ConstructDatabase(std::uint32_t*) override;
    void ConstructMotion(std::uint32_t*) override;
    void ConstructDNameNode(std::uint32_t*) override;
    void ConstructNamedNode(std::uint32_t*, std::string_view) override;
    void DestructDatabase(std::uint32_t*) override;
    void DestructDNameNode(std::uint32_t*) override;
    void ResetMemberVtable(std::uint32_t*) override;
    void DestructNamedNode(std::uint32_t*) override;
    void SetNamedNodeErrorFlag(std::uint32_t*, std::uint8_t) override;
    std::int32_t GameConfig(std::uint32_t) override;
    std::uint32_t Logger() override;
    std::uint32_t Log(std::uint32_t, std::string_view,
                      std::string_view, std::uint32_t) override;
    void PrepareScripts() override;
    void CleanupScripts() override;
    void InitializeComponent(std::uint32_t*) override;
    void InitializeDatabase(std::uint32_t*) override;
    void InitializeMotion(std::uint32_t*) override;
    void ShutdownComponent(std::uint32_t*) override;
    void ShutdownDatabase(std::uint32_t*, std::uint32_t) override;
    void ShutdownMotion(std::uint32_t*) override;
    std::uint8_t ResetController(PalActorControlState&) override;
    void UpdateComponent(std::uint32_t*) override;
    void UpdateDatabase(std::uint32_t*) override;
    std::uint32_t UpdateMotion(std::uint32_t*) override;
    void ProcessComponentInput(std::uint32_t*, std::uint32_t) override;
    void ProcessDatabaseInput(std::uint32_t*, std::uint32_t,
                              std::uint32_t) override;
    std::uint32_t ProcessMotionInput(std::uint32_t*,
                                     std::uint32_t) override;
    std::uint32_t SetActorAnimationFlag(std::uint32_t,
                                        std::uint32_t) override;
    void ProcessComponentInputWithUi(std::uint32_t*, std::uint32_t) override;
    void ProcessDatabaseInputWithUi(std::uint32_t*, std::uint32_t,
                                    std::uint32_t) override;
    void ProcessMotionInputWithUi(std::uint32_t*, std::uint32_t) override;
};

class PalActorControl final {
public:
    static constexpr std::uint32_t kFirstMemberVtable = 0x842660u;
    static constexpr std::uint32_t kSecondMemberVtable = 0x842654u;
    static constexpr std::uint32_t kFloorSoundCallback = 0x425300u;

    [[nodiscard]] static std::uint32_t SetControlWord(
        PalActorControlState& state, std::uint32_t value);
    [[nodiscard]] static std::uint32_t GetControlWord(
        const PalActorControlState& state);
    [[nodiscard]] static std::uint32_t GetMode(
        const PalActorControlState& state);

    [[nodiscard]] static PalActorControlState& Construct(
        PalActorControlState& state, IPalActorControlRuntime& runtime);
    static void Destruct(PalActorControlState& state,
                         IPalActorControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        PalActorControlState& state, std::uint32_t base_address,
        IPalActorControlRuntime& runtime);
    [[nodiscard]] static std::uint8_t Shutdown(
        PalActorControlState& state, std::uint32_t event_context,
        IPalActorControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateComponents(
        PalActorControlState& state, IPalActorControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessInput(
        PalActorControlState& state, std::uint32_t event_context,
        IPalActorControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessInputAndClearVector(
        PalActorControlState& state, std::uint32_t event_context,
        IPalActorControlRuntime& runtime);
};

}  // namespace pal4
