#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptPetVisibilityRuntime {
public:
    virtual ~IScriptPetVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PetManager() = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void RemovePet(std::uint32_t arena) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPet(
        std::uint32_t manager) = 0;
    virtual void CreatePet(std::uint32_t manager,
                           std::string_view kind, std::string_view id,
                           std::uint32_t mode) = 0;
    virtual void InitializePet(std::uint32_t manager,
                               std::uint32_t player) = 0;
    virtual void SetStateFlag(std::uint32_t manager,
                              std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t AddPet(std::uint32_t arena) = 0;
};

class ScriptPetVisibility final {
public:
    // IDA 0x5d8760, giPetShow.
    [[nodiscard]] static std::uint32_t SetEnabled(
        std::uint8_t enabled, IScriptPetVisibilityRuntime& runtime);
};

}  // namespace pal4
