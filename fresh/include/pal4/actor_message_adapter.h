#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/nil_string_data_object.h"

namespace pal4 {

struct ActorMessageAccessState {
    std::uint8_t guard = 0;
    std::uint32_t instance = 0;
};

class IActorMessageAccessRuntime {
public:
    virtual ~IActorMessageAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) = 0;
};

class ActorMessageAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e3f98u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e3f94u;
    static constexpr std::uint32_t kFactoryAddress = 0x40ff40u;
    static constexpr std::uint32_t kDestroyAddress = 0x40ffa0u;

    [[nodiscard]] static std::uint32_t Get(
        ActorMessageAccessState& state,
        IActorMessageAccessRuntime& runtime);
};

class ActorMessageAdapter;

class IActorMessageAdapterRuntime : public INilStringDataRuntime {
public:
    virtual void Free(std::uint32_t address) override = 0;
    [[nodiscard]] virtual ActorMessageAdapter& Access(
        std::uint32_t address) = 0;
};

class ActorMessageAdapter final {
public:
    static constexpr std::uint32_t kBytes = 0x2cu;
    static constexpr std::uint32_t kCopyCallback = 0x40feb0u;
    static constexpr std::uint32_t kCreateCallback = 0x40fee0u;
    static constexpr std::uint32_t kFreeCallback = 0x5fc6c0u;

    [[nodiscard]] static std::uint32_t Create(
        IActorMessageAdapterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, IActorMessageAdapterRuntime& runtime);
    static void Destroy(std::uint32_t address,
                        IActorMessageAdapterRuntime& runtime);
    static void DestroyAndFree(std::uint32_t address,
                               IActorMessageAdapterRuntime& runtime);
    static void Clear(std::uint32_t address,
                      IActorMessageAdapterRuntime& runtime);

    [[nodiscard]] CallbackCollectionState& Collection() noexcept {
        return collection_;
    }
    [[nodiscard]] const CallbackCollectionState& Collection() const noexcept {
        return collection_;
    }
    [[nodiscard]] std::uint32_t TrailingOwnedObject() const noexcept {
        return trailing_owned_object_;
    }
    void SetTrailingOwnedObject(std::uint32_t value) noexcept {
        trailing_owned_object_ = value;
    }

private:
    CallbackCollectionState collection_{};
    std::uint32_t trailing_owned_object_ = 0;
};

static_assert(sizeof(ActorMessageAdapter) == ActorMessageAdapter::kBytes);

}  // namespace pal4
