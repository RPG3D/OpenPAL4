#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ThunderObjectState final {
    std::array<std::uint32_t, 73u> words{};
};
static_assert(sizeof(ThunderObjectState) == 0x124u);

class IThunderObjectRuntime {
public:
    virtual ~IThunderObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte() = 0;
    virtual void ResetErrorField(ThunderObjectState& state,
                                 std::uint32_t offset) = 0;
    virtual void ResetProperties(ThunderObjectState& state,
                                 std::uint32_t offset) = 0;
    virtual void CleanupMemoryBlock(ThunderObjectState& state,
                                    std::uint32_t offset) = 0;
};

class ThunderObjectRuntimeStub final : public IThunderObjectRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t ConstructorStackByte() override { return 0u; }
    void ResetErrorField(ThunderObjectState&, std::uint32_t) override {}
    void ResetProperties(ThunderObjectState&, std::uint32_t) override {}
    void CleanupMemoryBlock(ThunderObjectState&, std::uint32_t) override {}
};

class ThunderObject final {
public:
    // IDA 0x61ab40/0x61ad90.
    static ThunderObjectState& Construct(ThunderObjectState& state,
                                         IThunderObjectRuntime& runtime);
    static void Destroy(ThunderObjectState& state,
                        IThunderObjectRuntime& runtime);
    // IDA 0x61b800/0x61b810.
    [[nodiscard]] static std::uint32_t ResourceHandle(
        const ThunderObjectState& state);
    [[nodiscard]] static std::string_view ClassName();
};

class IThunderObjectPlaybackRuntime {
public:
    virtual ~IThunderObjectPlaybackRuntime() = default;
    virtual void StartFacet(ThunderObjectState& state,
                            std::array<std::uint32_t, 4u>& outputs) = 0;
    [[nodiscard]] virtual double Duration(ThunderObjectState& state) = 0;
};

class ThunderObjectPlayback final {
public:
    // IDA 0x61b820.
    static void SetEnabled(ThunderObjectState& state, std::uint8_t enabled,
                           IThunderObjectPlaybackRuntime& runtime);
};

}  // namespace pal4
