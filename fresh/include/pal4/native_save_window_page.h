#pragma once
#include "pal4/save_window_lifecycle.h"
#include <filesystem>
#include <memory>
#include <cstdint>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class DialogManagerState;
// Owns the original520-byte numbered rows. Borrowed original controls, DLL,
// database, UI and globals outlive Close; reset the list before freeing rows.
class NativeSaveWindowPage final {
public:
    NativeSaveWindowPage(NativeCeguiUiSession&,SaveWindowState&,const NativeGameDatabase&,
        const std::filesystem::path& archive_root,const DialogManagerState&);
    ~NativeSaveWindowPage();
    NativeSaveWindowPage(const NativeSaveWindowPage&)=delete;
    NativeSaveWindowPage& operator=(const NativeSaveWindowPage&)=delete;
    void Populate(std::int32_t page,bool reset);
    void Close();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
