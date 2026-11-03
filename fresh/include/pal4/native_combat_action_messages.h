#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeScriptEventRouter;
// Borrows task images (at least 96 bytes) and router. Detach before destroying
// a task. Payloads are synchronous borrowed views; retaining consumers copy them.
// Host router tokens occupy the original task's +80/+84 and +88/+92 pairs.
class NativeCombatActionMessages final {
public:
    using Animation = std::function<void(std::uint32_t task, std::uint32_t command)>;
    using Effect = std::function<void(std::uint32_t task, std::uint32_t first,
                                    std::uint32_t second, std::uint32_t text_header)>;
    explicit NativeCombatActionMessages(NativeScriptEventRouter&);
    ~NativeCombatActionMessages();
    NativeCombatActionMessages(const NativeCombatActionMessages&) = delete;
    NativeCombatActionMessages& operator=(const NativeCombatActionMessages&) = delete;
    // Both consumers are required. Register rejects already owned/nonempty slots.
    void Register(std::uint32_t task, Animation, Effect);
    void Detach(std::uint32_t task) noexcept;
    void Clear() noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
