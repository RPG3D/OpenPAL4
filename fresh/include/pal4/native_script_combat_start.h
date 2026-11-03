#pragma once
#include "pal4/script_combat_start.h"
#include <functional>
#include <memory>

namespace pal4 {
class NativeEventDocumentRuntime;
class NativeScriptCombatConfig;

// Borrows the actual pending executor, native XML memory and config owner.
// Entry synchronously consumes/copies the document; it must not retain the
// temporary root address. A failed entry preserves the pending monster queue.
class NativeScriptCombatStart final : public IScriptCombatStartRuntime {
public:
    using Enter = std::function<void(std::uint32_t document)>;
    NativeScriptCombatStart(ScriptCombatExecutor& executor,
        NativeEventDocumentRuntime& documents,
        NativeScriptCombatConfig& config, std::uint32_t game, Enter enter);
    ~NativeScriptCombatStart();
    ScriptCombatExecutor& ScriptExecutor() override;
    void CreateDocument(std::string_view root) override;
    void SetCombatWorld(std::string_view scene, std::string_view area,
        std::uint32_t mode, std::uint32_t property110) override;
    void SetCombatMonster(std::uint32_t id, std::uint32_t position, std::uint32_t abnormal) override;
    void LogStartCombat(std::uint32_t line, std::string_view placeholder,
        std::uint32_t count, std::string_view world) override;
    std::uint32_t GameInstance() override;
    void EnterGameState(std::uint32_t game, std::uint32_t state) override;
    std::uint32_t DestroyDocument() noexcept override;
    [[nodiscard]] std::uint32_t Document() const noexcept { return document_; }
private:
    class Properties;
    ScriptCombatExecutor& executor_;
    NativeEventDocumentRuntime& documents_;
    NativeScriptCombatConfig& config_;
    std::uint32_t game_;
    Enter enter_;
    std::uint32_t document_{};
    std::unique_ptr<Properties> properties_;
};
}
