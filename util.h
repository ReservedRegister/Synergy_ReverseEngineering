#ifndef UTIL_H
#define UTIL_H

#include <math.h>

typedef enum {
    SYNERGY,
    BLACK_MESA
} Game;

class HooksUtil
{
public:
	static uint32_t EmptyCall();
	static uint32_t CallocHook(uint32_t nitems, uint32_t size);
	static uint32_t MallocHookSmall(uint32_t size);
	static uint32_t MallocHookLarge(uint32_t size);
	static uint32_t OperatorNewHook(uint32_t size);
	static uint32_t OperatorNewArrayHook(uint32_t size);
	static uint32_t ReallocHook(uint32_t old_ptr, uint32_t new_size);
	static uint32_t CreateEntityByNameHook(uint32_t arg0, uint32_t arg1);
	static uint32_t PhysSimEnt(uint32_t arg0);
	static uint32_t HookInstaKill(uint32_t arg0);
	static uint32_t UTIL_RemoveHookFailsafe(uint32_t arg0);
	static uint32_t UTIL_RemoveBaseHook(uint32_t arg0);
	static uint32_t AcceptInputHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
	static uint32_t UpdateOnRemove(uint32_t arg0);
	static uint32_t VPhysicsSetObjectHook(uint32_t arg0, uint32_t arg1);
	static uint32_t CollisionRulesChangedHook(uint32_t arg0);
	static uint32_t GlobalEntityListClear(uint32_t arg0);
	static uint32_t CanSatisfyVpkCacheInternalHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);
	static uint32_t PackedStoreDestructorHook(uint32_t arg0);
	static uint32_t PlayerSpawnHook(uint32_t arg0);
};

typedef uint32_t (*pZeroArgProt)();
typedef uint32_t (*pOneArgProt)(uint32_t);
typedef uint32_t (*pTwoArgProt)(uint32_t, uint32_t);
typedef uint32_t (*pThreeArgProt)(uint32_t, uint32_t, uint32_t);
typedef uint32_t (*pFourArgProt)(uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*pFiveArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*pSixArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*pSevenArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*pNineArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef uint32_t (*pElevenArgProt)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

typedef uint32_t (__attribute__((regparm(2))) *pTwoArgProtOptLink)(uint32_t, uint32_t);
typedef uint32_t (__attribute__((fastcall)) *pOneArgProtFastCall)(uint32_t);
typedef uint32_t (__attribute__((fastcall)) *pTwoArgProtFastCall)(uint32_t, uint32_t);

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
} game_offsets;

typedef struct _game_functions {
	pOneArgProt SpawnPlayer;
	pOneArgProt RemoveNormalDirect;
	pOneArgProt RemoveNormal;
	pOneArgProt RemoveInsta;
    pOneArgProt GetCBaseEntity;
	pTwoArgProt SetSolidFlags;
	pTwoArgProt DisableEntityCollisions;
	pTwoArgProt EnableEntityCollisions;
	pOneArgProt CollisionRulesChanged;
	pThreeArgProt FindEntityByClassname;
	pOneArgProt CleanupDeleteList;
	pTwoArgProt CreateEntityByName;
	pOneArgProt PhysSimEnt;
	pSixArgProt AcceptInput;
	pOneArgProt UpdateOnRemoveBase;
	pOneArgProt VphysicsSetObject;
	pOneArgProt ClearAllEntities;
	pOneArgProt PackedStoreDestructor;
	pSevenArgProt CanSatisfyVpkCacheInternal;
} game_functions;

typedef struct _Vector {
	float x = 0;
	float y = 0;
	float z = 0;
} Vector;

typedef struct _Library {
	void* library_linkmap;
	char* library_signature;
	uint32_t library_base_address;
	uint32_t library_size;
} Library;

typedef struct _Value {
	void* value;
	struct _Value* nextVal;
} Value;

typedef Value** ValueList;

typedef struct _EntityKV {
	uint32_t entityRef;
	uint32_t key;
	uint32_t value;
} EntityKV;

typedef struct _VpkMemoryLeak {
	uint32_t packed_ref;
	ValueList leaked_refs;
} VpkMemoryLeak;

typedef struct _EntityFrameCount {
	uint32_t entity_ref;
	int frames;
} EntityFrameCount;

typedef struct _EntityOrigin {
	uint32_t refHandle;
	float x;
	float y;
	float z;
} EntityOrigin;

extern Game game;

extern game_fields fields;
extern game_offsets offsets;
extern game_functions functions;

extern bool loaded_extension;

extern bool firstplayer_hasjoined;
extern bool player_collision_rules_changed;
extern bool player_worldspawn_collision_disabled;

extern uint32_t hook_exclude_list_offset[512];
extern uint32_t hook_exclude_list_base[512];
extern uint32_t memory_prots_save_list[512];
extern uint32_t our_libraries[512];
extern uint32_t loaded_libraries[512];
extern uint32_t collisions_entity_list[512];

extern uint32_t engine_srv;
extern uint32_t dedicated_srv;
extern uint32_t vphysics_srv;
extern uint32_t server_srv;
extern uint32_t server;
extern uint32_t sdktools;

extern uint32_t engine_srv_size;
extern uint32_t dedicated_srv_size;
extern uint32_t vphysics_srv_size;
extern uint32_t server_size;
extern uint32_t server_srv_size;
extern uint32_t sdktools_size;

extern bool isTicking;
extern bool server_sleeping;
extern uint32_t global_vpk_cache_buffer;
extern uint32_t current_vpk_buffer_ref;
extern ValueList leakedResourcesVpkSystem;

void InitUtil();
void* copy_val(void* val, size_t copy_size);
bool IsAddressExcluded(uint32_t base_address, uint32_t search_address);
void HookFunction(uint32_t base_address, uint32_t size, void* target_pointer, void* hook_pointer);
Library* FindLibrary(char* lib_name, bool less_intense_search);
Library* LoadLibrary(char* library_full_path);
void ClearLoadedLibraries();
Library* getlibrary(char* file_line);
void AllowWriteToMappedMemory();
void ForceMemoryAccess();
void RestoreMemoryProtections();
void ZeroVector(uint32_t vector);
bool IsVectorNaN(uint32_t base);
void UpdateAllCollisions();
void RemoveBadEnts();
void RemoveEntityNormal(uint32_t entity_object, bool validate);
void InstaKill(uint32_t entity_object, bool validate);
bool IsMarkedForDeletion(uint32_t arg0);
bool IsEntityPositionReasonable(uint32_t v);
uint32_t IsEntityValid(uint32_t entity);
void LogVpkMemoryLeaks();
void FixPlayerCollisionGroup();
void HookFunctionsUtil();
void SpawnPlayers();

ValueList AllocateValuesList();
Value* CreateNewValue(void* valueInput);
int DeleteAllValuesInList(ValueList list, bool free_val, pthread_mutex_t* lockInput);
bool IsInValuesList(ValueList list, void* searchVal, pthread_mutex_t* lockInput);
bool RemoveFromValuesList(ValueList list, void* searchVal, pthread_mutex_t* lockInput);
int ValueListItems(ValueList list, pthread_mutex_t* lockInput);
bool InsertToValuesList(ValueList list, Value* head, pthread_mutex_t* lockInput, bool tail, bool duplicate_chk);
EntityKV* CreateNewEntityKV(uint32_t refHandle, uint32_t keyIn, uint32_t valueIn);
void InsertEntityToCollisionsList(uint32_t ent);
void DisablePlayerCollisions();
void DisablePlayerWorldSpawnCollision();
bool FixSlashes(char* string);
bool IsValidVector(uint32_t base);
bool is_denormalized(float value);
bool is_negative_zero(float value);

#endif