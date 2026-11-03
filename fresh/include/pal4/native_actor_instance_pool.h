#pragma once

#include "pal4/native_actor_instance_owner.h"
#include <functional>
#include <memory>
#include <string_view>

namespace pal4 {
// Owns cached children; checked-out children belong to their caller. Both
// services are mandatory: activity comes from Combat, creation from its loader.
// Pool policy does not substitute resource preparation for original Play.
class NativeActorInstancePool final {
public:
    using Owner=std::unique_ptr<NativeActorInstanceOwner>;
    using Factory=std::function<Owner(std::string_view)>;
    NativeActorInstancePool(std::function<bool()> combat_inactive, Factory factory,
                            std::uint32_t limit=12u);
    ~NativeActorInstancePool();
    NativeActorInstancePool(const NativeActorInstancePool&)=delete;
    NativeActorInstancePool& operator=(const NativeActorInstancePool&)=delete;
    [[nodiscard]] Owner TakeOrCreate(std::string_view name);
    // On deactivation failure the caller retains ownership. On success owner
    // becomes null; the cache evicts its oldest entry if already at the limit.
    void Return(Owner& owner, std::uint32_t effect_context=0u);
    void Clear();
    [[nodiscard]] std::size_t CachedCount() const noexcept;
    [[nodiscard]] std::uint32_t Capacity() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pal4
