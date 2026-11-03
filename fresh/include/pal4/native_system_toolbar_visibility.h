#pragma once

namespace pal4 {
class NativePalGameIVTextureStorage;
class NativeEventHandlerArray;
struct PlayerManagerObjectState;

// Borrowed only during a visibility transition. The active array borrows the
// toolbar component until hide; remove it before destroying its controller.
struct NativeSystemToolbarVisibilityServices {
    NativePalGameIVTextureStorage& game;
    PlayerManagerObjectState& players;
    NativeEventHandlerArray& active;
};
}  // namespace pal4
