#pragma once

#include "pal4/actor_control_dummy_clump.h"
#include "pal4/actor_control_database.h"
#include "pal4/actor_control_input_component.h"
#include "pal4/actor_control_named_node.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/pal_actor_control.h"
#include "pal4/rw_dict_schema.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace pal4 {
class NativePkFileManagerRuntime;

// Owns the original x86 ActorControl image. Render and script services are
// intentionally separate from its construction and destruction lifetime.
class NativeActorControlSession final {
public:
    NativeActorControlSession(NativeBaseObjectMemory& memory,
                              IRwDictSchemaRuntime& schema,
                              std::array<std::int32_t, 2> game_config,
                              NativePkFileManagerRuntime* packages = nullptr,
                              IActorControlDummyClump* dummy = nullptr,
                              std::filesystem::path game_root = {},
                              std::uint32_t game_address = 0u);
    NativeActorControlSession(const NativeActorControlSession&) = delete;
    NativeActorControlSession& operator=(const NativeActorControlSession&) = delete;
    ~NativeActorControlSession();

    [[nodiscard]] std::uint32_t Construct();
    void Destroy();
    [[nodiscard]] bool Constructed() const noexcept { return constructed_; }
    [[nodiscard]] PalActorControlState& State() noexcept { return state_; }
    [[nodiscard]] IPalActorControlRuntime& Runtime() noexcept { return runtime_; }
    [[nodiscard]] std::uint32_t Address() noexcept;
    void BindActorSubobject(std::uint32_t* storage, std::uint32_t actor,
                            std::uint32_t virtual_slot,
                            IActorControlInputComponentRuntime& input_runtime);

private:
    class DatabaseRuntime final : public ActorControlDatabaseRuntimeStub {
    public:
        DatabaseRuntime(NativeBaseObjectMemory& memory,
                        IRwDictSchemaRuntime& schema,
                        IActorControlDummyClump* dummy,
                        const std::filesystem::path& game_root,
                        std::uint32_t game_address)
            : memory_(memory), schema_(schema), dummy_(dummy),
              game_root_(game_root), game_address_(game_address) {}
        void ConstructResourceManager(std::uint32_t* storage) override;
        void ConstructNameNode(std::uint32_t* storage) override;
        void DestructNameNode(std::uint32_t* storage) override;
        void DestructResourceManager(std::uint32_t* storage) override;
        void ConcatenatePath(ActorControlPathBuffer& output,
                             std::uint32_t first,
                             std::uint32_t second) override;
        void AppendPath(ActorControlPathBuffer& path,
                        std::string_view suffix) override;
        std::uint32_t GameInstance() override;
        void LoadClump(std::uint32_t* resource_manager,
                       std::string_view name, std::uint32_t path,
                       std::string_view model, std::uint32_t palette,
                       std::uint32_t first, std::uint32_t second,
                       std::uint32_t enabled) override;
        void ReleasePath(ActorControlPathBuffer& path) override;
        std::uint32_t HAnimFrameId(std::uint32_t clump) override;
        void CleanupClump(std::uint32_t* resource_manager,
                          std::uint32_t palette) override;
        bool NameNodeNeedsUpdate(std::uint32_t* name_node) override;
        [[nodiscard]] bool DummyBound() const noexcept {
            return dummy_ != nullptr;
        }
    private:
        NativeBaseObjectMemory& memory_;
        IRwDictSchemaRuntime& schema_;
        IActorControlDummyClump* dummy_;
        const std::filesystem::path& game_root_;
        std::uint32_t game_address_;
        std::string path_;
    };

    class NamedRuntime final : public ActorControlNamedNodeRuntimeStub {
    public:
        explicit NamedRuntime(NativeBaseObjectMemory& memory)
            : memory_(memory) {}
        std::uint8_t ReadByte(std::uint32_t address) override;
        std::uint32_t ReadDword(std::uint32_t address) override;
        void WriteByte(std::uint32_t address, std::uint8_t value) override;
        void WriteDword(std::uint32_t address, std::uint32_t value) override;
        std::uint32_t TextLength(std::uint32_t address) override;
        void SetErrorFlag(std::uint32_t record, std::uint8_t value) override;
        void ReallocateText(std::uint32_t record,
                            std::uint32_t length) override;
        void CopyBytes(std::uint32_t destination, std::uint32_t source,
                       std::uint32_t length) override;
        std::uint32_t FreeAddress(std::uint32_t address) override;
    private:
        NativeBaseObjectMemory& memory_;
    };

    class RuntimeImpl final : public PalActorControlRuntimeStub {
    public:
        RuntimeImpl(NativeBaseObjectMemory& memory,
                    IRwDictSchemaRuntime& schema,
                    std::array<std::int32_t, 2> game_config,
                    NativePkFileManagerRuntime* packages,
                    IActorControlDummyClump* dummy,
                    const std::filesystem::path& game_root,
                    std::uint32_t game_address)
            : database_(memory, schema, dummy, game_root, game_address),
              named_(memory),
              memory_(memory), game_config_(game_config),
              packages_(packages) {}
        void ConstructDatabase(std::uint32_t* storage) override;
        void DestructDatabase(std::uint32_t* storage) override;
        void ConstructDNameNode(std::uint32_t* storage) override;
        void DestructDNameNode(std::uint32_t* storage) override;
        void ConstructNamedNode(std::uint32_t* storage,
                                std::string_view name) override;
        void DestructNamedNode(std::uint32_t* storage) override;
        void SetNamedNodeErrorFlag(std::uint32_t* field,
                                   std::uint8_t value) override;
        std::int32_t GameConfig(std::uint32_t index) override;
        void PrepareScripts() override;
        void CleanupScripts() override;
        void InitializeDatabase(std::uint32_t* storage) override;
        void ShutdownDatabase(std::uint32_t* storage,
                              std::uint32_t event_context) override;
        void BindDatabaseActor(std::uint32_t* storage,
                               std::uint32_t actor);
        void BindNamedActor(std::uint32_t* storage,
                            std::uint32_t actor);
    private:
        DatabaseRuntime database_;
        NamedRuntime named_;
        NativeBaseObjectMemory& memory_;
        std::array<std::int32_t, 2> game_config_;
        NativePkFileManagerRuntime* packages_;
        bool scripts_active_{};
    };

    PalActorControlState state_{};
    std::filesystem::path game_root_;
    RuntimeImpl runtime_;
    bool constructed_{};
};

}  // namespace pal4
