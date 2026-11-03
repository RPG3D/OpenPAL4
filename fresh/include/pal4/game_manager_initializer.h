#pragma once

#include "pal4/screen_effect_dispatcher.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ManagerObjectImage {
    std::uint32_t size{};
    std::uint32_t base_vtable{};
    std::uint32_t final_vtable{};
    std::array<std::uint8_t, 48> bytes{};
    std::array<bool, 48> written{};

    [[nodiscard]] bool HasWrite(std::size_t offset) const;
    [[nodiscard]] std::uint32_t Dword(std::size_t offset) const;
    [[nodiscard]] std::uint8_t IsInitialized() const;
    [[nodiscard]] std::uint8_t IsEnabled() const;
    void InstallBaseVtable();
    ManagerObjectImage& InitializeExtendedTail(std::uint8_t source_byte);
};

class IGameManagerInitRuntime {
public:
    virtual ~IGameManagerInitRuntime() = default;
    virtual void ResetExisting(ScreenEffectDispatcherState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCameraRecord() = 0;
    [[nodiscard]] virtual bool InitializeCamera(std::uint32_t record) = 0;
    virtual void DestroyCameraRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateObject(std::uint32_t size) = 0;
    virtual void InitializeObject(std::uint32_t address,
                                  const ManagerObjectImage& image) = 0;
    virtual void MakePair(std::uint32_t key, std::uint32_t address) = 0;
    virtual void InsertManager(std::uint32_t key,
                               std::uint32_t address) = 0;
    virtual void SetNodeData(std::uint32_t key) = 0;
};

class GameManagerInitRuntimeStub final : public IGameManagerInitRuntime {
public:
    void ResetExisting(ScreenEffectDispatcherState& state) override;
    [[nodiscard]] std::uint32_t AllocateCameraRecord() override;
    [[nodiscard]] bool InitializeCamera(std::uint32_t record) override;
    void DestroyCameraRecord(std::uint32_t record) override;
    [[nodiscard]] std::uint32_t AllocateObject(std::uint32_t size) override;
    void InitializeObject(std::uint32_t address,
                          const ManagerObjectImage& image) override;
    void MakePair(std::uint32_t key, std::uint32_t address) override;
    void InsertManager(std::uint32_t key,
                       std::uint32_t address) override;
    void SetNodeData(std::uint32_t key) override;
};

class GameManagerInitializer final {
public:
    [[nodiscard]] static bool Initialize(ScreenEffectDispatcherState& state,
                                         std::uint8_t uninitialized_stack_byte,
                                         IGameManagerInitRuntime& runtime);
};

}  // namespace pal4
