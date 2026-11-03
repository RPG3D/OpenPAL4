#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ILoadWindowUiRuntime {
public:
    virtual ~ILoadWindowUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t WindowNameAddress() = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree_address,
                                 std::string_view event_name,
                                 std::uint32_t& callback_address) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class LoadWindowUiRuntimeStub : public ILoadWindowUiRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t WindowNameAddress() override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertOrReplace(std::uint32_t, std::string_view,
                         std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class LoadWindowUiRegistration final {
public:
    static constexpr std::uint32_t kResultAddress = 0x8e43b8u;
    static constexpr std::uint32_t kCallbackVtable = 0x843494u;

    [[nodiscard]] static std::uint32_t RegisterShowUi(
        ILoadWindowUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowUiThunk(
        ILoadWindowUiRuntime& runtime);
};

}  // namespace pal4
