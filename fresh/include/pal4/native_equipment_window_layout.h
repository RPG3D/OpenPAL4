#pragma once
#include "pal4/equipment_window.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;
// Owns original XML, DLL subscribers and two native navigation buffers. The
// page supplies rewritten handlers; no original EXE vtables are invoked.
class NativeEquipmentWindowLayout final {
public:
    using Handler=std::function<bool(std::uint32_t owner,std::uint32_t handler,std::uint32_t argument)>;
    NativeEquipmentWindowLayout(NativeCeguiUiSession&,std::uint32_t frame_manager,Handler);
    ~NativeEquipmentWindowLayout();
    NativeEquipmentWindowLayout(const NativeEquipmentWindowLayout&)=delete;
    NativeEquipmentWindowLayout& operator=(const NativeEquipmentWindowLayout&)=delete;
    EquipmentWindowState& State();
    std::uint32_t FrameAddress()const noexcept;
    void DispatchCommand(std::string_view,std::uint32_t argument);
    void RethrowEventError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
