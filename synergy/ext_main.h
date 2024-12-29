#ifndef EXT_MAIN_H
#define EXT_MAIN_H

bool InitExtensionSynergy();
void ApplyPatchesSynergy();
void HookFunctionsSynergy();

class HooksSynergy
{
public:
	static uint32_t UTIL_RemoveBaseHook(uint32_t arg0);
	static uint32_t UTIL_RemoveIserverHook(uint32_t arg0);
	static uint32_t InstaKillHook(uint32_t arg0);
};

#endif