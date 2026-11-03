#pragma once

#include "pal4/effect_handle.h"
#include "pal4/effect_system_timed.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectTreeStorage;

// XML effects are HookEffect/StripSystemEffect objects, not pooled Dialogs.
// The owning dispatcher must implement their lifecycle without calling EXE vtables.
class INativeContextEffectObjects {
public:
    virtual ~INativeContextEffectObjects() = default;
    virtual void Stop(std::uint32_t object) = 0;
    virtual void Destroy(std::uint32_t object, std::uint32_t flag) = 0;
    virtual float DrainTime(std::uint32_t object) = 0;
    virtual void Update(std::uint32_t object, double seconds) = 0;
    virtual void Draw(std::uint32_t object, std::uint32_t context) = 0;
};

// Borrows initialized context/timed storage, owner validation and payload services.
// Reuses the shared handle/tree/timer algorithms; it never owns payloads itself.
class NativeEffectHandleContextRuntime final : public IEffectHandleContextRuntime,
    public IEffectSystemTimedRuntime {
public:
    NativeEffectHandleContextRuntime(NativeBaseObjectMemory&, NativeEffectTreeStorage&,
        std::uint32_t owner, IEffectHandlePropertyRuntime&, INativeContextEffectObjects&);
    std::uint32_t InitializeSystem() override;
    std::uint32_t RegisterSystem() override;
    bool Validate(std::uint32_t system, std::uint32_t owner) override;
    bool ContainsOwner(std::uint32_t system, std::uint32_t owner) override;
    std::uint32_t FindElement(std::uint32_t tree, const EffectSystemHandleWords&) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::string ReadNameAttribute(std::uint32_t attributes) override;
    void StopContextDialog(std::uint32_t object) override;
    void DestroyContextDialog(std::uint32_t object) override;
    float ContextDialogTime(std::uint32_t object) override;
    void PositionContextDialog(std::uint32_t owner, std::uint32_t object, float time) override;
    void EraseContext(std::uint32_t tree, std::uint32_t node) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    void WriteFloat(std::uint32_t address, float value) override;
    std::uint32_t SharedNil(std::uint32_t tree) override;
    void InsertTimedNode(std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t object, float lifetime) override;
    void UpdateTimedObject(std::uint32_t object, double seconds) override;
    void DrawTimedObject(std::uint32_t object, std::uint32_t context) override;
    void BeforeDestroyEffect(std::uint32_t object) override;
    void DestroyEffect(std::uint32_t object, std::uint32_t flag) override;
    std::uint32_t EraseTimedNode(std::uint32_t tree, std::uint32_t node) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeEffectTreeStorage& trees_;
    std::uint32_t owner_;
    IEffectHandlePropertyRuntime& properties_;
    INativeContextEffectObjects& objects_;
    void CheckTree(std::uint32_t tree, std::uint32_t offset) const;
};
} // namespace pal4
