#include "extension.h"
#include "util.h"
#include "core.h"
#include "ext_main.h"
#include "hooks_specific.h"

int save_frames;
bool saving_now;
bool savegame_internal;
uint32_t global_restore_player;
ValueList restore_vehicle_list;
ValueList dangling_restore_vehicles;
ValueList save_player_vehicles_list;

bool InitExtensionSynergy()
{
    if(loaded_extension)
    {
        rootconsole->ConsolePrint("Attempted to load extension twice!");
        return false;
    }

    InitCoreSynergy();
    AllowWriteToMappedMemory();

    char* root_dir = getenv("PWD");
    const size_t max_path_length = 1024;

    char server_srv_fullpath[max_path_length];
    char synergy_srv_fullpath[max_path_length];
    char engine_srv_fullpath[max_path_length];
    char dedicated_srv_fullpath[max_path_length];
    char vphysics_srv_fullpath[max_path_length];
    char sdktools_path[max_path_length];

    snprintf(server_srv_fullpath, max_path_length, "/synergy/bin/server_srv.so");
    snprintf(synergy_srv_fullpath, max_path_length, "/synergy/bin/synergy_srv.so");
    snprintf(engine_srv_fullpath, max_path_length, "/bin/engine_srv.so");
    snprintf(dedicated_srv_fullpath, max_path_length, "/bin/dedicated_srv.so");
    snprintf(vphysics_srv_fullpath, max_path_length, "/bin/vphysics_srv.so");
    snprintf(sdktools_path, max_path_length, "/extensions/sdktools.ext.2.sdk2013.so");

    Library* server_srv_lib = FindLibrary(server_srv_fullpath, true);
    Library* synergy_srv_lib = FindLibrary(synergy_srv_fullpath, true);
    Library* engine_srv_lib = FindLibrary(engine_srv_fullpath, true);
    Library* dedicated_srv_lib = FindLibrary(dedicated_srv_fullpath, true);
    Library* vphysics_srv_lib = FindLibrary(vphysics_srv_fullpath, true);
    Library* sdktools_lib = FindLibrary(sdktools_path, true);

    if(!(engine_srv_lib && dedicated_srv_lib && vphysics_srv_lib && server_srv_lib && synergy_srv_lib && sdktools_lib))
    {
        RestoreMemoryProtections();
        ClearLoadedLibraries();
        rootconsole->ConsolePrint("----------------------  Failed to load Synergy " SMEXT_CONF_NAME " " SMEXT_CONF_VERSION "  ----------------------");
        return false;
    }

    game = SYNERGY;

    rootconsole->ConsolePrint("server_srv_lib [%X] size [%X]", server_srv_lib->library_base_address, server_srv_lib->library_size);
    rootconsole->ConsolePrint("synergy_srv_lib [%X] size [%X]", synergy_srv_lib->library_base_address, synergy_srv_lib->library_size);
    rootconsole->ConsolePrint("engine_srv_lib [%X] size [%X]", engine_srv_lib->library_base_address, engine_srv_lib->library_size);
    rootconsole->ConsolePrint("dedicated_srv_lib [%X] size [%X]", dedicated_srv_lib->library_base_address, dedicated_srv_lib->library_size);
    rootconsole->ConsolePrint("vphysics_srv_lib [%X] size [%X]", vphysics_srv_lib->library_base_address, vphysics_srv_lib->library_size);
    rootconsole->ConsolePrint("sdktools_lib [%X] size [%X]", sdktools_lib->library_base_address, sdktools_lib->library_size);

    server_srv = server_srv_lib->library_base_address;
    synergy_srv = synergy_srv_lib->library_base_address;
    engine_srv = engine_srv_lib->library_base_address;
    dedicated_srv = dedicated_srv_lib->library_base_address;
    vphysics_srv = vphysics_srv_lib->library_base_address;
    sdktools = sdktools_lib->library_base_address;

    server_srv_size = server_srv_lib->library_size;
    synergy_srv_size = synergy_srv_lib->library_size;
    engine_srv_size = engine_srv_lib->library_size;
    dedicated_srv_size = dedicated_srv_lib->library_size;
    vphysics_srv_size = vphysics_srv_lib->library_size;
    sdktools_size = sdktools_lib->library_size;

    sdktools_passed = IsAllowedToPatchSdkTools(sdktools, sdktools_size);

    RestoreLinkedLists();
    SaveProcessId();

    save_frames = 0;
    saving_now = false;
    savegame_internal = false;
    global_restore_player = 0;
    restore_vehicle_list = AllocateValuesList();
    dangling_restore_vehicles = AllocateValuesList();
    save_player_vehicles_list = AllocateValuesList();

    fields.CGlobalEntityList = server_srv + 0x00EAB5BC;
    fields.sv = engine_srv + 0x00394538;
    fields.RemoveImmediateSemaphore = server_srv + 0x00F3BCB0;
    fields.sv_cheats_cvar = engine_srv + 0x00394450;
    fields.deferMindist = vphysics_srv + 0x001B3900;

    offsets.classname_offset = 0x74;
    offsets.abs_origin_offset = 0x2A4;
    offsets.abs_angles_offset = 0x338;
    offsets.abs_velocity_offset = 0x23C;
    offsets.origin_offset = 0x344;
    offsets.mnetwork_offset = 0x30;
    offsets.refhandle_offset = 0x35C;
    offsets.iserver_offset = 0x24;
    offsets.collision_property_offset = 0x170;
    offsets.ismarked_offset = 0x128;
    offsets.vphysics_object_offset = 0x208;
    offsets.m_CollisionGroup_offset = 516;

    functions.GetCBaseEntity = (pOneArgProt)(GetCBaseEntitySynergy);
    functions.SpawnPlayer = (pOneArgProt)(server_srv + 0x00C2F140);
    functions.RemoveNormalDirect = (pOneArgProt)(server_srv + 0x008A0AC0);
    functions.RemoveNormal = (pOneArgProt)(server_srv + 0x008A0BD0);
    functions.RemoveInsta = (pOneArgProt)(server_srv + 0x008A0DF0);
    functions.CreateEntityByName = (pTwoArgProt)(server_srv + 0x007005E0);
    functions.PhysSimEnt = (pOneArgProt)(server_srv + 0x0074E3D0);
    functions.AcceptInput = (pSixArgProt)(server_srv + 0x005B4850);
    functions.UpdateOnRemoveBase = (pOneArgProt)(server_srv + 0x005AF050);
    functions.VphysicsSetObject = (pOneArgProt)(server_srv + 0x005D01E0);
    functions.ClearAllEntities = (pOneArgProt)(server_srv + 0x0064AEE0);
    functions.SetSolidFlags = (pTwoArgProt)(server_srv + 0x00615830);
    functions.DisableEntityCollisions = (pTwoArgProt)(server_srv + 0x0077C2A0);
    functions.EnableEntityCollisions = (pTwoArgProt)(server_srv + 0x0077C400);
    functions.CollisionRulesChanged = (pOneArgProt)(server_srv + 0x005D0240);
    functions.FindEntityByClassname = (pThreeArgProt)(server_srv + 0x0064B210);
    functions.CleanupDeleteList = (pOneArgProt)(server_srv + 0x0064AC50);
    functions.PackedStoreDestructor = (pOneArgProt)(dedicated_srv + 0x000C4B70);
    functions.CanSatisfyVpkCacheInternal = (pSevenArgProt)(dedicated_srv + 0x000C7EB0);
    functions.AddSearchPath = (pFourArgProt)(dedicated_srv + 0x0006DAB0);
    functions.SV_ReplicateConVarChange = (pTwoArgProt)(engine_srv + 0x0027BC50);

    PopulateHookExclusionListsSynergy();

    InitUtil();

    ApplyPatchesSynergy();
    ApplyPatchesSpecificSynergy();

    HookFunctionsSynergy();
    HookFunctionsSpecificSynergy();

    RestoreMemoryProtections();

    rootconsole->ConsolePrint("\n\nServer Map: [%s]\n\n", fields.sv+0x11);
    rootconsole->ConsolePrint("----------------------  Synergy " SMEXT_CONF_NAME " " SMEXT_CONF_VERSION " loaded  ----------------------");
    loaded_extension = true;

    return true;
}

void ApplyPatchesSynergy()
{
    uint32_t offset = 0;

    uint32_t hook_dedicated_vpk_malloc = dedicated_srv + 0x000C81D4;
    offset = (uint32_t)HooksSynergy::DirectMallocHookDedicatedSrv - hook_dedicated_vpk_malloc - 5;
    *(uint32_t*)(hook_dedicated_vpk_malloc+1) = offset;

    uint32_t patch_stack_vpk_cache_allocation = dedicated_srv + 0x000C81CD;
    memset((void*)patch_stack_vpk_cache_allocation, 0x90, 7);

    *(uint8_t*)(patch_stack_vpk_cache_allocation) = 0x89;
    *(uint8_t*)(patch_stack_vpk_cache_allocation+1) = 0x34;
    *(uint8_t*)(patch_stack_vpk_cache_allocation+2) = 0x24;

    uint32_t force_jump_vpk_allocation = dedicated_srv + 0x000C80B3;
    memset((void*)force_jump_vpk_allocation, 0x90, 6);

    *(uint8_t*)(force_jump_vpk_allocation) = 0xE9;
    *(uint32_t*)(force_jump_vpk_allocation+1) = 0x112;

    if(sdktools_passed)
    {
        memset((void*)(sdktools + 0x00016903), 0x90, 2);
        memset((void*)(sdktools + 0x00016907), 0x90, 2);
    }

    uint32_t hook_game_frame = server_srv + 0x006B1E64;
    offset = (uint32_t)HooksSynergy::SimulateEntitiesHook - hook_game_frame - 5;
    *(uint32_t*)(hook_game_frame+1) = offset;

    uint32_t hook_reverse_order = server_srv + 0x006B1E70;
    offset = (uint32_t)HooksUtil::EmptyCall - hook_reverse_order - 5;
    *(uint32_t*)(hook_reverse_order+1) = offset;

    uint32_t hook_post_systems = server_srv + 0x006B1E7C;
    offset = (uint32_t)HooksUtil::EmptyCall - hook_post_systems - 5;
    *(uint32_t*)(hook_post_systems+1) = offset;

    uint32_t hook_service_event_queue = server_srv + 0x006B1E8A;
    offset = (uint32_t)HooksUtil::EmptyCall - hook_service_event_queue - 5;
    *(uint32_t*)(hook_service_event_queue+1) = offset;

    uint32_t nearplayer_bypass = server_srv + 0x00C2AE3C;
    *(uint8_t*)(nearplayer_bypass) = 0xE9;
    *(uint32_t*)(nearplayer_bypass+1) = 0x1A9;

    //spawning crash
    uint32_t patch_player_spawn_crash = server_srv + 0x00C2F28C;
    *(uint8_t*)(patch_player_spawn_crash) = 0xEB;

    //spawning crash
    uint32_t patch_player_restore = server_srv + 0x00BDD0CC;
    memset((void*)patch_player_restore, 0x90, 0x26);

    //player vehicle restoring patch
    uint32_t removebad_restorecode = server_srv + 0x00BDCEED;
    memset((void*)removebad_restorecode, 0x90, 2);

    //player vehicle restoring patch
    uint32_t vehicle_restore_hook = server_srv + 0x00BDCED7;
    offset = (uint32_t)HooksSynergy::VehicleInitializeRestore - vehicle_restore_hook - 5;
    *(uint32_t*)(vehicle_restore_hook+1) = offset;

    //CMessageEntity
    uint32_t remove_extra_call = server_srv + 0x0070715B;
    offset = (uint32_t)HooksUtil::EmptyCall - remove_extra_call - 5;
    *(uint32_t*)(remove_extra_call+1) = offset;
}

void ReorderGameSearchPaths()
{
    pTwoArgProt pDynamicTwoArgFunc;
    pFourArgProt pDynamicFourArgFunc;

    Value* search_path_node = *game_search_paths;
    if(!search_path_node) return;

    //RemoveSearchPaths
    pDynamicTwoArgFunc = (pTwoArgProt)(dedicated_srv + 0x000639E0);
    pDynamicTwoArgFunc(dedicated_srv + 0x00278900, (uint32_t)"GAME");

    //RemoveSearchPaths
    pDynamicTwoArgFunc = (pTwoArgProt)(dedicated_srv + 0x000639E0);
    pDynamicTwoArgFunc(dedicated_srv + 0x00278900, (uint32_t)"SHADOW_ep2");

    //RemoveSearchPaths
    pDynamicTwoArgFunc = (pTwoArgProt)(dedicated_srv + 0x000639E0);
    pDynamicTwoArgFunc(dedicated_srv + 0x00278900, (uint32_t)"SHADOW_ep1");

    while(search_path_node && search_path_node->nextVal)
    {
        char* game_search_path_abs = (char*)search_path_node->value;
        uint32_t head_tail = *(uint32_t*)(search_path_node->nextVal->value);

        char* game_search_path_hl2 = strstr(game_search_path_abs, "Half-Life 2/");
        char* game_search_path_syn = strstr(game_search_path_abs, "Synergy/");

        if(game_search_path_hl2 && strcmp(game_search_path_hl2, "Half-Life 2/ep2/") == 0)
        {
            search_path_node = search_path_node->nextVal->nextVal;
            continue;
        }

        if(game_search_path_hl2 && strcmp(game_search_path_hl2, "Half-Life 2/episodic/") == 0)
        {
            search_path_node = search_path_node->nextVal->nextVal;
            continue;
        }

        //AddSearchPath
        pDynamicFourArgFunc = (pFourArgProt)(functions.AddSearchPath);
        pDynamicFourArgFunc(dedicated_srv + 0x00278900, (uint32_t)game_search_path_abs, (uint32_t)"GAME", head_tail);

        rootconsole->ConsolePrint("Added reordered path %s %s", "GAME", (uint32_t)game_search_path_abs);

        if(game_search_path_hl2 && strcmp(game_search_path_hl2, "Half-Life 2/ep2/ep2_pak_dir.vpk") == 0)
        {
            Value* folder_path = FindStringInList(game_search_paths, "Half-Life 2/ep2/", NULL, true, NULL);

            while(folder_path)
            {
                char* folder_search_path_trimmed = strstr((char*)folder_path->value, "Half-Life 2/");

                if(folder_search_path_trimmed && strcmp(folder_search_path_trimmed, "Half-Life 2/ep2/") == 0)
                {
                    //AddSearchPath
                    pDynamicFourArgFunc = (pFourArgProt)(functions.AddSearchPath);
                    pDynamicFourArgFunc(dedicated_srv + 0x00278900, (uint32_t)folder_path->value, (uint32_t)"SHADOW_ep2", head_tail);

                    //AddSearchPath
                    pDynamicFourArgFunc = (pFourArgProt)(functions.AddSearchPath);
                    pDynamicFourArgFunc(dedicated_srv + 0x00278900, (uint32_t)folder_path->value, (uint32_t)"GAME", head_tail);

                    rootconsole->ConsolePrint("Added reordered path %s %s", "GAME", (uint32_t)folder_path->value);
                }

                if(folder_path->nextVal == NULL)
                    break;

                folder_path = FindStringInList(game_search_paths, "Half-Life 2/ep2/", NULL, true, folder_path->nextVal);
            }
        }
        else if(game_search_path_hl2 && strcmp(game_search_path_hl2, "Half-Life 2/episodic/ep1_pak_dir.vpk") == 0)
        {
            Value* folder_path = FindStringInList(game_search_paths, "Half-Life 2/episodic/", NULL, true, NULL);

            while(folder_path)
            {
                char* folder_search_path_trimmed = strstr((char*)folder_path->value, "Half-Life 2/");

                if(folder_search_path_trimmed && strcmp(folder_search_path_trimmed, "Half-Life 2/episodic/") == 0)
                {
                    //AddSearchPath
                    pDynamicFourArgFunc = (pFourArgProt)(functions.AddSearchPath);
                    pDynamicFourArgFunc(dedicated_srv + 0x00278900, (uint32_t)folder_path->value, (uint32_t)"SHADOW_ep1", head_tail);

                    //AddSearchPath
                    pDynamicFourArgFunc = (pFourArgProt)(functions.AddSearchPath);
                    pDynamicFourArgFunc(dedicated_srv + 0x00278900, (uint32_t)folder_path->value, (uint32_t)"GAME", head_tail);

                    rootconsole->ConsolePrint("Added reordered path %s %s", "GAME", (uint32_t)folder_path->value);
                }

                if(folder_path->nextVal == NULL)
                    break;

                folder_path = FindStringInList(game_search_paths, "Half-Life 2/episodic/", NULL, true, folder_path->nextVal);
            }
        }

        search_path_node = search_path_node->nextVal->nextVal;
    }
}

void FixCarSlashes()
{
    uint32_t mainEnt = 0;

    while((mainEnt = functions.FindEntityByClassname(fields.CGlobalEntityList, mainEnt, (uint32_t)"*")) != 0)
    {
        char* clsname = (char*) ( *(uint32_t*)(mainEnt+offsets.classname_offset) );
        
        if(strcmp(clsname, "prop_vehicle_jeep") != 0 && strcmp(clsname, "prop_vehicle_mp") != 0 && strcmp(clsname, "prop_vehicle_airboat") != 0)
            continue;

        uint32_t model = *(uint32_t*)(mainEnt+556);
        uint32_t script = *(uint32_t*)(mainEnt+1544);

        bool fixed_model = FixSlashes((char*)model);
        bool fixed_script = FixSlashes((char*)script);

        if(fixed_model)
        {
            rootconsole->ConsolePrint("FIXED_MODEL_NAME: [%s]", model);
        }

        if(fixed_script)
        {
            rootconsole->ConsolePrint("FIXED_SCRIPT_NAME: [%s]", script);
        }
    }
}

uint32_t GetPassengerIndex(uint32_t player, uint32_t player_vehicle)
{
    pOneArgProt pDynamicOneArgFunc;
    pTwoArgProt pDynamicTwoArgFunc;

    if(IsEntityValid(player) && IsEntityValid(player_vehicle))
    {
        char* vehicle_classname = (char*)(*(uint32_t*)(player_vehicle+offsets.classname_offset));

        if((vehicle_classname && strcmp(vehicle_classname, "prop_vehicle_airboat")) == 0
            ||
        (vehicle_classname && strcmp(vehicle_classname, "prop_vehicle_mp")) == 0
            ||
        (vehicle_classname && strncmp(vehicle_classname, "prop_vehicle_jeep", 17)) == 0)
        {
            uint32_t iserver_vehicle = *(uint32_t*)(player_vehicle+0x674);
            uint32_t base_vehicle = *(uint32_t*)(iserver_vehicle+0x30);

            //GetPassengerCount
            pDynamicOneArgFunc = (pOneArgProt)(*(uint32_t*)((*(uint32_t*)(base_vehicle))+0x4C));
            uint32_t passengers = pDynamicOneArgFunc(base_vehicle);

            for(uint32_t i = 0; i < passengers; i++)
            {
                pDynamicTwoArgFunc = (pTwoArgProt)(*(uint32_t*)(*(uint32_t*)(iserver_vehicle)));
                uint32_t passenger = pDynamicTwoArgFunc(iserver_vehicle, i);

                if(IsEntityValid(passenger))
                {
                    if(passenger == player)
                        return i;
                }
            }
        }
        else
            return -1;
    }

    rootconsole->ConsolePrint("Failed to get passenger index!");
    return 0;
}

void MakePlayersLeaveVehicles()
{
    pOneArgProt pDynamicOneArgFunc;
    pTwoArgProt pDynamicTwoArgFunc;
    pThreeArgProt pDynamicThreeArgFunc;
    uint32_t player = 0;

    while((player = functions.FindEntityByClassname(fields.CGlobalEntityList, player, (uint32_t)"player")) != 0)
    {
        if(IsEntityValid(player))
        {
            Vector emptyVector;
            uint32_t player_vehicle = functions.GetCBaseEntity(*(uint32_t*)(player+0x0D38));

            if(IsEntityValid(player_vehicle))
            {
                char* vehicle_classname = (char*)(*(uint32_t*)(player_vehicle+offsets.classname_offset));
                uint32_t passenger = GetPassengerIndex(player, player_vehicle);

                if(passenger != -1u)
                {
                    Value* player_value = CreateNewValue((void*)*(uint32_t*)(player+offsets.refhandle_offset));
                    Value* vehicle_value = CreateNewValue((void*)*(uint32_t*)(player_vehicle+offsets.refhandle_offset));
                    Value* passenger_value = CreateNewValue((void*)passenger);

                    InsertToValuesList(save_player_vehicles_list, player_value, NULL, true, false);
                    InsertToValuesList(save_player_vehicles_list, vehicle_value, NULL, true, false);
                    InsertToValuesList(save_player_vehicles_list, passenger_value, NULL, true, false);
                }

                //LeaveVehicle
                pDynamicThreeArgFunc = (pThreeArgProt)( *(uint32_t*) ((*(uint32_t*)(player))+0x648) );
                pDynamicThreeArgFunc(player, (uint32_t)&emptyVector, (uint32_t)&emptyVector);
            }
        }
    }
}

void RemoveDanglingRestoredVehicles()
{
    Value* first_vehicle = *dangling_restore_vehicles;

    while(first_vehicle)
    {
        Value* next_vehicle = first_vehicle->nextVal;

        rootconsole->ConsolePrint("Removed dangling vehicle!");

        RemoveEntityNormal(functions.GetCBaseEntity((uint32_t)first_vehicle->value), true);

        free(first_vehicle);
        first_vehicle = next_vehicle;
    }

    *dangling_restore_vehicles = NULL;
}

void EnterVehicles(ValueList vehi_list)
{
    pThreeArgProt pDynamicThreeArgFunc;
    Value* first_player = *vehi_list;

    while(first_player && first_player->nextVal && first_player->nextVal->nextVal)
    {
        uint32_t player = functions.GetCBaseEntity((uint32_t)first_player->value);
        uint32_t vehicle = functions.GetCBaseEntity((uint32_t)first_player->nextVal->value);
        uint32_t passenger = (uint32_t)first_player->nextVal->nextVal->value;

        if(IsEntityValid(player) && IsEntityValid(vehicle))
        {
            rootconsole->ConsolePrint("Vehicle Entered! passenger [%d]", passenger);

            //EnterVehicle
            pDynamicThreeArgFunc = (pThreeArgProt)( *(uint32_t*) ((*(uint32_t*)(player))+0x644) );
            pDynamicThreeArgFunc(player, *(uint32_t*)(vehicle+0x674), passenger);
        }

        Value* nextPlayer = first_player->nextVal->nextVal->nextVal;

        free(first_player->nextVal->nextVal);
        free(first_player->nextVal);
        free(first_player);

        first_player = nextPlayer;
    }

    *vehi_list = NULL;
}

uint32_t HooksSynergy::VehicleInitializeRestore(uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    pThreeArgProt pDynamicThreeArgFunc;

    uint32_t vehi_cbase = arg0-0x4CC;
    uint32_t vehi_refhandle = *(uint32_t*)(vehi_cbase+offsets.refhandle_offset);

    if(global_restore_player)
    {
        uint32_t passenger = GetPassengerIndex(functions.GetCBaseEntity(global_restore_player), vehi_cbase);

        if(passenger != -1u)
        {
            Value* player_value = CreateNewValue((void*)global_restore_player);
            Value* vehicle_value = CreateNewValue((void*)vehi_refhandle);
            Value* passenger_value = CreateNewValue((void*)passenger);

            InsertToValuesList(restore_vehicle_list, player_value, NULL, true, false);
            InsertToValuesList(restore_vehicle_list, vehicle_value, NULL, true, false);
            InsertToValuesList(restore_vehicle_list, passenger_value, NULL, true, false);
        }

        global_restore_player = 0;
    }
    else
    {
        uint32_t vehicle_refhandle = *(uint32_t*)(vehi_cbase+offsets.refhandle_offset);

        Value* dangling_vehicle = CreateNewValue((void*)vehicle_refhandle);
        InsertToValuesList(dangling_restore_vehicles, dangling_vehicle, NULL, false, false);
    }

    pDynamicThreeArgFunc = (pThreeArgProt)(server_srv + 0x0068DC60);
    return pDynamicThreeArgFunc(arg0, arg1, arg2);
}

uint32_t HooksSynergy::RestorePlayerHook(uint32_t arg0, uint32_t arg1)
{
    pTwoArgProt pDynamicTwoArgFunc;
    pThreeArgProt pDynamicThreeArgFunc;

    global_restore_player = *(uint32_t*)(arg1+offsets.refhandle_offset);

    pDynamicTwoArgFunc = (pTwoArgProt)(server_srv + 0x00BDC650);
    uint32_t returnVal = pDynamicTwoArgFunc(arg0, arg1);

    Vector emptyVector;

    //LeaveVehicle
    pDynamicThreeArgFunc = (pThreeArgProt)( *(uint32_t*) ((*(uint32_t*)(arg1))+0x648) );
    pDynamicThreeArgFunc(arg1, (uint32_t)&emptyVector, (uint32_t)&emptyVector);

    return returnVal;
}

uint32_t HooksSynergy::DirectMallocHookDedicatedSrv(uint32_t arg0)
{
    uint32_t ebp = 0;
    asm volatile ("movl %%ebp, %0" : "=r" (ebp));

    uint32_t arg0_return = *(uint32_t*)(ebp-4);
    uint32_t packed_store_ref = arg0_return-0x228;

    uint32_t vpk_buffer = *(uint32_t*)(arg0+0x10);

    if(vpk_buffer == 0)
    {
        current_vpk_buffer_ref = arg0;
        return global_vpk_cache_buffer;
    }

    bool saved_reference = false;

    Value* a_leak = *leakedResourcesVpkSystem;

    while(a_leak)
    {
        VpkMemoryLeak* the_leak = (VpkMemoryLeak*)(a_leak->value);
        uint32_t packed_object = the_leak->packed_ref;

        if(packed_object == packed_store_ref)
        {
            saved_reference = true;

            ValueList vpk_leak_list = the_leak->leaked_refs;

            Value* new_vpk_leak = CreateNewValue((void*)(vpk_buffer));
            bool added = InsertToValuesList(vpk_leak_list, new_vpk_leak, NULL, false, true);

            if(added)
            {
                rootconsole->ConsolePrint("[VPK Hook] " HOOK_MSG, vpk_buffer);
            }

            break;
        }

        a_leak = a_leak->nextVal;
    }

    if(!saved_reference)
    {
        VpkMemoryLeak* omg_leaks = (VpkMemoryLeak*)(malloc(sizeof(VpkMemoryLeak)));
        ValueList empty_list = AllocateValuesList();

        Value* original_vpk_buffer = CreateNewValue((void*)vpk_buffer);
        InsertToValuesList(empty_list, original_vpk_buffer, NULL, false, false);

        omg_leaks->packed_ref = packed_store_ref;
        omg_leaks->leaked_refs = empty_list;

        Value* leaked_resource = CreateNewValue((void*)omg_leaks);
        InsertToValuesList(leakedResourcesVpkSystem, leaked_resource, NULL, false, false);

        rootconsole->ConsolePrint("[VPK Hook First] " HOOK_MSG, vpk_buffer);
    }

    return vpk_buffer;
}

uint32_t HooksSynergy::SimulateEntitiesHook(uint8_t simulating)
{
    isTicking = true;
    save_frames++;

    if(save_frames > 1000) save_frames = 100;

    pOneArgProt pDynamicOneArgFunc;
    pTwoArgProt pDynamicTwoArgFunc;

    pOneArgProtFastCall pDynamicFastCallOneArgFunc;
    pTwoArgProtFastCall pDynamicFastCallTwoArgFunc;

    functions.CleanupDeleteList(0);

    uint32_t firstplayer = functions.FindEntityByClassname(fields.CGlobalEntityList, 0, (uint32_t)"player");

    if(!firstplayer)
    {
        server_sleeping = true;
    }
    else
    {
        server_sleeping = false;
    }

    RemoveBadEnts();

    functions.CleanupDeleteList(0);

    //SimulateEntities
    pDynamicOneArgFunc = (pOneArgProt)(server_srv + 0x0074E6A0);
    pDynamicOneArgFunc(simulating);

    functions.CleanupDeleteList(0);
    
    RemoveBadEnts();

    SpawnPlayers();
    RemoveDanglingRestoredVehicles();
    EnterVehicles(restore_vehicle_list);

    if(savegame)
    {
        rootconsole->ConsolePrint("Saving game!");

        save_frames = 0;

        FixCarSlashes();

        saving_now = true;

        //Autosave_Silent
        pDynamicFastCallOneArgFunc = (pOneArgProtFastCall)(server_srv + 0x00BEC530);
        pDynamicFastCallOneArgFunc(0);

        saving_now = false;

        savegame = false;
    }

    UpdateAllCollisions();

    functions.CleanupDeleteList(0);

    //ReverseOrder
    pDynamicFastCallTwoArgFunc = (pTwoArgProtFastCall)(server_srv + 0x006E6080);
    pDynamicFastCallTwoArgFunc(0x2D, 0);

    //PostSystems
    pDynamicFastCallTwoArgFunc = (pTwoArgProtFastCall)(server_srv + 0x006E6350);
    pDynamicFastCallTwoArgFunc(0x41, 0);

    functions.CleanupDeleteList(0);
    
    RemoveBadEnts();

    functions.CleanupDeleteList(0);

    //ServiceEvents
    pDynamicOneArgFunc = (pOneArgProt)(server_srv + 0x00607AA0);
    pDynamicOneArgFunc(server_srv + 0x00EA2570);

    functions.CleanupDeleteList(0);

    RemoveBadEnts();

    CorrectPhysics();
    ReplicateCheatsOnClient();
    
    return 0;
}

uint32_t HooksSynergy::AutosaveHook(uint32_t arg0)
{
    savegame = true;
    return 0;
}

uint32_t HooksSynergy::fix_wheels_hook(uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    pThreeArgProt pDynamicThreeArgFunc;

    if(save_frames < 30)
    {
        rootconsole->ConsolePrint("Prevented vehicle exit!");
        return 0;
    }

    //rootconsole->ConsolePrint("Allowed usage!");
    
    pDynamicThreeArgFunc = (pThreeArgProt)(vphysics_srv + 0x000DC6F0);
    return pDynamicThreeArgFunc(arg0, arg1, arg2);
}

uint32_t HooksSynergy::SaveGameStateHook(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    pFourArgProt pDynamicFourArgFunc;

    MakePlayersLeaveVehicles();
    FixCarSlashes();

    functions.CleanupDeleteList(0);

    savegame_internal = true;

    pDynamicFourArgFunc = (pFourArgProt)(server_srv + 0x00BE5840);
    uint32_t returnVal = pDynamicFourArgFunc(arg0, arg1, arg2, arg3);

    savegame_internal = false;

    functions.CleanupDeleteList(0);

    EnterVehicles(save_player_vehicles_list);

    return returnVal;
}

uint32_t HooksSynergy::MountContentHook(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;
    pTwoArgProt pDynamicTwoArgFunc;

    //g_file_system   00278900

    rootconsole->ConsolePrint("\nStarting to mount content\n");

    pDynamicOneArgFunc = (pOneArgProt)(synergy_srv + 0x000440C0);
    uint32_t returnVal = pDynamicOneArgFunc(arg0);

    //PrintPaths
    pDynamicOneArgFunc = (pOneArgProt)(dedicated_srv + 0x000607D0);
    pDynamicOneArgFunc(dedicated_srv + 0x00278900);

    return returnVal;
}

void HookFunctionsSynergy()
{
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x00BEC530), (void*)HooksSynergy::AutosaveHook);
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x00BDC650), (void*)HooksSynergy::RestorePlayerHook);
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x00BE5840), (void*)HooksSynergy::SaveGameStateHook);

    HookFunction(synergy_srv, synergy_srv_size, (void*)(synergy_srv + 0x000440C0), (void*)HooksSynergy::MountContentHook);

    HookFunction(vphysics_srv, vphysics_srv_size, (void*)(vphysics_srv + 0x000DC6F0), (void*)HooksSynergy::fix_wheels_hook);
}