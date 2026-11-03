#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct CeguiKeyMapping {
    std::uint32_t windows_code;
    std::uint32_t cegui_code;
};

class CeguiSystemState final {
public:
    static constexpr std::uint32_t kObservedBytes = 0x20u;

    [[nodiscard]] std::array<std::uint8_t, kObservedBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kObservedBytes>& Bytes()
        const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    [[nodiscard]] float ReadFloat(std::uint32_t offset) const;
    void WriteFloat(std::uint32_t offset, float value);
    void EnableMouseCapture() noexcept;
    void DisableMouseCapture() noexcept;
    [[nodiscard]] std::array<float, 2> TransformMouseCoordinates(
        const std::array<float, 2>& point) const;
    [[nodiscard]] std::array<float, 2> TransformUiCoordinates(
        const std::array<float, 2>& point) const;

private:
    std::array<std::uint8_t, kObservedBytes> bytes_{};
};

static_assert(sizeof(CeguiSystemState) == CeguiSystemState::kObservedBytes);

class ICeguiSystemRuntime {
public:
    virtual ~ICeguiSystemRuntime() = default;
    virtual void ConstructObject(std::uint32_t address) = 0;
    virtual void InsertKeyMapping(std::uint32_t object,
                                  CeguiKeyMapping mapping) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateSystem(
        std::uint32_t bytes) = 0;
    virtual void ConstructSystemWithProvider(
        std::uint32_t address, std::uint32_t renderer,
        std::uint32_t provider, std::string_view log_filename) = 0;
    virtual void ConstructSystemWithDefaultProvider(
        std::uint32_t address, std::uint32_t renderer,
        std::string_view log_filename) = 0;
    [[nodiscard]] virtual std::uint32_t GetLogger() = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
};

class CeguiRuntimeStub final : public ICeguiSystemRuntime {
public:
    void ConstructObject(std::uint32_t address) override;
    void InsertKeyMapping(std::uint32_t object,
                          CeguiKeyMapping mapping) override;
    [[nodiscard]] std::uint32_t AllocateSystem(
        std::uint32_t bytes) override;
    void ConstructSystemWithProvider(
        std::uint32_t address, std::uint32_t renderer,
        std::uint32_t provider, std::string_view log_filename) override;
    void ConstructSystemWithDefaultProvider(
        std::uint32_t address, std::uint32_t renderer,
        std::string_view log_filename) override;
    [[nodiscard]] std::uint32_t GetLogger() override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
};

class CeguiSystemInitializer final {
public:
    static constexpr std::uint32_t kSystemBytes = 0x1b0u;
    static constexpr std::uint32_t kSystemVtable = 0x841ea8u;
    static constexpr std::string_view kLogFilename = "CEGUI.log";

    [[nodiscard]] static const std::array<CeguiKeyMapping, 100>&
    KeyMappings() noexcept;
    [[nodiscard]] static std::uint32_t Initialize(
        CeguiSystemState& state, std::uint32_t self_address,
        std::uint32_t renderer, std::uint32_t resource_provider,
        std::int32_t screen_width, std::int32_t screen_height,
        ICeguiSystemRuntime& runtime);
};

}  // namespace pal4
