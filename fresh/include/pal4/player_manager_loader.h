#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using PlayerManagerVector = std::array<std::uint32_t, 3>;

class IPlayerManagerLoadRuntime {
public:
    virtual ~IPlayerManagerLoadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Log(std::uint32_t line) = 0;
    virtual void ProcessScripts() = 0;
    [[nodiscard]] virtual std::uint32_t GameDB() = 0;
    [[nodiscard]] virtual std::uint32_t FindRole(
        std::uint32_t role_table, std::uint32_t index) = 0;
    virtual void CopySlotData(std::uint32_t slot,
                              std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void CallLoadActor(std::uint32_t slot,
        std::uint32_t virtual_slot, std::uint32_t name,
        std::uint32_t model, std::uint32_t enabled) = 0;
    virtual void CallVectorMethod(std::uint32_t slot,
        std::uint32_t virtual_slot, PlayerManagerVector& vector,
        std::uint32_t option) = 0;
    virtual void SetStateFlag(std::uint32_t slot,
                              std::uint32_t value) = 0;
    virtual void CallScriptIndex(std::uint32_t script,
        std::uint32_t virtual_slot, std::uint32_t index) = 0;
    virtual void ProcessLoadedData(std::uint32_t manager) = 0;
    virtual void CleanupScripts() = 0;
};

class PlayerManagerLoadRuntimeStub : public IPlayerManagerLoadRuntime {
public:
    std::uint32_t Log(std::uint32_t) override;
    void ProcessScripts() override;
    std::uint32_t GameDB() override;
    std::uint32_t FindRole(std::uint32_t, std::uint32_t) override;
    void CopySlotData(std::uint32_t, std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void CallLoadActor(std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t) override;
    void CallVectorMethod(std::uint32_t, std::uint32_t,
        PlayerManagerVector&, std::uint32_t) override;
    void SetStateFlag(std::uint32_t, std::uint32_t) override;
    void CallScriptIndex(std::uint32_t, std::uint32_t,
        std::uint32_t) override;
    void ProcessLoadedData(std::uint32_t) override;
    void CleanupScripts() override;
};

class PlayerManagerLoader final {
public:
    static constexpr std::uint32_t kSlotBytes = 0xb14u;
    static constexpr std::uint32_t kSlotCount = 4u;
    static constexpr std::uint32_t kScriptOffset = 0x654u;
    static constexpr std::uint32_t kModelOffset = 0xa1cu;
    static constexpr std::uint32_t kEmptyString = 0x841964u;
    static constexpr std::array<std::uint32_t, kSlotCount> kNames{
        0x8a58e4u, 0x8a58d4u, 0x8a58c4u, 0x8a58b4u};

    // IDA 0x441250.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t manager, IPlayerManagerLoadRuntime& runtime);
};

}  // namespace pal4
