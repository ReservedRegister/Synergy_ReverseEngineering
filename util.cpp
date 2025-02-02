#include "extension.h"
#include "util.h"

#include <link.h>
#include <sys/mman.h>

Game game;

game_fields fields;
game_offsets offsets;
game_functions functions;

bool loaded_extension;
bool toggle_frame;
bool firstplayer_hasjoined;
bool player_collision_rules_changed;
bool player_worldspawn_collision_disabled;

uint32_t hook_exclude_list_offset[512] = {};
uint32_t hook_exclude_list_base[512] = {};
uint32_t memory_prots_save_list[512] = {};
uint32_t our_libraries[512] = {};
uint32_t loaded_libraries[512] = {};
uint32_t collisions_entity_list[512] = {};

uint32_t engine_srv;
uint32_t dedicated_srv;
uint32_t vphysics_srv;
uint32_t server_srv;
uint32_t server;
uint32_t sdktools;

uint32_t engine_srv_size;
uint32_t dedicated_srv_size;
uint32_t vphysics_srv_size;
uint32_t server_srv_size;
uint32_t server_size;
uint32_t sdktools_size;

bool isTicking;
bool server_sleeping;
uint32_t global_vpk_cache_buffer;
uint32_t current_vpk_buffer_ref;

ValueList leakedResourcesVpkSystem;
ValueList player_spawn_list;
ValueList game_search_paths;

void InitUtil()
{
    loaded_extension = false;
    toggle_frame = true;
    firstplayer_hasjoined = false;
    player_collision_rules_changed = false;
    player_worldspawn_collision_disabled = false;
    isTicking = false;
    server_sleeping = false;
    global_vpk_cache_buffer = (uint32_t)malloc(0x00100000);
    current_vpk_buffer_ref = 0;
    player_spawn_list = AllocateValuesList();
    game_search_paths = AllocateValuesList();

    HookFunctionsUtil();
}

void HookFunctionsUtil()
{
    HookFunction(engine_srv, engine_srv_size, (void*)(functions.SV_ReplicateConVarChange), (void*)HooksUtil::SV_ReplicateConVarChangeHook);

    HookFunction(server_srv, server_srv_size, (void*)(functions.CreateEntityByName), (void*)HooksUtil::CreateEntityByNameHook);
    HookFunction(server_srv, server_srv_size, (void*)(functions.RemoveNormalDirect), (void*)HooksUtil::UTIL_RemoveHookFailsafe);
    HookFunction(server_srv, server_srv_size, (void*)(functions.RemoveNormal), (void*)HooksUtil::UTIL_RemoveBaseHook);
    HookFunction(server_srv, server_srv_size, (void*)(functions.RemoveInsta), (void*)HooksUtil::HookInstaKill);
    HookFunction(server_srv, server_srv_size, (void*)(functions.PhysSimEnt), (void*)HooksUtil::PhysSimEnt);
    HookFunction(server_srv, server_srv_size, (void*)(functions.AcceptInput), (void*)HooksUtil::AcceptInputHook);
    HookFunction(server_srv, server_srv_size, (void*)(functions.UpdateOnRemoveBase), (void*)HooksUtil::UpdateOnRemove);
    HookFunction(server_srv, server_srv_size, (void*)(functions.VphysicsSetObject), (void*)HooksUtil::VPhysicsSetObjectHook);
    HookFunction(server_srv, server_srv_size, (void*)(functions.CollisionRulesChanged), (void*)HooksUtil::CollisionRulesChangedHook);
    HookFunction(server_srv, server_srv_size, (void*)(functions.ClearAllEntities), (void*)HooksUtil::GlobalEntityListClear);
    HookFunction(server_srv, server_srv_size, (void*)(functions.SpawnPlayer), (void*)HooksUtil::PlayerSpawnHook);
    HookFunction(server_srv, server_srv_size, (void*)malloc, (void*)HooksUtil::MallocHookSmall);

    HookFunction(dedicated_srv, dedicated_srv_size, (void*)(functions.PackedStoreDestructor), (void*)HooksUtil::PackedStoreDestructorHook);
    HookFunction(dedicated_srv, dedicated_srv_size, (void*)(functions.CanSatisfyVpkCacheInternal), (void*)HooksUtil::CanSatisfyVpkCacheInternalHook);
    HookFunction(dedicated_srv, dedicated_srv_size, (void*)(functions.AddSearchPath), (void*)HooksUtil::AddSearchPathHook);
    HookFunction(dedicated_srv, dedicated_srv_size, (void*)malloc, (void*)HooksUtil::MallocHookLarge);

    //Manual Memory Block Function Hooking (inlined function hooking)
    if(game == SYNERGY)
    {
        Signature start_signatures[32];
        Signature end_signatures[32];

        Signature args_signatures[32];
        Signature stack_machine_code[32];

        Signature no_operation_signatures[32];

        Signature start_signature_esi;
        uint8_t start_signature_preload_esi[512] = {0x83, 0xBE, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(start_signature_esi.signature, start_signature_preload_esi, sizeof(start_signature_preload_esi));
        start_signature_esi.signature_size = 7;

        Signature start_signature_edi;
        uint8_t start_signature_preload_edi[512] = {0x83, 0xBF, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(start_signature_edi.signature, start_signature_preload_edi, sizeof(start_signature_preload_edi));
        start_signature_edi.signature_size = 7;

        Signature start_signature_eax;
        uint8_t start_signature_preload_eax[512] = {0x83, 0xB8, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(start_signature_eax.signature, start_signature_preload_eax, sizeof(start_signature_preload_eax));
        start_signature_eax.signature_size = 7;

        Signature start_signature_ecx;
        uint8_t start_signature_preload_ecx[512] = {0x83, 0xB9, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(start_signature_ecx.signature, start_signature_preload_ecx, sizeof(start_signature_preload_ecx));
        start_signature_ecx.signature_size = 7;

        Signature start_signature_edx;
        uint8_t start_signature_preload_edx[512] = {0x83, 0xBA, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(start_signature_edx.signature, start_signature_preload_edx, sizeof(start_signature_preload_edx));
        start_signature_edx.signature_size = 7;

        Signature start_signature_ebx;
        uint8_t start_signature_preload_ebx[512] = {0x83, 0xBB, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(start_signature_ebx.signature, start_signature_preload_ebx, sizeof(start_signature_preload_ebx));
        start_signature_ebx.signature_size = 7;


        Signature end_signature_esi;
        uint8_t end_signature_preload_esi[512] = {0xFF, 0x56, 0x6C, 0xEB, 0xE6};
        memcpy(end_signature_esi.signature, end_signature_preload_esi, sizeof(end_signature_preload_esi));
        end_signature_esi.signature_size = 5;

        Signature end_signature_edi;
        uint8_t end_signature_preload_edi[512] = {0xFF, 0x57, 0x6C, 0xEB, 0xE6};
        memcpy(end_signature_edi.signature, end_signature_preload_edi, sizeof(end_signature_preload_edi));
        end_signature_edi.signature_size = 5;

        Signature end_signature_eax;
        uint8_t end_signature_preload_eax[512] = {0xFF, 0x50, 0x6C, 0xEB, 0xE6};
        memcpy(end_signature_eax.signature, end_signature_preload_eax, sizeof(end_signature_preload_eax));
        end_signature_eax.signature_size = 5;

        Signature end_signature_ecx;
        uint8_t end_signature_preload_ecx[512] = {0xFF, 0x51, 0x6C, 0xEB, 0xE6};
        memcpy(end_signature_ecx.signature, end_signature_preload_ecx, sizeof(end_signature_preload_ecx));
        end_signature_ecx.signature_size = 5;

        Signature end_signature_edx;
        uint8_t end_signature_preload_edx[512] = {0xFF, 0x52, 0x6C, 0xEB, 0xE6};
        memcpy(end_signature_edx.signature, end_signature_preload_edx, sizeof(end_signature_preload_edx));
        end_signature_edx.signature_size = 5;

        Signature end_signature_ebx;
        uint8_t end_signature_preload_ebx[512] = {0xFF, 0x53, 0x6C, 0xEB, 0xE6};
        memcpy(end_signature_ebx.signature, end_signature_preload_ebx, sizeof(end_signature_preload_ebx));
        end_signature_ebx.signature_size = 5;


        Signature argument_signature_esi;
        uint8_t argument_signature_preload_esi[512] = {0x83, 0xBE, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(argument_signature_esi.signature, argument_signature_preload_esi, sizeof(argument_signature_preload_esi));
        argument_signature_esi.signature_size = 7;

        Signature argument_signature_edi;
        uint8_t argument_signature_preload_edi[512] = {0x83, 0xBF, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(argument_signature_edi.signature, argument_signature_preload_edi, sizeof(argument_signature_preload_edi));
        argument_signature_edi.signature_size = 7;

        Signature argument_signature_eax;
        uint8_t argument_signature_preload_eax[512] = {0x83, 0xB8, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(argument_signature_eax.signature, argument_signature_preload_eax, sizeof(argument_signature_preload_eax));
        argument_signature_eax.signature_size = 7;

        Signature argument_signature_ecx;
        uint8_t argument_signature_preload_ecx[512] = {0x83, 0xB9, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(argument_signature_ecx.signature, argument_signature_preload_ecx, sizeof(argument_signature_preload_ecx));
        argument_signature_ecx.signature_size = 7;

        Signature argument_signature_edx;
        uint8_t argument_signature_preload_edx[512] = {0x83, 0xBA, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(argument_signature_edx.signature, argument_signature_preload_edx, sizeof(argument_signature_preload_edx));
        argument_signature_edx.signature_size = 7;

        Signature argument_signature_ebx;
        uint8_t argument_signature_preload_ebx[512] = {0x83, 0xBB, 0x08, 0x02, 0x00, 0x00, 0x00};
        memcpy(argument_signature_ebx.signature, argument_signature_preload_ebx, sizeof(argument_signature_preload_ebx));
        argument_signature_ebx.signature_size = 7;


        Signature stack_signature_esi;
        uint8_t stack_signature_preload_esi[512] = {0x89, 0x34, 0x24};
        memcpy(stack_signature_esi.signature, stack_signature_preload_esi, sizeof(stack_signature_preload_esi));
        stack_signature_esi.signature_size = 3;

        Signature stack_signature_edi;
        uint8_t stack_signature_preload_edi[512] = {0x89, 0x3C, 0x24};
        memcpy(stack_signature_edi.signature, stack_signature_preload_edi, sizeof(stack_signature_preload_edi));
        stack_signature_edi.signature_size = 3;

        Signature stack_signature_eax;
        uint8_t stack_signature_preload_eax[512] = {0x89, 0x04, 0x24};
        memcpy(stack_signature_eax.signature, stack_signature_preload_eax, sizeof(stack_signature_preload_eax));
        stack_signature_eax.signature_size = 3;

        Signature stack_signature_ecx;
        uint8_t stack_signature_preload_ecx[512] = {0x89, 0x0C, 0x24};
        memcpy(stack_signature_ecx.signature, stack_signature_preload_ecx, sizeof(stack_signature_preload_ecx));
        stack_signature_ecx.signature_size = 3;

        Signature stack_signature_edx;
        uint8_t stack_signature_preload_edx[512] = {0x89, 0x14, 0x24};
        memcpy(stack_signature_edx.signature, stack_signature_preload_edx, sizeof(stack_signature_preload_edx));
        stack_signature_edx.signature_size = 3;

        Signature stack_signature_ebx;
        uint8_t stack_signature_preload_ebx[512] = {0x89, 0x1C, 0x24};
        memcpy(stack_signature_ebx.signature, stack_signature_preload_ebx, sizeof(stack_signature_preload_ebx));
        stack_signature_ebx.signature_size = 3;


        Signature no_operation_signature_esi;
        uint8_t no_operation_signature_preload_esi[512] = {0xFF, 0x96, 0x90, 0x00, 0x00, 0x00};
        memcpy(no_operation_signature_esi.signature, no_operation_signature_preload_esi, sizeof(no_operation_signature_preload_esi));
        no_operation_signature_esi.signature_size = 6;

        Signature no_operation_signature_edi;
        uint8_t no_operation_signature_preload_edi[512] = {0xFF, 0x97, 0x90, 0x00, 0x00, 0x00};
        memcpy(no_operation_signature_edi.signature, no_operation_signature_preload_edi, sizeof(no_operation_signature_preload_edi));
        no_operation_signature_edi.signature_size = 6;

        Signature no_operation_signature_eax;
        uint8_t no_operation_signature_preload_eax[512] = {0xFF, 0x90, 0x90, 0x00, 0x00, 0x00};
        memcpy(no_operation_signature_eax.signature, no_operation_signature_preload_eax, sizeof(no_operation_signature_preload_eax));
        no_operation_signature_eax.signature_size = 6;

        Signature no_operation_signature_ecx;
        uint8_t no_operation_signature_preload_ecx[512] = {0xFF, 0x91, 0x90, 0x00, 0x00, 0x00};
        memcpy(no_operation_signature_ecx.signature, no_operation_signature_preload_ecx, sizeof(no_operation_signature_preload_ecx));
        no_operation_signature_ecx.signature_size = 6;

        Signature no_operation_signature_edx;
        uint8_t no_operation_signature_preload_edx[512] = {0xFF, 0x92, 0x90, 0x00, 0x00, 0x00};
        memcpy(no_operation_signature_edx.signature, no_operation_signature_preload_edx, sizeof(no_operation_signature_preload_edx));
        no_operation_signature_edx.signature_size = 6;

        Signature no_operation_signature_ebx;
        uint8_t no_operation_signature_preload_ebx[512] = {0xFF, 0x93, 0x90, 0x00, 0x00, 0x00};
        memcpy(no_operation_signature_ebx.signature, no_operation_signature_preload_ebx, sizeof(no_operation_signature_preload_ebx));
        no_operation_signature_ebx.signature_size = 6;


        start_signatures[0] = start_signature_esi;
        start_signatures[1] = start_signature_edi;
        start_signatures[2] = start_signature_eax;
        start_signatures[3] = start_signature_ecx;
        start_signatures[4] = start_signature_edx;
        start_signatures[5] = start_signature_ebx;

        end_signatures[0] = end_signature_esi;
        end_signatures[1] = end_signature_edi;
        end_signatures[2] = end_signature_eax;
        end_signatures[3] = end_signature_ecx;
        end_signatures[4] = end_signature_edx;
        end_signatures[5] = end_signature_ebx;

        args_signatures[0] = argument_signature_esi;
        args_signatures[1] = argument_signature_edi;
        args_signatures[2] = argument_signature_eax;
        args_signatures[3] = argument_signature_ecx;
        args_signatures[4] = argument_signature_edx;
        args_signatures[5] = argument_signature_ebx;

        stack_machine_code[0] = stack_signature_esi;
        stack_machine_code[1] = stack_signature_edi;
        stack_machine_code[2] = stack_signature_eax;
        stack_machine_code[3] = stack_signature_ecx;
        stack_machine_code[4] = stack_signature_edx;
        stack_machine_code[5] = stack_signature_ebx;

        no_operation_signatures[0] = no_operation_signature_esi;
        no_operation_signatures[1] = no_operation_signature_edi;
        no_operation_signatures[2] = no_operation_signature_eax;
        no_operation_signatures[3] = no_operation_signature_ecx;
        no_operation_signatures[4] = no_operation_signature_edx;
        no_operation_signatures[5] = no_operation_signature_ebx;

        HookMemoryBlock(server_srv, server_srv_size, start_signatures, 6, end_signatures, 6, args_signatures, stack_machine_code, 6, no_operation_signatures, 6, 100, 150, 1, (void*)HooksUtil::CollisionRulesChangedHook);
    }
}

void CorrectPhysics()
{
    uint8_t deferMindist = *(uint8_t*)(fields.deferMindist);

    if(deferMindist)
    {
        rootconsole->ConsolePrint("Warning defer mindist was set! Physics might break!");
    }

    *(uint8_t*)(fields.deferMindist) = 0;
}

bool FixSlashes(char* string)
{
    bool fixed_name = false;

    if(string == NULL)
        return fixed_name;

    for(int i = 0; i <= (int)strlen((char*)string); i++)
    {
        char byte = (char)(*(uint8_t*)(string+i));

        if(byte == '\\')
        {
            *(uint8_t*)(string+i) = (uint8_t)'/';
            fixed_name = true;
        }
    }

    return fixed_name;
}

void ReplicateCheatsOnClient()
{
    functions.SV_ReplicateConVarChange(fields.sv_cheats_cvar, (uint32_t)"1");
}

void SpawnPlayers()
{
    pThreeArgProt pDynamicThreeArgFunc;
    Value* first_player = *player_spawn_list;

    while(first_player)
    {
        uint32_t player = functions.GetCBaseEntity((uint32_t)first_player->value);

        if(IsEntityValid(player))
        {
            functions.CleanupDeleteList(0);

            functions.SpawnPlayer(player);

            functions.CleanupDeleteList(0);

            firstplayer_hasjoined = true;
            player_worldspawn_collision_disabled = false;
        }

        Value* next_player = first_player->nextVal;

        free(first_player);
        first_player = next_player;
    }

    *player_spawn_list = NULL;
}

uint32_t HooksUtil::CallocHook(uint32_t nitems, uint32_t size)
{
    if(nitems <= 0) return (uint32_t)calloc(nitems, size);

    uint32_t enlarged_size = nitems*2.5;
    return (uint32_t)calloc(enlarged_size, size);
}

uint32_t HooksUtil::MallocHookSmall(uint32_t size)
{
    if(size <= 0) return (uint32_t)malloc(size);
    
    return (uint32_t)malloc(size*1.3);
}

uint32_t HooksUtil::MallocHookLarge(uint32_t size)
{
    if(size <= 0) return (uint32_t)malloc(size);

    return (uint32_t)malloc(size*3.0);
}

uint32_t HooksUtil::OperatorNewHook(uint32_t size)
{
    if(size <= 0) return (uint32_t)operator new(size);

    return (uint32_t)operator new(size*1.4);
}

uint32_t HooksUtil::OperatorNewArrayHook(uint32_t size)
{
    if(size <= 0) return (uint32_t)operator new[](size);

    return (uint32_t)operator new[](size*3.0);
}

uint32_t HooksUtil::ReallocHook(uint32_t old_ptr, uint32_t new_size)
{
    if(new_size <= 0) return (uint32_t)realloc((void*)old_ptr, new_size);

    return (uint32_t)realloc((void*)old_ptr, new_size*1.2);
}

uint32_t HooksUtil::EmptyCall()
{
    return 0;
}

uint32_t HooksUtil::SV_ReplicateConVarChangeHook(uint32_t arg0, uint32_t arg1)
{
    pTwoArgProt pDynamicTwoArgFunc;

    if(arg0 == fields.sv_cheats_cvar)
        arg1 = (uint32_t)"1";

    pDynamicTwoArgFunc = (pTwoArgProt)(functions.SV_ReplicateConVarChange);
    return pDynamicTwoArgFunc(arg0, arg1);
}

uint32_t HooksUtil::AddSearchPathHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    pFourArgProt pDynamicFourArgFunc;

    if(strcmp((char*)arg2, "GAME") == 0)
    {
        if(FindStringInList(game_search_paths, (const char*)arg1, NULL, false, NULL) == NULL)
        {
            void* search_path_copy = copy_val((void*)arg1, strlen((char*)arg1)+1);

            uint32_t head_tail_copy = (uint32_t)malloc(sizeof(uint32_t)+1);
            *(uint32_t*)(head_tail_copy) = 1;
            *(uint8_t*)(head_tail_copy+3) = 0;
            
            Value* new_search_path = CreateNewValue(search_path_copy);
            Value* head_tail = CreateNewValue((void*)head_tail_copy);

            InsertToValuesList(game_search_paths, new_search_path, NULL, true, false);
            InsertToValuesList(game_search_paths, head_tail, NULL, true, false);

            rootconsole->ConsolePrint("Added search path %s %s", arg2, search_path_copy);
        }
    }

    pDynamicFourArgFunc = (pFourArgProt)(functions.AddSearchPath);
    return pDynamicFourArgFunc(arg0, arg1, arg2, arg3);
}

uint32_t HooksUtil::CanSatisfyVpkCacheInternalHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6)
{
    pSevenArgProt pDynamicSevenArgFunc;

    pDynamicSevenArgFunc = (pSevenArgProt)(functions.CanSatisfyVpkCacheInternal);
    uint32_t returnVal = pDynamicSevenArgFunc(arg0, arg1, arg2, arg3, arg4, arg5, arg6);

    if(current_vpk_buffer_ref)
    {
        uint32_t allocated_vpk_buffer = *(uint32_t*)(current_vpk_buffer_ref+0x10);

        if(allocated_vpk_buffer && global_vpk_cache_buffer == allocated_vpk_buffer)
        {
            //rootconsole->ConsolePrint("Removed global vpk buffer from VPK tree!");
            *(uint32_t*)(current_vpk_buffer_ref+0x10) = 0;
        }
        else
        {
            rootconsole->ConsolePrint("Failed to remove global vpk buffer!!!");
            exit(1);
        }

        current_vpk_buffer_ref = 0;
    }

    return returnVal;
}

uint32_t HooksUtil::PackedStoreDestructorHook(uint32_t arg0)
{
    //Remove ref to store only valid objects!
    pOneArgProt pDynamicOneArgFunc;

    pDynamicOneArgFunc = (pOneArgProt)(functions.PackedStoreDestructor);
    uint32_t returnVal = pDynamicOneArgFunc(arg0);

    Value* a_leak = *leakedResourcesVpkSystem;

    while(a_leak)
    {
        VpkMemoryLeak* the_leak = (VpkMemoryLeak*)(a_leak->value);
        uint32_t packed_object = the_leak->packed_ref;

        if(packed_object == arg0)
        {
            ValueList vpk_leak_list = the_leak->leaked_refs;
            
            int removed_items = DeleteAllValuesInList(vpk_leak_list, true, NULL);

            rootconsole->ConsolePrint("[VPK Hook] released [%d] memory leaks!", removed_items);

            bool success = RemoveFromValuesList(leakedResourcesVpkSystem, the_leak, NULL);

            free(vpk_leak_list);
            free(the_leak);

            if(!success)
            {
                rootconsole->ConsolePrint("[VPK Hook] Expected to remove leak but failed!");
                exit(EXIT_FAILURE);
            }

            return returnVal;
        }

        a_leak = a_leak->nextVal;
    }

    return returnVal;
}

uint32_t HooksUtil::PlayerSpawnHook(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;

    uint32_t refHandle = *(uint32_t*)(arg0+offsets.refhandle_offset);

    pDynamicOneArgFunc = (pOneArgProt)(functions.SpawnPlayer);
    return pDynamicOneArgFunc(arg0);
}

uint32_t HooksUtil::GlobalEntityListClear(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;

    //LogVpkMemoryLeaks();
    DeleteAllValuesInList(game_search_paths, true, NULL);

    isTicking = false;
    firstplayer_hasjoined = false;

    pDynamicOneArgFunc = (pOneArgProt)(functions.ClearAllEntities);
    return pDynamicOneArgFunc(arg0);
}

uint32_t HooksUtil::UTIL_RemoveHookFailsafe(uint32_t arg0)
{
    // THIS IS UTIL_Remove(IServerNetworable*)
    // THIS HOOK IS FOR UNUSUAL CALLS TO UTIL_Remove probably from sourcemod!

    if(arg0 == 0) return 0;
    RemoveEntityNormal(arg0-offsets.iserver_offset, true);
    return 0;
}

uint32_t HooksUtil::UpdateOnRemove(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;

    char* classname = (char*)(*(uint32_t*)(arg0+offsets.classname_offset));

    pDynamicOneArgFunc = (pOneArgProt)(functions.UpdateOnRemoveBase);
    return pDynamicOneArgFunc(arg0);
}

uint32_t HooksUtil::PhysSimEnt(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;

    if(arg0 == 0)
    {
        rootconsole->ConsolePrint("Passed NULL simulation entity!");
        exit(EXIT_FAILURE);
        return 0;
    }

    uint32_t sim_ent_ref = *(uint32_t*)(arg0+offsets.refhandle_offset);
    uint32_t object_check = functions.GetCBaseEntity(sim_ent_ref);

    if(object_check == 0)
    {
        rootconsole->ConsolePrint("Passed in non-existant simulation entity!");
        exit(EXIT_FAILURE);
        return 0;
    }

    char* clsname =  (char*) ( *(uint32_t*)(arg0+offsets.classname_offset) );

    if(IsMarkedForDeletion(arg0+offsets.iserver_offset))
    {
        rootconsole->ConsolePrint("Simulation ignored for [%s]", clsname);
        return 0;
    }

    pDynamicOneArgFunc = (pOneArgProt)(functions.PhysSimEnt);
    return pDynamicOneArgFunc(arg0);
}

uint32_t HooksUtil::CreateEntityByNameHook(uint32_t arg0, uint32_t arg1)
{
    pTwoArgProt pDynamicTwoArgFunc;

    pDynamicTwoArgFunc = (pTwoArgProt)(functions.CreateEntityByName);
    return pDynamicTwoArgFunc(arg0, arg1);
}

uint32_t HooksUtil::HookInstaKill(uint32_t arg0)
{
    InstaKill(arg0, true);
    return 0;
}

uint32_t HooksUtil::VPhysicsSetObjectHook(uint32_t arg0, uint32_t arg1)
{
    if(IsEntityValid(arg0))
    {
        uint32_t vphysics_object = *(uint32_t*)(arg0+offsets.vphysics_object_offset);

        if(vphysics_object)
        {
            rootconsole->ConsolePrint("Attempting override existing vphysics object!!!!");
            return 0;
        }

        *(uint32_t*)(arg0+offsets.vphysics_object_offset) = arg1;

        return 0;
    }

    rootconsole->ConsolePrint("Entity was invalid failed to set vphysics object!");
    return 0;
}

uint32_t HooksUtil::AcceptInputHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    pSixArgProt pDynamicSixArgProt;

    // CBaseEntity arg0 arg2 arg3

    bool failure = false;

    if(IsEntityValid(arg0) == 0) failure = true;

    if(failure)
    {
        //rootconsole->ConsolePrint("AcceptInput blocked on marked entity");
        return 0;
    }

    //Passed sanity check
    pDynamicSixArgProt = (pSixArgProt)(functions.AcceptInput);
    return pDynamicSixArgProt(arg0, arg1, arg2, arg3, arg4, arg5);
}

uint32_t HooksUtil::CollisionRulesChangedHook(uint32_t arg0)
{
    uint32_t vphysics_object = *(uint32_t*)(arg0+offsets.vphysics_object_offset);
    if(vphysics_object) InsertEntityToCollisionsList(arg0);
    return 0;
}

uint32_t HooksUtil::UTIL_RemoveBaseHook(uint32_t arg0)
{
    RemoveEntityNormal(arg0, true);
    return 0;
}

void LogVpkMemoryLeaks()
{
    Value* firstLeak = *leakedResourcesVpkSystem;

    int running_total_of_leaks = 0;

    while(firstLeak)
    {
        VpkMemoryLeak* the_leak = (VpkMemoryLeak*)(firstLeak->value);

        ValueList refs = the_leak->leaked_refs;
        Value* firstLeakedRef = *refs;

        int leaked_vpk_refs = 0;

        while(firstLeakedRef)
        {
            leaked_vpk_refs++;

            firstLeakedRef = firstLeakedRef->nextVal;
        }

        running_total_of_leaks = running_total_of_leaks + leaked_vpk_refs;

        rootconsole->ConsolePrint("Found [%d] leaked refs in object [%p]", leaked_vpk_refs, the_leak->packed_ref);

        firstLeak = firstLeak->nextVal;
    }

    rootconsole->ConsolePrint("Total VPK leaks [%d]", running_total_of_leaks);
}

void* copy_val(void* val, size_t copy_size)
{
    if(val == 0)
        return 0;
    
    void* copy_ptr = malloc(copy_size);
    memcpy(copy_ptr, val, copy_size);
    return copy_ptr;
}

bool IsAddressExcluded(uint32_t base_address, uint32_t search_address)
{
    for(int i = 0; i < 512; i++)
    {
        if(hook_exclude_list_offset[i] == 0 || hook_exclude_list_base[i] == 0)
            continue;

        uint32_t patch_address = base_address + hook_exclude_list_offset[i];

        if(patch_address == search_address && hook_exclude_list_base[i] == base_address)
            return true;
    }

    return false;
}

uint32_t FindSignature(uint32_t block_start_start, uint32_t block_end_end, Signature signature_main)
{
    uint32_t block_size = block_end_end - block_start_start;

    int signature_search_counter = 0;
    int block_counter = 0;
    int block_fail_offset = 0;
    uint32_t signature_found_start = 0;

    while(true)
    {
        uint8_t arg_byte = signature_main.signature[signature_search_counter];
        uint8_t block_byte = *(uint8_t*)(block_start_start+block_counter+block_fail_offset);

        if(signature_search_counter >= signature_main.signature_size)
            break;

        if(block_start_start + block_fail_offset >= block_end_end)
            break;

        if(arg_byte == block_byte)
        {
            if(block_counter == 0)
                signature_found_start = block_start_start+block_fail_offset;

            signature_search_counter++;
            block_counter++;
        }
        else
        {
            signature_found_start = 0;
            signature_search_counter = 0;
            block_counter = 0;
            block_fail_offset++;
        }
    }

    return signature_found_start;
}

uint32_t ApplyNoOperation(uint32_t block_start_start, uint32_t block_end_end, Signature no_operation_signatures[32], int no_operation_signatures_size)
{
    for(int i = 0; i < no_operation_signatures_size; i++)
    {
        Signature no_operation_signature = no_operation_signatures[i];
        uint32_t block_start_no_operation = FindSignature(block_start_start, block_end_end, no_operation_signature);

        if(block_start_no_operation)
        {
            memset((void*)block_start_no_operation, 0x90, block_end_end - block_start_no_operation);
            return block_start_no_operation;
        }
    }

    return 0;
}

bool ApplyBlockHook(uint32_t block_start_start, uint32_t block_end_end, Signature args_signatures[32], Signature stack_machine_code[32], int stack_arguments_size, Signature no_operation_signatures[32], int no_operation_signatures_size, int expected_arguments, void* hook_pointer)
{
    uint32_t block_size = block_end_end - block_start_start;
    int actual_arguments_found = 0;

    for(int i = 0; i < stack_arguments_size; i++)
    {
        Signature argument_signature = args_signatures[i];
        Signature stack_signature = stack_machine_code[i];

        if(FindSignature(block_start_start, block_end_end, argument_signature))
        {
            uint32_t block_start_no_operation = ApplyNoOperation(block_start_start, block_end_end, no_operation_signatures, no_operation_signatures_size);

            if(block_start_no_operation)
            {
                rootconsole->ConsolePrint("Found the argument signature! %d", block_size);

                //SET STACK MEMORY FROM ARRAY USING CORRECT INDEX
                //APPLY HOOK POINTER

                for(int k = 0; k < stack_signature.signature_size; k++)
                    *(uint8_t*)(block_start_no_operation+k) = stack_signature.signature[k];

                uint32_t hook_block_hook_pointer_base = block_start_no_operation+stack_signature.signature_size;
                uint32_t offset = (uint32_t)hook_pointer - hook_block_hook_pointer_base - 5;

                *(uint8_t*)(hook_block_hook_pointer_base) = 0xE8;
                *(uint32_t*)(hook_block_hook_pointer_base+1) = offset;

                actual_arguments_found++;
            }
        }
        else
        {
            rootconsole->ConsolePrint("Failed to find argument signature %d", i);
        }
    }

    return actual_arguments_found == expected_arguments;
}

void HookMemoryBlock(uint32_t base_address, uint32_t size, Signature start_signatures[32], int start_signatures_size, Signature end_signatures[32], int end_signatures_size, Signature args_signatures[32], Signature stack_machine_code[32], int stack_arguments_size, Signature no_operation_signatures[32], int no_operation_signatures_size, uint32_t estimated_block_size_min, uint32_t estimated_block_size_max, int expected_arguments, void* hook_pointer)
{
    uint32_t search_address = base_address;
    uint32_t search_address_max = base_address+size;

    uint32_t block_start_start = 0;
    uint32_t block_end_start = 0;

    int start_signature_signature_counter = 0;
    int end_signature_signature_counter = 0;

    int signature_found_counter = 0;
    bool found_something = false;
    bool found_start_signature = false;

    while(true)
    {
        if(!found_start_signature)
        {
            if(search_address > search_address_max)
            {
                search_address = base_address;
                start_signature_signature_counter++;

                signature_found_counter = 0;
                found_something = false;
                block_start_start = 0;
            }

            if(start_signature_signature_counter >= start_signatures_size)
            {
                rootconsole->ConsolePrint("No more start signatures left!");
                break;
            }

            // Handle whenever the signature was found successfully

            if(signature_found_counter >= start_signatures[start_signature_signature_counter].signature_size)
            {
                if(IsAddressExcluded(base_address, block_start_start))
                {
                    rootconsole->ConsolePrint("Skipped block hook at [%X]", block_start_start);

                    search_address = block_start_start+1;
                    found_something = false;
                    signature_found_counter = 0;
                    continue;
                }

                signature_found_counter = 0;
                found_start_signature = true;
                found_something = false;
                continue;
            }

            // Handle finding the byte in the signature

            if(*(uint8_t*)(search_address) == start_signatures[start_signature_signature_counter].signature[signature_found_counter])
            {
                // Move on to the next byte since we found this one

                if(signature_found_counter == 0)
                    block_start_start = search_address;

                signature_found_counter++;
                found_something = true;
            }
            else
            {
                // Reset the counter since we did not find the signature

                if(found_something)
                {
                    search_address = block_start_start+1;
                    found_something = false;
                    signature_found_counter = 0;
                    continue;
                }

                signature_found_counter = 0;
            }
        }
        else
        {
            if((search_address > search_address_max) || (search_address > block_start_start+estimated_block_size_max))
            {
                search_address = block_start_start;
                end_signature_signature_counter++;

                found_something = false;
                block_end_start = 0;
                signature_found_counter = 0;
            }

            // Handle whenever it runs out of end signatures to check
            if(end_signature_signature_counter >= end_signatures_size)
            {
                //rootconsole->ConsolePrint("No more end signatures left!");

                next_block:

                search_address = block_start_start+1;

                end_signature_signature_counter = 0;
                
                block_start_start = 0;
                block_end_start = 0;

                signature_found_counter = 0;
                found_something = false;
                found_start_signature = false;
                continue;
            }

            //Handle whenever the signature was found successfully

            if(signature_found_counter >= end_signatures[end_signature_signature_counter].signature_size)
            {
                // Apply the hook

                uint32_t block_size = search_address - block_start_start;

                //rootconsole->ConsolePrint("\nFound signature base and end %p %p", block_start_start, search_address);

                if(!(block_size >= estimated_block_size_min && block_size <= estimated_block_size_max))
                {
                    //rootconsole->ConsolePrint("Out of bounds block size real [%d] min [%d] max [%d]", block_size, estimated_block_size_min, estimated_block_size_max);
                    goto found_nothing;
                }

                if(ApplyBlockHook(block_start_start, search_address, args_signatures, stack_machine_code, stack_arguments_size, no_operation_signatures, no_operation_signatures_size, expected_arguments, hook_pointer))
                {
                    rootconsole->ConsolePrint("\nFound signature base and end %p %p", block_start_start, search_address);
                    goto next_block;
                }

                goto found_nothing;
            }

            // Handle finding the byte in the signature

            if(*(uint8_t*)(search_address) == end_signatures[end_signature_signature_counter].signature[signature_found_counter])
            {
                // Move on to the next byte since we found this one

                if(signature_found_counter == 0)
                    block_end_start = search_address;

                signature_found_counter++;
                found_something = true;
            }
            else
            {
                found_nothing:
                // Reset the counter since we did not find the signature

                if(found_something)
                {
                    search_address = block_end_start+1;
                    found_something = false;
                    signature_found_counter = 0;
                    continue;
                }

                signature_found_counter = 0;
            }
        }

        search_address++;
    }
}

void HookFunction(uint32_t base_address, uint32_t size, void* target_pointer, void* hook_pointer)
{
    if(!target_pointer || !hook_pointer)
    {
        rootconsole->ConsolePrint("Failed to hook function due to target pointer or hook pointer being NULL");
        return;
    }

    uint32_t search_address = base_address;
    uint32_t search_address_max = base_address+size;

    while(search_address + 3 <= search_address_max)
    {
        uint32_t four_byte_addr = *(uint32_t*)(search_address);

        if(four_byte_addr == (uint32_t)target_pointer)
        {
            if(IsAddressExcluded(base_address, search_address))
            {
                rootconsole->ConsolePrint("(abs) Skipped patch at [%X]", search_address);
                search_address++;
                continue;
            }

            //rootconsole->ConsolePrint("Patched abs address: [%X]", search_address);
            *(uint32_t*)(search_address) = (uint32_t)hook_pointer;
            
            search_address++;
            continue;
        }

        uint8_t byte = *(uint8_t*)(search_address);

        if(byte == 0xE8 || byte == 0xE9)
        {
            uint32_t call_address = *(uint32_t*)(search_address + 1);
            uint32_t chk = search_address + call_address + 5;

            if(chk == (uint32_t)target_pointer)
            {
                if(IsAddressExcluded(base_address, search_address))
                {
                    rootconsole->ConsolePrint("(unsigned) Skipped patch at [%X]", search_address);
                    search_address++;
                    continue;
                }

                //rootconsole->ConsolePrint("(unsigned) Hooked address: [%X]", search_address - base_address);
                uint32_t offset = (uint32_t)hook_pointer - search_address - 5;
                *(uint32_t*)(search_address+1) = offset;
            }
            else
            {
                //check signed addition
                chk = search_address + (int32_t)call_address + 5;

                if(chk == (uint32_t)target_pointer)
                {
                    if(IsAddressExcluded(base_address, search_address))
                    {
                        rootconsole->ConsolePrint("(signed) Skipped patch at [%X]", search_address);
                        search_address++;
                        continue;
                    }

                    rootconsole->ConsolePrint("(signed) Hooked address: [%X]", search_address - base_address);
                    uint32_t offset = (uint32_t)hook_pointer - search_address - 5;
                    *(uint32_t*)(search_address+1) = offset;
                }
            }
        }

        search_address++;
    }
}

Library* FindLibrary(char* lib_name, bool less_intense_search)
{
    for(int i = 0; i < 512; i++)
    {
        if(loaded_libraries[i] == 0) continue;
        Library* existing_lib = (Library*)loaded_libraries[i];
        
        if(less_intense_search)
        {
            if(strcasestr(existing_lib->library_signature, lib_name) != NULL) return existing_lib;
        }

        if(strcmp(existing_lib->library_signature, lib_name) == 0) return existing_lib;
    }

    return NULL;
}

void ClearLoadedLibraries()
{
    for(int i = 0; i < 512; i++)
    {
        if(loaded_libraries[i] != 0)
        {
            Library* delete_this = (Library*)loaded_libraries[i];

            dlclose(delete_this->library_linkmap);
            free(delete_this->library_signature);

            free(delete_this);

            loaded_libraries[i] = 0;
        }
    }
}

Library* LoadLibrary(char* library_full_path)
{
    if(library_full_path)
    {
        Library* found_lib = FindLibrary(library_full_path, false);
        if(found_lib) return found_lib;

        struct link_map* library_lm = (struct link_map*)(dlopen(library_full_path, RTLD_NOW));

        if(library_lm)
        {
            for(int i = 0; i < 512; i++)
            {
                if(loaded_libraries[i] == 0)
                {
                    Library* new_lib = (Library*)(malloc(sizeof(Library)));
                    new_lib->library_linkmap = (void*)library_lm;
                    new_lib->library_signature = (char*)copy_val(library_full_path, strlen(library_full_path)+1);
                    new_lib->library_base_address = library_lm->l_addr;
                    new_lib->library_size = 0;
                    loaded_libraries[i] = (uint32_t)new_lib;
                    
                    rootconsole->ConsolePrint("Loaded [%s]", library_full_path);
                    return new_lib;
                }
            }

            rootconsole->ConsolePrint("Failed to save library to list!");
            exit(EXIT_FAILURE);
        }
    }

    return NULL;
}

Library* getlibrary(char* file_line)
{
    for(int i = 0; i < 512; i++)
    {
        if(our_libraries[i] == 0) continue;

        char* match = strcasestr(file_line, (char*)our_libraries[i]);

        if(match)
        {
            int temp_char_reverser = 0;
            char* abs_path = NULL;

            while(abs_path == NULL)
            {
                if(*(char*)(match-temp_char_reverser) == ' ')
                {
                    if(*(char*)(match-temp_char_reverser+1) == '/')
                    {
                        abs_path = match-temp_char_reverser+1;
                    }
                }

                temp_char_reverser++;
            }

            /*char file_line_temp[512];
            snprintf(file_line_temp, 512, "%s", file_line);
            strtok(file_line_temp, " \t");
            for(int i = 0; i < 4; i++) strtok(NULL, " \t");
            char* abs_path = strtok(NULL, " \t");*/

            //rootconsole->ConsolePrint("abs [%s]", abs_path);

            Library* found_lib = LoadLibrary(abs_path);

            if(found_lib)
            {
                //rootconsole->ConsolePrint("Detected our library [%s]", our_libraries[i]);
                return found_lib;
            }
        }
    }

    return NULL;
}

void AllowWriteToMappedMemory()
{
    for(int i = 0; i < 512; i++)
    {
        memory_prots_save_list[i] = 0;
    }

    FILE* smaps_file = fopen("/proc/self/smaps", "r");    

    if(!smaps_file)
    {
        rootconsole->ConsolePrint("Error opening smaps");
        return;
    }

    char* file_line = (char*) malloc(sizeof(char) * 1024);

    while(fgets(file_line, 1024, smaps_file))
    {
        sscanf(file_line, "%[^\n]s", file_line);

        Library* currentLibrary = getlibrary(file_line);
        if(!currentLibrary) continue;

        char* file_line_cpy = (char*) malloc(strlen(file_line)+1);
        snprintf(file_line_cpy, strlen(file_line)+1, "%s", file_line);

        char* address_range = strtok(file_line_cpy, " \t");
        char* protections = strtok(NULL, " \t");

        char* start_address = strtok(address_range, "-");
        char* end_address = strtok(NULL, "-");

        uint32_t start_address_parsed = 0;
        uint32_t end_address_parsed = 0;

        if(start_address) start_address_parsed = strtoul(start_address, NULL, 16);
        if(end_address) end_address_parsed = strtoul(end_address, NULL, 16);

        int save_protections = PROT_NONE;

        if(strstr(protections, "r") != 0)
            save_protections = PROT_READ;
        if(strstr(protections, "w") != 0)
            save_protections = save_protections | PROT_WRITE;
        if(strstr(protections, "x") != 0)
            save_protections = save_protections | PROT_EXEC;

        if(start_address_parsed && end_address_parsed)
        {
            int address_size = end_address_parsed - start_address_parsed;

            for(int i = 0; i < 512 && i+1 < 512 && i+2 < 512; i = i+3)
            {
                if(memory_prots_save_list[i] == 0 && memory_prots_save_list[i+1] == 0 && memory_prots_save_list[i+2] == 0)
                {
                    memory_prots_save_list[i] = start_address_parsed;
                    memory_prots_save_list[i+1] = end_address_parsed;
                    memory_prots_save_list[i+2] = (uint32_t)save_protections;
                    //rootconsole->ConsolePrint("Saved [%X] [%X] [%s]", end_address_parsed, start_address_parsed, currentLibrary->library_signature);
                    break;
                }
            }

            currentLibrary->library_size += address_size;
        }

        free(file_line_cpy);
    }

    free(file_line);
    fclose(smaps_file);

    ForceMemoryAccess();
}

void ForceMemoryAccess()
{
    for(int i = 0; i < 512 && i+1 < 512 && i+2 < 512; i = i+3)
    {
        if(memory_prots_save_list[i] == 0 && memory_prots_save_list[i+1] == 0 && memory_prots_save_list[i+2] == 0) continue;
        
        size_t pagesize = sysconf(_SC_PAGE_SIZE);
        uint32_t pagestart = memory_prots_save_list[i] & -pagesize;

        if(mprotect((void*)pagestart, memory_prots_save_list[i+1] - memory_prots_save_list[i], PROT_READ | PROT_WRITE | PROT_EXEC) == -1)
        {
            //rootconsole->ConsolePrint("Failed protection change: [%X] [%X]", memory_prots_save_list[i+1], memory_prots_save_list[i]);

            //SELINUX shite

            //perror("mprotect");
            //exit(EXIT_FAILURE);
        }
        else
        {
            //rootconsole->ConsolePrint("Passed protection change: [%X] [%X]", memory_prots_save_list[i+1], memory_prots_save_list[i]);
        }
    }
}

void RestoreMemoryProtections()
{
    for(int i = 0; i < 512 && i+1 < 512 && i+2 < 512; i = i+3)
    {
        size_t pagesize = sysconf(_SC_PAGE_SIZE);
        uint32_t pagestart = memory_prots_save_list[i] & -pagesize;

        if(mprotect((void*)pagestart, memory_prots_save_list[i+1] - memory_prots_save_list[i], memory_prots_save_list[i+2]) == -1)
        {
            perror("mprotect");
            exit(EXIT_FAILURE);
        }

        memory_prots_save_list[i] = 0;
        memory_prots_save_list[i+1] = 0;
        memory_prots_save_list[i+2] = 0;
    }
}

void ZeroVector(uint32_t vector)
{
    *(float*)(vector) = 0;
    *(float*)(vector+4) = 0;
    *(float*)(vector+8) = 0;
}

bool IsVectorNaN(uint32_t base)
{
    float s0 = *(float*)(base);
    float s1 = *(float*)(base+4);
    float s2 = *(float*)(base+8);

    if(s0 != s0 || s1 != s1 || s2 != s2)
        return true;

    return false;
}

bool IsValidVector(uint32_t base)
{
    float s0 = *(float*)(base);
    float s1 = *(float*)(base+4);
    float s2 = *(float*)(base+8);

    // Check for NaN and infinity
    if(isnan(s0) || isnan(s1) || isnan(s2) ||
        isinf(s0) || isinf(s1) || isinf(s2))
        return false;

    // Optional: Check for denormalized values
    if(is_denormalized(s0) || is_denormalized(s1) || is_denormalized(s2))
        return false;

    return true;
}

bool IsEntityPositionReasonable(uint32_t v)
{
    float x = *(float*)(v);
    float y = *(float*)(v+4);
    float z = *(float*)(v+8);

    if(IsValidVector(v))
    {
        float r = 32767.0f;

        return
            x > -r && x < r &&
            y > -r && y < r &&
            z > -r && z < r;
    }

    return false;
}

void InsertEntityToCollisionsList(uint32_t ent)
{
    if(IsEntityValid(ent))
    {
        char* classname = (char*)(*(uint32_t*)(ent+offsets.classname_offset));

        if(classname && strcmp(classname, "player") == 0)
        {
            player_collision_rules_changed = true;
            return;
        }

        for(int i = 0; i < 512; i++)
        {
            if(collisions_entity_list[i] != 0)
            {
                uint32_t refHandle = *(uint32_t*)(ent+offsets.refhandle_offset);

                if(refHandle == collisions_entity_list[i])
                    return;
            }
        }

        for(int i = 0; i < 512; i++)
        {
            if(collisions_entity_list[i] == 0)
            {
                uint32_t refHandle = *(uint32_t*)(ent+offsets.refhandle_offset);
                collisions_entity_list[i] = refHandle;

                break;
            }
        }
    }
}

void UpdateAllCollisions()
{
    functions.CleanupDeleteList(0);
    
    for(int i = 0; i < 512; i++)
    {
        if(collisions_entity_list[i] != 0)
        {
            uint32_t object = functions.GetCBaseEntity(collisions_entity_list[i]);

            if(IsEntityValid(object))
            {
                //rootconsole->ConsolePrint("Updated collisions!");
                functions.CollisionRulesChanged(object);
            }

            collisions_entity_list[i] = 0;
        }
    }

    uint32_t ent = 0;

    while((ent = functions.FindEntityByClassname(fields.CGlobalEntityList, ent, (uint32_t)"*")) != 0)
    {
        if(IsEntityValid(ent))
        {
            uint32_t m_Network = *(uint32_t*)(ent+offsets.mnetwork_offset);

            if(!m_Network)
            {
                functions.CollisionRulesChanged(ent);
            }
        }
    }
    
    ent = 0;

    while((ent = functions.FindEntityByClassname(fields.CGlobalEntityList, ent, (uint32_t)"player")) != 0)
    {
        if(IsEntityValid(ent))
        {
            functions.CollisionRulesChanged(ent);
        }
    }

    RemoveBadEnts();
}

void FixPlayerCollisionGroup()
{
    uint32_t player = 0;

    while((player = functions.FindEntityByClassname(fields.CGlobalEntityList, player, (uint32_t)"player")) != 0)
    {
        if(IsEntityValid(player))
        {
            uint32_t collision_flags = *(uint32_t*)(player+offsets.m_CollisionGroup_offset);

            if(collision_flags & 4)
            {
                *(uint32_t*)(player+offsets.m_CollisionGroup_offset) -= 4;
            }

            if(!(collision_flags & 2))
            {
                *(uint32_t*)(player+offsets.m_CollisionGroup_offset) += 2;
            }
        }
    }
}

void DisablePlayerWorldSpawnCollision()
{
    uint32_t player = 0;

    while((player = functions.FindEntityByClassname(fields.CGlobalEntityList, player, (uint32_t)"player")) != 0)
    {
        uint32_t worldspawn = functions.FindEntityByClassname(fields.CGlobalEntityList, 0, (uint32_t)"worldspawn");

        if(IsEntityValid(worldspawn) && IsEntityValid(player))
        {
            float player_velocity_x = *(uint32_t*)(player+offsets.abs_velocity_offset);
            float player_velocity_y = *(uint32_t*)(player+offsets.abs_velocity_offset+4);
            float player_velocity_z = *(uint32_t*)(player+offsets.abs_velocity_offset+8);

            if(player_velocity_x > 3260000000.0 || player_velocity_y > 3260000000.0)
            {
                functions.DisableEntityCollisions(player, worldspawn);
            }
            else
            {
                functions.EnableEntityCollisions(player, worldspawn);
            }

            //rootconsole->ConsolePrint("%f %f %f", player_velocity_x, player_velocity_y, player_velocity_z);

            //if(!player_worldspawn_collision_disabled)
            //{
            //    functions.DisableEntityCollisions(player, worldspawn);
            //}
        }
    }

    player_worldspawn_collision_disabled = true;
}

void DisablePlayerCollisions()
{
    uint32_t current_player = 0;

    while((current_player = functions.FindEntityByClassname(fields.CGlobalEntityList, current_player, (uint32_t)"player")) != 0)
    {
        if(IsEntityValid(current_player))
        {
            uint32_t other_players = 0;

            while((other_players = functions.FindEntityByClassname(fields.CGlobalEntityList, other_players, (uint32_t)"player")) != 0)
            {
                if(IsEntityValid(other_players) && other_players != current_player)
                {
                    //rootconsole->ConsolePrint("Disable player collisions!");
                    functions.DisableEntityCollisions(current_player, other_players);
                }
            }
        }
    }
}

void RemoveBadEnts()
{
    functions.CleanupDeleteList(0);

    uint32_t ent = 0;

    while((ent = functions.FindEntityByClassname(fields.CGlobalEntityList, ent, (uint32_t)"*")) != 0)
    {
        if(IsEntityValid(ent))
        {
            uint32_t abs_origin = ent+offsets.abs_origin_offset;
            uint32_t origin = ent+offsets.origin_offset;
            uint32_t abs_angles = ent+offsets.abs_angles_offset;
            uint32_t abs_velocity = ent+offsets.abs_velocity_offset;

            if
            (
            
            !IsEntityPositionReasonable(abs_origin)
            || 
            !IsEntityPositionReasonable(origin)
            || 
            !IsEntityPositionReasonable(abs_angles)
            ||
            !IsEntityPositionReasonable(abs_velocity)
            
            )
            {
                char* classname = (char*)(*(uint32_t*)(ent+offsets.classname_offset));
                
                if(classname)
                    rootconsole->ConsolePrint("Removed bad ent! [%s]", classname);
                else
                    rootconsole->ConsolePrint("Removed bad ent!");
                
                RemoveEntityNormal(ent, true);
            }
        }
    }

    functions.CleanupDeleteList(0);
}

void RemoveEntityNormal(uint32_t entity_object, bool validate)
{
    pOneArgProt pDynamicOneArgFunc;
    pThreeArgProt pDynamicThreeArgFunc;

    if(entity_object == 0) return;

    char* classname = (char*)(*(uint32_t*)(entity_object+offsets.classname_offset));
    uint32_t refHandle = *(uint32_t*)(entity_object+offsets.refhandle_offset);
    uint32_t object_verify = functions.GetCBaseEntity(refHandle);

    if(object_verify == 0)
    {
        if(!validate)
        {
            rootconsole->ConsolePrint("Warning: Entity delete request granted without validation!");
            object_verify = entity_object;
        }
    }

    if(object_verify)
    {
        if(classname && strcmp(classname, "player") == 0)
        {
            if(isTicking)
            {
                if(game == SYNERGY)
                {
                    Vector emptyVector;

                    //LeaveVehicle
                    pDynamicThreeArgFunc = (pThreeArgProt)( *(uint32_t*) ((*(uint32_t*)(object_verify))+0x648) );
                    pDynamicThreeArgFunc(object_verify, (uint32_t)&emptyVector, (uint32_t)&emptyVector);

                    functions.SpawnPlayer(object_verify);

                    //LeaveVehicle
                    pDynamicThreeArgFunc = (pThreeArgProt)( *(uint32_t*) ((*(uint32_t*)(object_verify))+0x648) );
                    pDynamicThreeArgFunc(object_verify, (uint32_t)&emptyVector, (uint32_t)&emptyVector);

                    functions.SpawnPlayer(object_verify);
                }
                else
                {
                    functions.SpawnPlayer(object_verify);
                    functions.SpawnPlayer(object_verify);
                }

                rootconsole->ConsolePrint("Tried killing player but was protected & respawned!");
                return;
            }
        }

        if(IsMarkedForDeletion(object_verify+offsets.iserver_offset)) return;

        if(game == SYNERGY)
        {
            //Synergy
            extern bool saving_now;
            extern bool savegame_internal;

            if(saving_now || savegame_internal)
            {
                InstaKill(object_verify, true);
                rootconsole->ConsolePrint("WARNING: Removing [%s] while a save file is being made!", classname);
                return;
            }
        }

        functions.RemoveNormal(object_verify);

        //rootconsole->ConsolePrint("Removed [%s]", clsname);

        return;
    }

    rootconsole->ConsolePrint("Failed to verify entity object!");
    exit(EXIT_FAILURE);
}

void InstaKill(uint32_t entity_object, bool validate)
{
    pOneArgProt pDynamicOneArgFunc;

    if(entity_object == 0) return;

    uint32_t refHandleInsta = *(uint32_t*)(entity_object+offsets.refhandle_offset);
    char* classname = (char*) ( *(uint32_t*)(entity_object+offsets.classname_offset) );
    uint32_t cbase_chk = functions.GetCBaseEntity(refHandleInsta);

    if(cbase_chk == 0)
    {
        if(!validate)
        {
            rootconsole->ConsolePrint("Warning: Entity delete request granted without validation!");
            cbase_chk = entity_object;
        }
        else
        {
            rootconsole->ConsolePrint("\n\nFailed to verify entity for fast kill [%X]\n\n", (uint32_t)__builtin_return_address(0) - server_srv);
            exit(EXIT_FAILURE);
            return;
        }
    }

    functions.RemoveInsta(cbase_chk);
}

bool IsMarkedForDeletion(uint32_t arg0)
{
    return *(uint32_t*)(*(uint32_t*)(arg0 + 8) + offsets.ismarked_offset) & 1;
}

uint32_t IsEntityValid(uint32_t entity)
{
    pOneArgProt pDynamicOneArgFunc;
    if(entity == 0) return entity;

    uint32_t object = functions.GetCBaseEntity(*(uint32_t*)(entity+offsets.refhandle_offset));

    if(object)
    {
        if(IsMarkedForDeletion(object+offsets.iserver_offset))
            return 0;

        return entity;
    }

    return 0;
}

ValueList AllocateValuesList()
{
    ValueList list = (ValueList) malloc(sizeof(ValueList));
    *list = NULL;
    return list;
}

Value* CreateNewValue(void* valueInput)
{
    Value* val = (Value*) malloc(sizeof(Value));

    val->value = valueInput;
    val->nextVal = NULL;
    return val;
}

int DeleteAllValuesInList(ValueList list, bool free_val, pthread_mutex_t* lockInput)
{
    if(lockInput)
    {
        while(pthread_mutex_trylock(lockInput) != 0);
    }

    int removed_items = 0;

    if(!list || !*list)
    {
        
        if(lockInput)
        {
            pthread_mutex_unlock(lockInput);
        }

        return removed_items;
    }
    
    Value* aValue = *list;

    while(aValue)
    {
        Value* detachedValue = aValue->nextVal;
        if(free_val) free(aValue->value);
        free(aValue);
        aValue = detachedValue;

        removed_items++;
    }

    *list = NULL;

    if(lockInput)
    {
        pthread_mutex_unlock(lockInput);
    }

    return removed_items;
}

Value* FindStringInList(ValueList list, const char* search_val, pthread_mutex_t* lockInput, bool substring, Value* start_value)
{
    if(lockInput)
    {
        while(pthread_mutex_trylock(lockInput) != 0);
    }

    Value* aValue = *list;

    if(start_value)
    {
        aValue = start_value;
    }

    while(aValue)
    {
        char* list_item = (char*)aValue->value;

        if(substring)
        {
            if(list_item && strstr(list_item, search_val) != NULL)
            {
                if(lockInput)
                {
                    pthread_mutex_unlock(lockInput);
                }

                return aValue;
            }
        }
        else
        {
            if(list_item && strcmp(list_item, search_val) == 0)
            {
                if(lockInput)
                {
                    pthread_mutex_unlock(lockInput);
                }

                return aValue;
            }
        }
        
        aValue = aValue->nextVal;
    }

    if(lockInput)
    {
        pthread_mutex_unlock(lockInput);
    }

    return NULL;
}

bool IsInValuesList(ValueList list, void* searchVal, pthread_mutex_t* lockInput)
{
    if(lockInput)
    {
        while(pthread_mutex_trylock(lockInput) != 0);
    }

    Value* aValue = *list;

    while(aValue)
    {
        if((uint32_t)aValue->value == (uint32_t)searchVal)
        {
            if(lockInput)
            {
                pthread_mutex_unlock(lockInput);
            }

            return true;
        }
        
        aValue = aValue->nextVal;
    }

    if(lockInput)
    {
        pthread_mutex_unlock(lockInput);
    }

    return false;
}

bool RemoveFromValuesList(ValueList list, void* searchVal, pthread_mutex_t* lockInput)
{
    if(lockInput)
    {
        while(pthread_mutex_trylock(lockInput) != 0);
    }

    Value* aValue = *list;

    if(aValue == NULL)
    {
        if(lockInput)
        {
            pthread_mutex_unlock(lockInput);
        }

        return false;
    }

    //search at the start of the list
    if(((uint32_t)aValue->value) == ((uint32_t)searchVal))
    {
        Value* detachedValue = aValue->nextVal;
        free(*list);
        *list = detachedValue;

        if(lockInput)
        {
            pthread_mutex_unlock(lockInput);
        }

        return true;
    }

    //search the rest of the list
    while(aValue->nextVal)
    {
        if(((uint32_t)aValue->nextVal->value) == ((uint32_t)searchVal))
        {
            Value* detachedValue = aValue->nextVal->nextVal;

            free(aValue->nextVal);
            aValue->nextVal = detachedValue;

            if(lockInput)
            {
                pthread_mutex_unlock(lockInput);
            }

            return true;
        }

        aValue = aValue->nextVal;
    }

    if(lockInput)
    {
        pthread_mutex_unlock(lockInput);
    }

    return false;
}

int ValueListItems(ValueList list, pthread_mutex_t* lockInput)
{
    if(lockInput)
    {
        while(pthread_mutex_trylock(lockInput) != 0);
    }

    Value* aValue = *list;
    int counter = 0;

    while(aValue)
    {
        counter++;
        aValue = aValue->nextVal;
    }

    if(lockInput)
    {
        pthread_mutex_unlock(lockInput);
    }

    return counter;
}

bool InsertToValuesList(ValueList list, Value* head, pthread_mutex_t* lockInput, bool tail, bool duplicate_chk)
{
    if(lockInput)
    {
        while(pthread_mutex_trylock(lockInput) != 0);
    }

    if(duplicate_chk)
    {
        Value* aValue = *list;

        while(aValue)
        {
            if((uint32_t)aValue->value == (uint32_t)head->value)
            {
                if(lockInput)
                {
                    pthread_mutex_unlock(lockInput);
                }

                return false;
            }
        
            aValue = aValue->nextVal;
        }
    }

    if(tail)
    {
        Value* aValue = *list;

        while(aValue)
        {
            if(aValue->nextVal == NULL)
            {
                aValue->nextVal = head;

                if(lockInput)
                {
                    pthread_mutex_unlock(lockInput);
                }

                return true;
            }

            aValue = aValue->nextVal;
        }
    }

    head->nextVal = *list;
    *list = head;

    if(lockInput)
    {
        pthread_mutex_unlock(lockInput);
    }

    return true;
}

EntityKV* CreateNewEntityKV(uint32_t refHandle, uint32_t keyIn, uint32_t valueIn)
{
    EntityKV* kv = (EntityKV*) malloc(sizeof(EntityKV));

    kv->entityRef = refHandle;
    kv->key = keyIn;
    kv->value = valueIn;

    return kv;
}

bool is_denormalized(float value)
{
    return fpclassify(value) == FP_SUBNORMAL;
}

bool is_negative_zero(float value)
{
    return value == 0.0 && signbit(value);
}