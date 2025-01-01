#ifndef EXT_MAIN_H
#define EXT_MAIN_H

bool InitExtensionSynergy();
void ApplyPatchesSynergy();
void HookFunctionsSynergy();

class HooksSynergy
{
public:
	static uint32_t CallocHook(uint32_t nitems, uint32_t size);
	static uint32_t MallocHookSmall(uint32_t size);
	static uint32_t MallocHookLarge(uint32_t size);
	static uint32_t OperatorNewHook(uint32_t size);
	static uint32_t OperatorNewArrayHook(uint32_t size);
	static uint32_t ReallocHook(uint32_t old_ptr, uint32_t new_size);
	static uint32_t CreateEntityByNameHook(uint32_t arg0, uint32_t arg1);
	static uint32_t DirectMallocHookDedicatedSrv(uint32_t arg0);
	static uint32_t EmptyCall();
	static uint32_t SimulateEntitiesHook(uint8_t simulating);
	static uint32_t PhysSimEnt(uint32_t arg0);
	static uint32_t HookInstaKill(uint32_t arg0);
	static uint32_t PackedStoreDestructorHook(uint32_t arg0);
	static uint32_t UTIL_RemoveHookFailsafe(uint32_t arg0);
	static uint32_t UTIL_RemoveBaseHook(uint32_t arg0);
	static uint32_t AcceptInputHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
	static uint32_t UpdateOnRemove(uint32_t arg0);
	static uint32_t VPhysicsSetObjectHook(uint32_t arg0, uint32_t arg1);
	static uint32_t CollisionRulesChangedHook(uint32_t arg0);
	static __attribute__((fastcall)) uint32_t AutosaveHook(uint32_t arg0);
	static uint32_t CanSatisfyVpkCacheInternalHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);
};

#endif