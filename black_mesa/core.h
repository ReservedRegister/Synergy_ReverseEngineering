#ifndef CORE_H
#define CORE_H

#define HOOK_MSG "Saved memory reference to leaked resources list: [%X]"
#define EXT_PREFIX "[BlackMesaUtils] "

extern uint32_t last_ragdoll_gib;
extern int ragdoll_breaking_gib_counter;
extern bool is_currently_ragdoll_breaking;

void InitCoreBlackMesa();
void PopulateHookExclusionListsBlackMesa();
uint32_t GetCBaseEntityBlackMesa(uint32_t EHandle);

void CheckForLocation();

#endif