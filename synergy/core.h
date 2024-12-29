#ifndef CORE_H
#define CORE_H

#define HOOK_MSG "Saved memory reference to leaked resources list: [%X]"
#define EXT_PREFIX "[SynergyUtils] "

extern bool disable_internal_remove_incrementor;

bool InitCoreSynergy();
void PopulateHookExclusionListsSynergy();
uint32_t GetCBaseEntitySynergy(uint32_t EHandle);
void InstaKillSynergy(uint32_t entity_object, bool validate);
void RemoveEntityNormalSynergy(uint32_t entity_object, bool validate);

#endif