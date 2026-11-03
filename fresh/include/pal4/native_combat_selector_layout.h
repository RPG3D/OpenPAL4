#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
enum class NativeCombatSelectorKind {Magic,Property,Stunt};
// Actual three selector constructors and original DLL controls/subscriptions.
// Hidden construction is independent of list/game services. Controllers bind
// their handlers and visibility/input; this owner never simulates game selection.
class NativeCombatSelectorLayout final {
public:
    using Handler=std::function<bool(std::uint32_t callback,std::uint32_t event)>;
    NativeCombatSelectorLayout(NativeCombatSelectorKind,NativeCeguiUiSession&,const std::filesystem::path&);
    // Original Action console embeds these images; borrowed storage outlives
    // this owner and must have exactly22/23/16 initially-zero words.
    NativeCombatSelectorLayout(NativeCombatSelectorKind,NativeCeguiUiSession&,const std::filesystem::path&,
        std::span<std::uint32_t> storage);
    ~NativeCombatSelectorLayout();
    NativeCombatSelectorLayout(const NativeCombatSelectorLayout&)=delete;
    NativeCombatSelectorLayout& operator=(const NativeCombatSelectorLayout&)=delete;
    std::uint32_t Address()const noexcept;
    std::uint32_t Root()const noexcept;
    std::uint32_t List()const noexcept;
    std::uint32_t Description()const noexcept;
    std::uint32_t Control(std::string_view suffix);
    std::uint32_t& Word(std::uint32_t offset);
    std::size_t SubscriptionCount()const noexcept;
    std::size_t TemporaryAllocations()const noexcept;
    bool HasHandler()const noexcept;
    bool InCallback()const noexcept;
    void BindHandler(Handler);
    void SetVisible(bool visible, std::uint32_t parent = 0u);
    void RethrowEventError();
    void Close();
private:
    class State;
    class Runtime;
    std::shared_ptr<State> state_;
};
}
