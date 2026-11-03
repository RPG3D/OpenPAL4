#pragma once

#include "pal4/native_dialog_particle_effects.h"
#include <memory>
#include <string_view>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectTreeStorage;

class INativeEffectObjectClassification {
public:
    virtual ~INativeEffectObjectClassification() = default;
    virtual std::string_view ObjectTypeName(std::uint32_t object) = 0;
    virtual bool UseFallbackQueue(std::uint32_t object) = 0;
    virtual bool UseCameraQueue(std::uint32_t object) = 0;
};

// Owns only the twelve list heads/nodes at group+68..+200. The caller supplies
// at least 212 bytes of group storage and its enabled byte at +4. Stop Dialogs
// before destruction. Inline EffectObject nodes belong to separately initialized
// group tree storage, borrowed through BindInlineTree.
class NativeEffectObjectQueues final : public INativeDialogParticleQueues {
public:
    NativeEffectObjectQueues(NativeBaseObjectMemory&, std::uint32_t group,
                             INativeEffectObjectClassification&);
    // Group lifecycle owns all list heads/nodes and must remain initialized
    // while this view is used. Its destructor releases no borrowed storage.
    struct BorrowInitialized {};
    NativeEffectObjectQueues(NativeBaseObjectMemory&, std::uint32_t group,
                             INativeEffectObjectClassification&, BorrowInitialized);
    ~NativeEffectObjectQueues();
    NativeEffectObjectQueues(const NativeEffectObjectQueues&) = delete;
    NativeEffectObjectQueues& operator=(const NativeEffectObjectQueues&) = delete;
    void QueueEffect(std::uint32_t parent, std::uint32_t effect) override;
    void RemoveEffect(std::uint32_t parent, std::uint32_t effect) override;
    void BindInlineTree(NativeEffectTreeStorage&);
    [[nodiscard]] std::uint32_t Count(std::uint32_t list_offset) const;
    [[nodiscard]] bool Contains(std::uint32_t list_offset, std::uint32_t object) const;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
