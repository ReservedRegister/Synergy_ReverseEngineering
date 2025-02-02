#ifndef EXT_MAIN_H
#define EXT_MAIN_H

bool InitExtensionBlackMesa();
void ApplyPatchesBlackMesa();
void HookFunctionsBlackMesa();
void CorrectVphysicsEntity(uint32_t ent);

class HooksBlackMesa
{
public:
	static uint32_t SimulateEntitiesHook(uint32_t arg0);
	static uint32_t UTIL_GetLocalPlayerHook();
	static uint32_t CXenShieldController_UpdateOnRemoveHook(uint32_t arg0);
	static uint32_t InstaKillPatchHook(uint32_t arg0);
	static uint32_t TestGroundMove(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);
	static uint32_t ShouldHitEntityHook(uint32_t arg0, uint32_t arg1, uint32_t arg2);
	static uint32_t VpkCacheBufferAllocHook(uint32_t arg0);
	static uint32_t RagdollBreakHook(uint32_t arg0, uint32_t arg1, uint32_t arg2);
	static uint32_t CreateNoSpawnHookRagdollBreaking(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
	static uint32_t VPhysicsUpdateHook(uint32_t arg0, uint32_t arg1);
};

#endif