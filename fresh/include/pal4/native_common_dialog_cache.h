#pragma once
#include "pal4/native_common_dialog_layout.h"
#include <array>
#include <stdexcept>
#include <utility>
namespace pal4 {
// One live layout per kind/UI session. Mirrors the identity of the original
// UIFrameManager cached dialog without allocating duplicate CEGUI window names.
// Consumers share the complete layout/controller, callbacks and generation.
class NativeCommonDialogCache final {
public:
    NativeCommonDialogCache(HMODULE module,NativeCeguiUiSession& ui,std::filesystem::path root)
        :module_(module),ui_(ui),root_(std::move(root)){}
    std::shared_ptr<NativeCommonDialogLayout> Acquire(NativeCommonDialogLayout::Kind kind=NativeCommonDialogLayout::Kind::common){
        const auto index=static_cast<unsigned>(kind);
        if(index>=dialogs_.size())throw std::invalid_argument("unknown common dialog kind");
        if(auto live=dialogs_[index].lock())return live;
        auto dialog=std::make_shared<NativeCommonDialogLayout>(module_,ui_,root_,kind);
        dialogs_[index]=dialog;return dialog;
    }
    // Read-only inspection must not construct a missing prompt. In particular
    // System can own the shared dialog before any script has requested it.
    std::shared_ptr<NativeCommonDialogLayout> Find(NativeCommonDialogLayout::Kind kind=NativeCommonDialogLayout::Kind::common)const{
        const auto index=static_cast<unsigned>(kind);
        if(index>=dialogs_.size())throw std::invalid_argument("unknown common dialog kind");
        return dialogs_[index].lock();
    }
    std::size_t LiveCount()const noexcept{
        std::size_t count{};for(const auto& dialog:dialogs_)count+=!dialog.expired();return count;
    }
private:
    HMODULE module_;NativeCeguiUiSession& ui_;std::filesystem::path root_;
    std::array<std::weak_ptr<NativeCommonDialogLayout>,2> dialogs_;
};
}
