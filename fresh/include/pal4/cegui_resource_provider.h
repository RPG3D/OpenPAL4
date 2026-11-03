#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

class ICeguiResourceRuntime {
public:
    virtual ~ICeguiResourceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DefaultProviderVtable() = 0;
    [[nodiscard]] virtual std::uint32_t BaseProviderVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructAlternateStream(
        std::uint32_t address) = 0;
    virtual void ConstructDefaultStream(std::uint32_t address) = 0;
};

class CeguiResourceRuntimeStub : public ICeguiResourceRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    [[nodiscard]] std::uint32_t DefaultProviderVtable() override;
    [[nodiscard]] std::uint32_t BaseProviderVtable() override;
    void DestroyString(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ConstructAlternateStream(
        std::uint32_t address) override;
    void ConstructDefaultStream(std::uint32_t address) override;

private:
    std::uint32_t next_address_ = 0x10000u;
    std::map<std::uint32_t, std::uint32_t> words_;
};

class CeguiResourceProvider final {
public:
    static constexpr std::uint32_t kBytes = 0x9cu;
    static constexpr std::uint32_t kVtable = 0x842100u;

    // 0x415550 and 0x4155b0.
    [[nodiscard]] static std::uint32_t Create(ICeguiResourceRuntime& runtime);
    static void Destroy(std::uint32_t object,
                        ICeguiResourceRuntime& runtime);
};

class CeguiFileStreamFactory final {
public:
    static constexpr std::uint32_t kBytes = 8u;
    static constexpr std::uint32_t kDefaultVtable = 0x84210cu;

    // 0x4155d0. The flag is read from g_GameState + 0x58c.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t game_state_base, ICeguiResourceRuntime& runtime);
};

}  // namespace pal4
