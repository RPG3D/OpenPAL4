#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

struct ActorControlPlayerPrefix;
class IActorControlUpdateRuntime;
class IActorControlDisplayRuntime;
class IActorControlDisplayResetRuntime;

struct ActorControlDatabasePrefix {
    std::array<std::uint32_t, 145> words{};
};
static_assert(sizeof(ActorControlDatabasePrefix) == 580u);

struct ActorControlPathBuffer {
    std::uint32_t data{};
    std::uint32_t length{};
    std::uint32_t capacity{};
};
static_assert(sizeof(ActorControlPathBuffer) == 12u);

class IActorControlDatabaseRuntime {
public:
    virtual ~IActorControlDatabaseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FreeAddress(void* address) = 0;
    virtual void ConstructResourceManager(std::uint32_t* storage) = 0;
    virtual void ConstructNameNode(std::uint32_t* storage) = 0;
    virtual void DestructNameNode(std::uint32_t* storage) = 0;
    virtual void DestructResourceManager(std::uint32_t* storage) = 0;
    virtual void ConcatenatePath(ActorControlPathBuffer& output,
                                 std::uint32_t first,
                                 std::uint32_t second) = 0;
    virtual void AppendPath(ActorControlPathBuffer& path,
                            std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void LoadClump(std::uint32_t* resource_manager,
                           std::string_view name, std::uint32_t path,
                           std::string_view model,
                           std::uint32_t palette, std::uint32_t first,
                           std::uint32_t second,
                           std::uint32_t enabled) = 0;
    virtual void ReleasePath(ActorControlPathBuffer& path) = 0;
    [[nodiscard]] virtual std::uint32_t HAnimFrameId(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t GameArenaManager() = 0;
    virtual void RemoveClump(std::uint32_t arena,
                             std::uint32_t clump) = 0;
    virtual void CleanupClump(std::uint32_t* resource_manager,
                              std::uint32_t palette) = 0;
    [[nodiscard]] virtual bool NameNodeNeedsUpdate(
        std::uint32_t* name_node) = 0;
    virtual void HandleNameEvent(std::uint32_t* name_node,
                                 std::uint32_t context,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void ProcessUiEvents(std::uint32_t manager,
                                 std::uint32_t mode,
                                 std::array<std::uint32_t, 2>& scratch) = 0;
    [[nodiscard]] virtual std::uint32_t CreateProgressBar(
        std::uint32_t manager) = 0;
    virtual void SetProgressBarVisible(std::uint32_t progress_bar,
                                       bool visible) = 0;
};

class ActorControlDatabaseRuntimeStub : public IActorControlDatabaseRuntime {
public:
    [[nodiscard]] std::uint32_t FreeAddress(void* address) override;
    void ConstructResourceManager(std::uint32_t* storage) override;
    void ConstructNameNode(std::uint32_t* storage) override;
    void DestructNameNode(std::uint32_t* storage) override;
    void DestructResourceManager(std::uint32_t* storage) override;
    void ConcatenatePath(ActorControlPathBuffer& output,
                         std::uint32_t first,
                         std::uint32_t second) override;
    void AppendPath(ActorControlPathBuffer& path,
                    std::string_view suffix) override;
    [[nodiscard]] std::uint32_t GameInstance() override;
    void LoadClump(std::uint32_t* resource_manager,
                   std::string_view name, std::uint32_t path,
                   std::string_view model, std::uint32_t palette,
                   std::uint32_t first, std::uint32_t second,
                   std::uint32_t enabled) override;
    void ReleasePath(ActorControlPathBuffer& path) override;
    [[nodiscard]] std::uint32_t HAnimFrameId(
        std::uint32_t clump) override;
    [[nodiscard]] std::uint32_t GameArenaManager() override;
    void RemoveClump(std::uint32_t arena,
                     std::uint32_t clump) override;
    void CleanupClump(std::uint32_t* resource_manager,
                      std::uint32_t palette) override;
    [[nodiscard]] bool NameNodeNeedsUpdate(
        std::uint32_t* name_node) override;
    void HandleNameEvent(std::uint32_t* name_node,
                         std::uint32_t context,
                         std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t UiFrameManager() override;
    void ProcessUiEvents(std::uint32_t manager,
                         std::uint32_t mode,
                         std::array<std::uint32_t, 2>& scratch) override;
    [[nodiscard]] std::uint32_t CreateProgressBar(
        std::uint32_t manager) override;
    void SetProgressBarVisible(std::uint32_t progress_bar,
                               bool visible) override;
};

class ActorControlBase final {
public:
    static constexpr std::uint32_t kVtable = 0x8425d4u;

    static void Construct(std::uint32_t* object);
    static void RestoreVtable(std::uint32_t* object);
    [[nodiscard]] static std::uint32_t* Delete(
        std::uint32_t* object, std::uint32_t flags,
        IActorControlDatabaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMember(
        std::uint32_t* object, std::uint32_t value);
};

class ActorControlDatabase final {
public:
    static constexpr std::uint32_t kVtable = 0x8425fcu;
    static constexpr std::uint32_t kPathFirstGlobal = 0x94d560u;
    static constexpr std::uint32_t kPathSecondGlobal = 0x94d53cu;

    [[nodiscard]] static ActorControlDatabasePrefix& Construct(
        ActorControlDatabasePrefix& state,
        IActorControlDatabaseRuntime& runtime);
    static void Destruct(ActorControlDatabasePrefix& state,
                         IActorControlDatabaseRuntime& runtime);
    [[nodiscard]] static ActorControlDatabasePrefix* Destroy(
        ActorControlDatabasePrefix* state, std::uint32_t flags,
        IActorControlDatabaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadDummyClump(
        ActorControlDatabasePrefix& state,
        IActorControlDatabaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t UnloadClump(
        ActorControlDatabasePrefix& state, std::uint32_t event_context,
        IActorControlDatabaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetDialog(
        ActorControlDatabasePrefix& state, std::uint32_t event_context,
        std::uint32_t value, IActorControlDatabaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetWithUi(
        ActorControlDatabasePrefix& state, std::uint32_t event_context,
        std::uint32_t value, IActorControlDatabaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        ActorControlDatabasePrefix& state, ActorControlPlayerPrefix& player,
        float delta_seconds, IActorControlUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetState28(
        const ActorControlDatabasePrefix& state);
    [[nodiscard]] static std::uint8_t UpdateDisplay(
        ActorControlDatabasePrefix& state, std::uint32_t value,
        IActorControlDisplayRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdateDisplay(
        std::span<std::uint32_t, 145> words, std::uint32_t value,
        IActorControlDisplayRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetDisplayState(
        const ActorControlDatabasePrefix& state);
    static void ResetPlayer(ActorControlDatabasePrefix& state,
                            ActorControlPlayerPrefix* player,
                            std::uint32_t event_context,
                            IActorControlUpdateRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResetAndUpdateDisplay(
        ActorControlDatabasePrefix& state,
        IActorControlDisplayResetRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResetAndUpdateDisplay(
        std::span<std::uint32_t, 145> words,
        IActorControlDisplayResetRuntime& runtime);
};

}  // namespace pal4
