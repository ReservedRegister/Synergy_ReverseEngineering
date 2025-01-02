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
	static uint32_t DirectMallocHookDedicatedSrv(uint32_t arg0);
	static uint32_t SimulateEntitiesHook(uint8_t simulating);
	static __attribute__((fastcall)) uint32_t AutosaveHook(uint32_t arg0);
};

#endif