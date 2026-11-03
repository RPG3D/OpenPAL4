#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ICollectionUiRegistrationRuntime {
public:
    virtual ~ICollectionUiRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::size_t bytes) = 0;
    virtual void InitializeCallback(std::uint32_t address,
                                    std::uint32_t vtable,
                                    std::string_view window_name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree_address,
                                 std::string_view event_name,
                                 std::uint32_t& callback_address) = 0;
    virtual void DeleteCallback(std::uint32_t address,
                                std::uint32_t flags) = 0;
};

class CollectionUiRegistrationRuntimeStub :
    public ICollectionUiRegistrationRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::size_t) override { return 0u; }
    void InitializeCallback(std::uint32_t, std::uint32_t,
                            std::string_view) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertOrReplace(std::uint32_t, std::string_view,
                         std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class CollectionUiRegistration final {
public:
    static constexpr std::uint32_t kResultAddress = 0x8e43a8u;
    static constexpr std::uint32_t kCallbackVtable = 0x843494u;

    [[nodiscard]] static std::uint32_t RegisterShowCastUi(
        ICollectionUiRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowCastUiThunk(
        ICollectionUiRegistrationRuntime& runtime);
};

}  // namespace pal4
