#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct BaseObjectState {
    static constexpr std::uint32_t kSize = 0x60cu;
    std::array<std::uint32_t, kSize / 4u> words{};

    [[nodiscard]] std::uint32_t* Data() noexcept {
        return words.data();
    }
    [[nodiscard]] const std::uint32_t* Data() const noexcept {
        return words.data();
    }

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const {
        return words[offset / 4u];
    }
    void Write(std::uint32_t offset, std::uint32_t value) {
        words[offset / 4u] = value;
    }
};

class IBaseObjectRuntime {
public:
    virtual ~IBaseObjectRuntime() = default;
    virtual void ConstructResource(BaseObjectState& state,
                                   std::uint32_t offset) = 0;
    virtual void DestroyResource(BaseObjectState& state,
                                 std::uint32_t offset) = 0;
    virtual void InitializeAnimation(BaseObjectState& state,
                                     std::uint32_t offset) = 0;
    virtual void CleanupAnimation(BaseObjectState& state,
                                  std::uint32_t offset) = 0;
    virtual void ConstructEffect(BaseObjectState& state,
                                 std::uint32_t offset) = 0;
    virtual void ResetEffect(BaseObjectState& state,
                             std::uint32_t offset) = 0;
    virtual void ConstructText(BaseObjectState& state,
                               std::uint32_t offset) = 0;
    virtual void CleanupText(BaseObjectState& state,
                             std::uint32_t offset) = 0;
    virtual void ConstructCharacter(BaseObjectState& state,
                                    std::uint32_t offset) = 0;
    virtual void DestroyCharacter(BaseObjectState& state,
                                  std::uint32_t offset) = 0;
};

class BaseObjectRuntimeStub : public IBaseObjectRuntime {
public:
    void ConstructResource(BaseObjectState&, std::uint32_t) override {}
    void DestroyResource(BaseObjectState&, std::uint32_t) override {}
    void InitializeAnimation(BaseObjectState&, std::uint32_t) override {}
    void CleanupAnimation(BaseObjectState&, std::uint32_t) override {}
    void ConstructEffect(BaseObjectState&, std::uint32_t) override {}
    void ResetEffect(BaseObjectState&, std::uint32_t) override {}
    void ConstructText(BaseObjectState&, std::uint32_t) override {}
    void CleanupText(BaseObjectState&, std::uint32_t) override {}
    void ConstructCharacter(BaseObjectState&, std::uint32_t) override {}
    void DestroyCharacter(BaseObjectState&, std::uint32_t) override {}
};

class BaseObjectLifecycle final {
public:
    [[nodiscard]] static std::uint32_t Construct(
        BaseObjectState& state, std::uint32_t address,
        IBaseObjectRuntime& runtime);
    static void Destroy(BaseObjectState& state, IBaseObjectRuntime& runtime);
};

}  // namespace pal4
