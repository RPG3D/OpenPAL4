#pragma once

#include <cstdint>
#include <map>
#include <string_view>

namespace pal4 {

struct PalTextureDictionaryAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IPalTextureDictionaryAccessRuntime {
public:
    virtual ~IPalTextureDictionaryAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) = 0;
};

class PalTextureDictionaryAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e40f8u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e40f4u;
    static constexpr std::uint32_t kFactoryAddress = 0x41a490u;
    static constexpr std::uint32_t kDestroyAddress = 0x7de5d0u;

    [[nodiscard]] static std::uint32_t Get(
        PalTextureDictionaryAccessState& state,
        IPalTextureDictionaryAccessRuntime& runtime);
};

struct PalTextureDictionaryState {
    std::uint32_t vtable{};
    std::uint32_t texture_head{};
    std::uint32_t flag{};
};
static_assert(sizeof(PalTextureDictionaryState) == 12u);

class IPalTextureDictionaryRuntime {
public:
    virtual ~IPalTextureDictionaryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual PalTextureDictionaryState& Access(
        std::uint32_t address) = 0;
    virtual void BaseConstruct(std::uint32_t address) = 0;
    virtual void BaseDestruct(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BaseAddTexture(
        std::uint32_t dictionary, std::uint32_t texture) = 0;
};

class PalTextureDictionaryRuntimeStub final
    : public IPalTextureDictionaryRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] PalTextureDictionaryState& Access(
        std::uint32_t address) override;
    void BaseConstruct(std::uint32_t address) override;
    void BaseDestruct(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t BaseAddTexture(
        std::uint32_t dictionary, std::uint32_t texture) override;

private:
    std::map<std::uint32_t, PalTextureDictionaryState> objects_;
    std::uint32_t next_address_{0x1000u};
};

class PalTextureDictionary final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x842174u;

    // 0x41a490 through 0x41a540, including the previously undefined
    // deleting destructor at 0x41a510.
    [[nodiscard]] static std::uint32_t Create(
        IPalTextureDictionaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, IPalTextureDictionaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t address, std::uint8_t flags,
        IPalTextureDictionaryRuntime& runtime);
    static void Destruct(std::uint32_t address,
                         IPalTextureDictionaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddTexture(
        std::uint32_t dictionary, std::uint32_t texture,
        IPalTextureDictionaryRuntime& runtime);
};

}  // namespace pal4
