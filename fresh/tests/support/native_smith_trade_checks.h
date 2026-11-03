#pragma once
namespace pal4 {
class NativeCeguiUiSession;class NativeSmithWindowLayout;class NativeSmithWindowLists;class NativeSmithWindowConfirmation;
class NativeSmithWindowCraftActions;class NativeInventorySession;class NativePrescriptionRecordCatalog;class NativeGameDatabase;
class NativeCommonDialogLayout;class NativeEventHandlerArray;class NativeInventoryEquipmentMutation;class NativeBaseObjectMemory;
struct PlayerManagerObjectState;
namespace test {
struct NativeSmithTradeFixture {
    NativeCeguiUiSession& ui;NativeSmithWindowLayout& layout;NativeSmithWindowLists& lists;
    NativeSmithWindowConfirmation& confirmation;NativeSmithWindowCraftActions& actions;
    NativeInventorySession& inventory;const NativePrescriptionRecordCatalog& recipes;const NativeGameDatabase& db;
    NativeCommonDialogLayout& dialog;NativeEventHandlerArray& activity;NativeInventoryEquipmentMutation& mutation;
    NativeBaseObjectMemory& memory;PlayerManagerObjectState& players;bool& smith_active;unsigned& mutation_sounds;
};
void CheckNativeSmithTrade(NativeSmithTradeFixture);
}
}
