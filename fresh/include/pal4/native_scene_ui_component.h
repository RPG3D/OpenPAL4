#pragma once
#include <windows.h>
#ifdef FindWindow
#undef FindWindow
#endif
#include "pal4/scene_ui_component.h"

namespace pal4 {
// Bind the six controls of an already loaded minimap layout, preserving477990.
// Returned window addresses are borrowed from the original CEGUI WindowManager.
SceneUiComponentState InitializeNativeSceneUiControls(HMODULE module);
}
