#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <windows.h>

typedef uint32_t(*pZeroArgProt)();
typedef uint32_t(*pOneArgProt)(uint32_t);
typedef uint32_t(*pTwoArgProt)(uint32_t, uint32_t);
typedef uint32_t(*pThreeArgProt)(uint32_t, uint32_t, uint32_t);
typedef uint32_t(*pFourArgProt)(uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t(*pFiveArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t(*pSixArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t(*pSevenArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t(*pEightArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t(*pNineArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t(*pElevenArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

typedef struct _game_fields {
	uint32_t CGlobalEntityList;
	uint32_t sv;
	uint32_t RemoveImmediateSemaphore;
} game_fields;

typedef struct _game_offsets {
	uint32_t classname_offset;
	uint32_t abs_origin_offset;
	uint32_t abs_angles_offset;
	uint32_t abs_velocity_offset;
	uint32_t origin_offset;
	uint32_t refhandle_offset;
	uint32_t iserver_offset;
	uint32_t mnetwork_offset;
	uint32_t collision_property_offset;
	uint32_t m_CollisionGroup_offset;
	uint32_t ismarked_offset;
	uint32_t vphysics_object_offset;
	uint32_t entity_release_offset;
	uint32_t updateonremove_offset;
} game_offsets;

typedef struct _game_functions {
	pTwoArgProt RemoveEntityNormal;
	pTwoArgProt InstaKill;
	pOneArgProt GetCBaseEntity;
	pTwoArgProt SetSolidFlags;
	pTwoArgProt DisableEntityCollisions;
	pTwoArgProt EnableEntityCollisions;
	pOneArgProt CollisionRulesChanged;
	pThreeArgProt FindEntityByClassname;
	pOneArgProt CleanupDeleteList;
} game_functions;

typedef struct _Vector {
	float x = 0;
	float y = 0;
	float z = 0;
} Vector;

typedef struct _Value {
	void* value;
	struct _Value* nextVal;
} Value;

typedef Value** ValueList;

typedef struct _VpkMemoryLeak {
	uint32_t packed_ref;
	ValueList leaked_refs;
} VpkMemoryLeak;

typedef struct _MemoryPageSnap {
	LPVOID base_address;
	SIZE_T region_size;
	DWORD prot_flags;
	struct _MemoryPageSnap* next_page;
} MemoryPageSnap;

typedef struct _ModuleMemory {
	HMODULE module_base;
	SIZE_T module_size;
	char module_name[MAX_PATH];
	uint32_t dll_load_base;
	uint32_t dll_rva;
	MemoryPageSnap* pages;
} ModuleMemory;

extern game_fields fields;
extern game_offsets offsets;
extern game_functions functions;

extern ModuleMemory* modules[512];

extern bool loaded_extension;
extern bool firstplayer_hasjoined;
extern bool player_collision_rules_changed;
extern bool player_worldspawn_collision_disabled;

extern int hooked_delete_counter;
extern int normal_delete_counter;

extern uint32_t hook_exclude_list_offset[512];
extern uint32_t hook_exclude_list_base[512];

extern ModuleMemory* server_dll;
extern ModuleMemory* engine_dll;

extern SIZE_T server_dll_size;
extern SIZE_T engine_dll_size;

extern uint32_t collisions_entity_list[512];

void InitUtil();
void PrintModuleMemoryToScreen(ModuleMemory* module_memory);
void AllowWriteToModules();
void AllowWriteToModule(ModuleMemory* module_memory);
void RestoreModulesMemory();
void RestoreModuleMemory(ModuleMemory* module_memory);
void ClearLoadedModules();
ModuleMemory* FindModule(const char* module_signature);
uint32_t FILEOFF(ModuleMemory* memory_module, uint32_t module_offset);
ModuleMemory* ReadModuleMemory(HMODULE hModule);
ModuleMemory* AllocateModuleMemoryList(HMODULE module_in);
MemoryPageSnap* CreateMemoryPage(LPVOID base_address_in, SIZE_T region_size_in, DWORD prot_flags_in);
void InsertMemoryPageToList(ModuleMemory* pages_list, MemoryPageSnap* page_in);
void* copy_val(void* val, size_t copy_size);
bool IsAddressExcluded(uint32_t base_address, uint32_t search_address);
void HookFunction(uint32_t base_address, uint32_t size, void* target_pointer, void* hook_pointer);
bool IsMarkedForDeletion(uint32_t arg0);
void ZeroVector(uint32_t vector);
bool IsVectorNaN(uint32_t base);
bool IsEntityPositionReasonable(uint32_t v);
void InsertEntityToCollisionsList(uint32_t ent);
void UpdateAllCollisions();
void FixPlayerCollisionGroup();
void DisablePlayerWorldSpawnCollision();
void DisablePlayerCollisions();
void RemoveBadEnts();
uint32_t IsEntityValid(uint32_t entity);
ValueList AllocateValuesList();
Value* CreateNewValue(void* valueInput);
int DeleteAllValuesInList(ValueList list, bool free_val, CRITICAL_SECTION* lockInput);
bool IsInValuesList(ValueList list, void* searchVal, CRITICAL_SECTION* lockInput);
bool RemoveFromValuesList(ValueList list, void* searchVal, CRITICAL_SECTION* lockInput);
int ValueListItems(ValueList list, CRITICAL_SECTION* lockInput);
bool InsertToValuesList(ValueList list, Value* head, CRITICAL_SECTION* lockInput, bool tail, bool duplicate_chk);

#endif