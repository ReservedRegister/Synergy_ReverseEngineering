#include "extension.h"
#include "util.h"
#include "core.h"
#include "ext_main.h"
#include "hooks_specific.h"

bool InitExtensionBlackMesa()
{
    if(loaded_extension)
    {
        rootconsole->ConsolePrint("Attempted to load extension twice!");
        return false;
    }

    InitCoreBlackMesa();
    AllowWriteToMappedMemory();

    char* root_dir = getenv("PWD");
    size_t max_path_length = 1024;

    char server_srv_fullpath[max_path_length];
    char engine_srv_fullpath[max_path_length];
    char vphysics_srv_fullpath[max_path_length];
    char dedicated_srv_fullpath[max_path_length];

    snprintf(server_srv_fullpath, max_path_length, "/bms/bin/server_srv.so");
    snprintf(engine_srv_fullpath, max_path_length, "/bin/engine_srv.so");
    snprintf(vphysics_srv_fullpath, max_path_length, "/bin/vphysics_srv.so");
    snprintf(dedicated_srv_fullpath, max_path_length, "/bin/dedicated_srv.so");

    Library* server_srv_lib = FindLibrary(server_srv_fullpath, true);
    Library* engine_srv_lib = FindLibrary(engine_srv_fullpath, true);
    Library* vphysics_srv_lib = FindLibrary(vphysics_srv_fullpath, true);
    Library* dedicated_srv_lib = FindLibrary(dedicated_srv_fullpath, true);

    if(!(engine_srv_lib && server_srv_lib && vphysics_srv_lib && dedicated_srv_lib))
    {
        RestoreMemoryProtections();
        ClearLoadedLibraries();
        rootconsole->ConsolePrint("----------------------  Failed to load Black Mesa " SMEXT_CONF_NAME " " SMEXT_CONF_VERSION "  ----------------------");
        return false;
    }

    game = BLACK_MESA;

    server_srv = server_srv_lib->library_base_address;
    engine_srv = engine_srv_lib->library_base_address;
    vphysics_srv = vphysics_srv_lib->library_base_address;
    dedicated_srv = dedicated_srv_lib->library_base_address;

    server_srv_size = server_srv_lib->library_size;
    engine_srv_size = engine_srv_lib->library_size;
    vphysics_srv_size = vphysics_srv_lib->library_size;
    dedicated_srv_size = dedicated_srv_lib->library_size;

    last_ragdoll_gib = 0;
    ragdoll_breaking_gib_counter = 0;
    is_currently_ragdoll_breaking = false;

    leakedResourcesVpkSystem = AllocateValuesList();

    fields.CGlobalEntityList = server_srv + 0x017B6BE0;
    fields.sv = engine_srv + 0x00315E80;
    fields.RemoveImmediateSemaphore = server_srv + 0x01811920;
    fields.sv_cheats_cvar = engine_srv + 0x00315BA0;
    fields.deferMindist = vphysics_srv + 0x001B5CA0;

    offsets.classname_offset = 0x64;
    offsets.abs_origin_offset = 0x294;
    offsets.abs_angles_offset = 0x310;
    offsets.abs_velocity_offset = 0x22C;
    offsets.origin_offset = 0x31C;
    offsets.mnetwork_offset = 0x20;
    offsets.refhandle_offset = 0x334;
    offsets.iserver_offset = 0x14;
    offsets.collision_property_offset = 0x160;
    offsets.ismarked_offset = 0x118;
    offsets.vphysics_object_offset = 0x1F8;
    offsets.m_CollisionGroup_offset = 500;

    functions.GetCBaseEntity = (pOneArgProt)(GetCBaseEntityBlackMesa);
    functions.SpawnPlayer = (pOneArgProt)(server_srv + 0x005983C0);
    functions.RemoveNormalDirect = (pOneArgProt)(server_srv + 0x00A92160);
    functions.RemoveNormal = (pOneArgProt)(server_srv + 0x00A921F0);
    functions.RemoveInsta = (pOneArgProt)(server_srv + 0x00A92260);
    functions.CreateEntityByName = (pTwoArgProt)(server_srv + 0x008B4C80);
    functions.PhysSimEnt = (pOneArgProt)(server_srv + 0x009919D0);
    functions.AcceptInput = (pSixArgProt)(server_srv + 0x004F5B50);
    functions.UpdateOnRemoveBase = (pOneArgProt)(server_srv + 0x004FD670);
    functions.VphysicsSetObject = (pOneArgProt)(server_srv + 0x00294D00);
    functions.ClearAllEntities = (pOneArgProt)(server_srv + 0x007E9040);
    functions.SetSolidFlags = (pTwoArgProt)(server_srv + 0x00336C60);
    functions.DisableEntityCollisions = (pTwoArgProt)(server_srv + 0x00379460);
    functions.EnableEntityCollisions = (pTwoArgProt)(server_srv + 0x003794D0);
    functions.CollisionRulesChanged = (pOneArgProt)(server_srv + 0x00294C60);
    functions.FindEntityByClassname = (pThreeArgProt)(server_srv + 0x007E7030);
    functions.CleanupDeleteList = (pOneArgProt)(server_srv + 0x007E6D20);
    functions.PackedStoreDestructor = (pOneArgProt)(dedicated_srv + 0x000B1AE0);
    functions.CanSatisfyVpkCacheInternal = (pSevenArgProt)(dedicated_srv + 0x000B5460);
    functions.AddSearchPath = (pFourArgProt)(dedicated_srv + 0x00052390);
    functions.SV_ReplicateConVarChange = (pTwoArgProt)(engine_srv + 0x0016A6C0);

    PopulateHookExclusionListsBlackMesa();

    InitUtil();

    ApplyPatchesBlackMesa();
    ApplyPatchesSpecificBlackMesa();
    
    HookFunctionsBlackMesa();
    HookFunctionsSpecificBlackMesa();

    RestoreMemoryProtections();

    rootconsole->ConsolePrint("----------------------  Black Mesa " SMEXT_CONF_NAME " " SMEXT_CONF_VERSION " loaded!" "  ----------------------");
    loaded_extension = true;
    return true;
}

void ApplyPatchesBlackMesa()
{
    uint32_t offset = 0;

    uint32_t patch_ragdoll_break_create = server_srv + 0x009FD863;
    offset = (uint32_t)HooksBlackMesa::CreateNoSpawnHookRagdollBreaking - patch_ragdoll_break_create - 5;
    *(uint32_t*)(patch_ragdoll_break_create+1) = offset;

    uint32_t patch_vpk_cache_buffer = dedicated_srv + 0x000B57D2;
    memset((void*)patch_vpk_cache_buffer, 0x90, 0x17);
    *(uint8_t*)(patch_vpk_cache_buffer) = 0x89;
    *(uint8_t*)(patch_vpk_cache_buffer+1) = 0x34;
    *(uint8_t*)(patch_vpk_cache_buffer+2) = 0x24;

    patch_vpk_cache_buffer = dedicated_srv + 0x000B57D2+3;
    offset = (uint32_t)HooksBlackMesa::VpkCacheBufferAllocHook - patch_vpk_cache_buffer - 5;
    *(uint8_t*)(patch_vpk_cache_buffer) = 0xE8;
    *(uint32_t*)(patch_vpk_cache_buffer+1) = offset;

    uint32_t force_jump_vpk_allocation = dedicated_srv + 0x000B5736;
    memset((void*)force_jump_vpk_allocation, 0x90, 6);
    *(uint8_t*)(force_jump_vpk_allocation) = 0xE9;
    *(uint32_t*)(force_jump_vpk_allocation+1) = 0x97;

    uint32_t hook_game_frame_delete_list = server_srv + 0x008404F3;
    offset = (uint32_t)HooksBlackMesa::SimulateEntitiesHook - hook_game_frame_delete_list - 5;
    *(uint32_t*)(hook_game_frame_delete_list+1) = offset;

    uint32_t eventqueue_hook = server_srv + 0x008404FD;
    offset = (uint32_t)HooksUtil::EmptyCall - eventqueue_hook - 5;
    *(uint32_t*)(eventqueue_hook+1) = offset;

    uint32_t remove_post_systems = server_srv + 0x008404F8;
    offset = (uint32_t)HooksUtil::EmptyCall - remove_post_systems - 5;
    *(uint32_t*)(remove_post_systems+1) = offset;

    uint32_t precache_other_patch = server_srv + 0x00A96061;
    offset = (uint32_t)HooksBlackMesa::InstaKillPatchHook - precache_other_patch - 5;
    *(uint32_t*)(precache_other_patch+1) = offset;

    uint32_t fix_vphysics_pair_crash = server_srv + 0x00378906;
    *(uint8_t*)(fix_vphysics_pair_crash) = 0xEB;

    uint32_t fix_localplayer_crash = server_srv + 0x0042C421;
    memset((void*)fix_localplayer_crash, 0x90, 6);
    *(uint8_t*)(fix_localplayer_crash) = 0xE9;
    *(uint32_t*)(fix_localplayer_crash+1) = 0x2844;

    uint32_t fix_vphysics_destructor = vphysics_srv + 0x0002B035;
    *(uint8_t*)(fix_vphysics_destructor) = 0xEB;

    //CMessageEntity
    uint32_t remove_extra_call = server_srv + 0x008BB59A;
    offset = (uint32_t)HooksUtil::EmptyCall - remove_extra_call - 5;
    *(uint32_t*)(remove_extra_call+1) = offset;
}

void HookFunctionsBlackMesa()
{
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x0078FC70), (void*)HooksBlackMesa::RagdollBreakHook);
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x0061B4C0), (void*)HooksBlackMesa::CXenShieldController_UpdateOnRemoveHook);
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x00A92540), (void*)HooksBlackMesa::UTIL_GetLocalPlayerHook);
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x0046B510), (void*)HooksBlackMesa::TestGroundMove);
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x00A02D40), (void*)HooksBlackMesa::ShouldHitEntityHook);
    HookFunction(server_srv, server_srv_size, (void*)(server_srv + 0x00295760), (void*)HooksBlackMesa::VPhysicsUpdateHook);
}

void CorrectVphysicsEntity(uint32_t ent)
{
    pThreeArgProt pDynamicThreeArgFunc;
    pFourArgProt pDynamicFourArgFunc;

    if(IsEntityValid(ent))
    {
        uint32_t vphysics_object = *(uint32_t*)(ent+0x1F8);

        if(vphysics_object)
        {
            Vector current_origin;
            Vector current_angles;
            Vector empty_vector;

            //GetPosition
            pDynamicThreeArgFunc = (pThreeArgProt)(  *(uint32_t*)((*(uint32_t*)(vphysics_object))+0xC0)  );
            pDynamicThreeArgFunc(vphysics_object, (uint32_t)&current_origin, (uint32_t)&current_angles);

            //rootconsole->ConsolePrint("%f %f %f", current_angles.x, current_angles.y, current_angles.z);

            if(!IsEntityPositionReasonable((uint32_t)&current_origin) && !IsEntityPositionReasonable((uint32_t)&current_angles))
            {
                //SetPosition
                pDynamicFourArgFunc = (pFourArgProt)(  *(uint32_t*)((*(uint32_t*)(vphysics_object))+0xB8)  );
                pDynamicFourArgFunc(vphysics_object, (uint32_t)&empty_vector, (uint32_t)&empty_vector, 1);

                rootconsole->ConsolePrint("Corrected vphysics origin & angles!");

                return;
            }

            if(!IsEntityPositionReasonable((uint32_t)&current_origin))
            {
                //SetPosition
                pDynamicFourArgFunc = (pFourArgProt)(  *(uint32_t*)((*(uint32_t*)(vphysics_object))+0xB8)  );
                pDynamicFourArgFunc(vphysics_object, (uint32_t)&empty_vector, (uint32_t)&current_angles, 1);

                rootconsole->ConsolePrint("Corrected vphysics origin!");
            }

            if(!IsEntityPositionReasonable((uint32_t)&current_angles))
            {
                //SetPosition
                pDynamicFourArgFunc = (pFourArgProt)(  *(uint32_t*)((*(uint32_t*)(vphysics_object))+0xB8)  );
                pDynamicFourArgFunc(vphysics_object, (uint32_t)&current_origin, (uint32_t)&empty_vector, 1);

                rootconsole->ConsolePrint("Corrected vphysics angles!");
            }
        }
    }
}

uint32_t HooksBlackMesa::VPhysicsUpdateHook(uint32_t arg0, uint32_t arg1)
{
    pTwoArgProt pDynamicTwoArgFunc;

    if(IsEntityValid(arg0))
    {
        CorrectVphysicsEntity(arg0);

        pDynamicTwoArgFunc = (pTwoArgProt)(server_srv + 0x00295760);
        return pDynamicTwoArgFunc(arg0, arg1);
    }

    rootconsole->ConsolePrint("Entity was invalid in vphysics update!");
    return 0;
}

uint32_t HooksBlackMesa::RagdollBreakHook(uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    pThreeArgProt pDynamicThreeArgFunc;

    if(IsEntityValid(arg0))
    {
        ragdoll_breaking_gib_counter = 0;
        is_currently_ragdoll_breaking = true;

        pDynamicThreeArgFunc = (pThreeArgProt)(server_srv + 0x0078FC70);
        pDynamicThreeArgFunc(arg0, arg1, arg2);

        is_currently_ragdoll_breaking = false;
        RemoveEntityNormal(last_ragdoll_gib, true);
        last_ragdoll_gib = 0;

        if(IsEntityValid(arg0) == 0)
        {
            rootconsole->ConsolePrint("Runtime error - failed to maintain entity integrity! (Ragdoll breaking)");
            exit(EXIT_FAILURE);
            return 0;
        }
    }

    rootconsole->ConsolePrint("Failed to ragdoll break!");
    return 0;
}

uint32_t HooksBlackMesa::CreateNoSpawnHookRagdollBreaking(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    pFourArgProt pDynamicFourArgFunc;

    if(is_currently_ragdoll_breaking && ragdoll_breaking_gib_counter > 8)
    {
        rootconsole->ConsolePrint("Ignored gib from ragdoll breaker!");

        if(IsEntityValid(last_ragdoll_gib))
        {
            return last_ragdoll_gib;
        }

        rootconsole->ConsolePrint("First gib was removed!!! - Critical Error");
        exit(EXIT_FAILURE);
        return 0;
    }

    pDynamicFourArgFunc = (pFourArgProt)(server_srv + 0x004FA240);
    uint32_t new_object = pDynamicFourArgFunc(arg0, arg1, arg2, arg3);

    if(IsEntityValid(new_object))
    {
        if(is_currently_ragdoll_breaking)
        {
            if(ragdoll_breaking_gib_counter == 8)
            {
                last_ragdoll_gib = new_object;
            }

            ragdoll_breaking_gib_counter++;
        }

        return new_object;
    }

    rootconsole->ConsolePrint("Failed to add entity to ragdoll created break list!");
    exit(EXIT_FAILURE);
    return 0;
}

uint32_t HooksBlackMesa::VpkCacheBufferAllocHook(uint32_t arg0)
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

uint32_t HooksBlackMesa::ShouldHitEntityHook(uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    pOneArgProt pDynamicOneArgFunc;
    pThreeArgProt pDynamicThreeArgFunc;

    if(arg1)
    {
        pDynamicOneArgFunc = (pOneArgProt)( *(uint32_t*)((*(uint32_t*)(arg1))+0x18) );
        uint32_t object = pDynamicOneArgFunc(arg1);

        if(IsEntityValid(object))
        {
            uint32_t vphysics_object = *(uint32_t*)(object+offsets.vphysics_object_offset);

            if(vphysics_object)
            {
                pDynamicThreeArgFunc = (pThreeArgProt)(server_srv + 0x00A02D40);
                return pDynamicThreeArgFunc(arg0, arg1, arg2);
            }
        }
    }

    rootconsole->ConsolePrint("ShouldHitEntity failed!");
    return 0;
}

uint32_t HooksBlackMesa::UTIL_GetLocalPlayerHook()
{
    pZeroArgProt pDynamicZeroArgProt;

    pDynamicZeroArgProt = (pZeroArgProt)(server_srv + 0x00A92540);
    uint32_t returnVal = pDynamicZeroArgProt();

    if(!returnVal)
        return functions.FindEntityByClassname(fields.CGlobalEntityList, 0, (uint32_t)"player");

    return returnVal;
}

uint32_t HooksBlackMesa::InstaKillPatchHook(uint32_t arg0)
{
    InstaKill(arg0, false);
    return 0;
}

uint32_t HooksBlackMesa::CXenShieldController_UpdateOnRemoveHook(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;

    pDynamicOneArgFunc = (pOneArgProt)(server_srv + 0x0061B4C0);
    uint32_t returnVal = pDynamicOneArgFunc(arg0);

    //Add missing call to UpdateOnRemove
    functions.UpdateOnRemoveBase(arg0);

    return returnVal;
}

uint32_t HooksBlackMesa::SimulateEntitiesHook(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;
    isTicking = true;

    functions.CleanupDeleteList(0);

    uint32_t firstPlayer = functions.FindEntityByClassname(fields.CGlobalEntityList, 0, (uint32_t)"player");

    if(!firstPlayer)
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
    pDynamicOneArgFunc = (pOneArgProt)(server_srv + 0x00991F80);
    pDynamicOneArgFunc(arg0);

    functions.CleanupDeleteList(0);

    RemoveBadEnts();

    SpawnPlayers();
    FixPlayerCollisionGroup();
    DisablePlayerWorldSpawnCollision();

    UpdateAllCollisions();

    functions.CleanupDeleteList(0);

    //PostSystems
    pDynamicOneArgFunc = (pOneArgProt)(server_srv + 0x0035C740);
    pDynamicOneArgFunc(0);

    functions.CleanupDeleteList(0);

    RemoveBadEnts();

    functions.CleanupDeleteList(0);

    //ServiceEventQueue
    pDynamicOneArgFunc = (pOneArgProt)(server_srv + 0x007B92B0);
    pDynamicOneArgFunc(0);

    functions.CleanupDeleteList(0);

    RemoveBadEnts();

    CorrectPhysics();
    ReplicateCheatsOnClient();

    return 0;
}

uint32_t HooksBlackMesa::TestGroundMove(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6)
{
    pSevenArgProt pDynamicSevenArgProt;

    if(arg6)
    {
        float inf_val_chk = *(float*)(arg6+0x20);

        if(isinf(inf_val_chk))
        {
            rootconsole->ConsolePrint("+Inf detected!");
            return 0;
        }
    }

    pDynamicSevenArgProt = (pSevenArgProt)(server_srv + 0x0046B510);
    return pDynamicSevenArgProt(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
}