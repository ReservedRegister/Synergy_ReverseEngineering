#ifndef EXT_MAIN_H
#define EXT_MAIN_H

bool InitExtensionSynergy();
void ApplyPatchesSynergy();
void HookFunctionsSynergy();

void FixCarSlashes();
uint32_t GetPassengerIndex(uint32_t player, uint32_t player_vehicle);
void MakePlayersLeaveVehicles();
void RemoveDanglingRestoredVehicles();
void EnterVehicles();
void ReorderGameSearchPaths();

class HooksSynergy
{
public:
	static uint32_t DirectMallocHookDedicatedSrv(uint32_t arg0);
	static uint32_t SimulateEntitiesHook(uint8_t simulating);
	static uint32_t SaveGameStateHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
	static uint32_t RestorePlayerHook(uint32_t arg0, uint32_t arg1);
	static uint32_t VehicleInitializeRestore(uint32_t arg0, uint32_t arg1, uint32_t arg2);
	static uint32_t fix_wheels_hook(uint32_t arg0, uint32_t arg1, uint32_t arg2);
	static uint32_t MountContentHook(uint32_t arg0);
	static __attribute__((fastcall)) uint32_t AutosaveHook(uint32_t arg0);
};

#endif