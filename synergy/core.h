#ifndef CORE_H
#define CORE_H

#define HOOK_MSG "Saved memory reference to leaked resources list: [%X]"
#define EXT_PREFIX "[SynergyUtils] "

extern ValueList leakedResourcesSaveRestoreSystem;
extern ValueList leakedResourcesEdtSystem;

extern bool sdktools_passed;
extern bool savegame;

bool IsAllowedToPatchSdkTools(uint32_t lib_base, uint32_t lib_size);
uint32_t GetCBaseEntitySynergy(uint32_t EHandle);
void PopulateHookExclusionListsSynergy();
void SaveLinkedList(ValueList leakList);
void RestoreLinkedLists();
void DestroyLinkedLists();
void SaveProcessId();
int ReleaseLeakedMemory(ValueList leakList, bool destroy, uint32_t current_cap, uint32_t allowed_cap, uint32_t free_perc);
void TriggerMovedExtension(uint32_t pTrigger);
bool HandleSourcemodOutputHook(uint32_t arg0, uint32_t player, uint32_t pTrigger, uint32_t outputName);
void TriggerMovedFailsafe();
void InitCoreSynergy();
void RemoveEntityNormalSynergy(uint32_t entity_object, bool validate);
void InstaKillSynergy(uint32_t entity_object, bool validate);

#endif