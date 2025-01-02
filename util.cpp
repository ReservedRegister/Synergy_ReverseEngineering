#include "extension.h"
#include "util.h"

#include <link.h>
#include <sys/mman.h>

game_fields fields;
game_offsets offsets;
game_functions functions;

bool loaded_extension;
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
int hooked_delete_counter;
int normal_delete_counter;
uint32_t global_vpk_cache_buffer;
uint32_t current_vpk_buffer_ref;

ValueList leakedResourcesVpkSystem;

void InitUtil()
{
    loaded_extension = false;
    firstplayer_hasjoined = false;
    player_collision_rules_changed = false;
    player_worldspawn_collision_disabled = false;
    isTicking = false;
    hooked_delete_counter = 0;
    normal_delete_counter = 0;
    server_sleeping = false;
    global_vpk_cache_buffer = (uint32_t)malloc(0x00100000);
    current_vpk_buffer_ref = 0;

    HookFunctionsUtil();
}

void HookFunctionsUtil()
{
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

    HookFunction(dedicated_srv, dedicated_srv_size, (void*)(functions.PackedStoreDestructor), (void*)HooksUtil::PackedStoreDestructorHook);
    HookFunction(dedicated_srv, dedicated_srv_size, (void*)(functions.CanSatisfyVpkCacheInternal), (void*)HooksUtil::CanSatisfyVpkCacheInternalHook);
}

uint32_t HooksUtil::EmptyCall()
{
    return 0;
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

    pDynamicOneArgFunc = (pOneArgProt)(functions.SpawnPlayer);
    uint32_t returnVal = pDynamicOneArgFunc(arg0);

    firstplayer_hasjoined = true;
    player_worldspawn_collision_disabled = false;

    return returnVal;
}

uint32_t HooksUtil::GlobalEntityListClear(uint32_t arg0)
{
    pOneArgProt pDynamicOneArgFunc;

    //LogVpkMemoryLeaks();

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

    if(*(uint32_t*)(fields.RemoveImmediateSemaphore) != 0)
        normal_delete_counter++;

    //rootconsole->ConsolePrint("normal counter: [%d] [%d] [%s]", normal_delete_counter, hooked_delete_counter, classname);

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
    InsertEntityToCollisionsList(arg0);
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

void HookFunction(uint32_t base_address, uint32_t size, void* target_pointer, void* hook_pointer)
{
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

bool IsEntityPositionReasonable(uint32_t v)
{
    float x = *(float*)(v);
    float y = *(float*)(v+4);
    float z = *(float*)(v+8);

    float r = 16384.0f;

    return
        x > -r && x < r &&
        y > -r && y < r &&
        z > -r && z < r;
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
    for(int i = 0; i < 512; i++)
    {
        if(collisions_entity_list[i] != 0)
        {
            uint32_t object = functions.GetCBaseEntity(collisions_entity_list[i]);

            if(IsEntityValid(object))
            {
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

                if(strcmp(classname, "player") == 0)
                {
                    ZeroVector(abs_origin);
                    ZeroVector(origin);
                    ZeroVector(abs_angles);
                    ZeroVector(abs_velocity);

                    continue;
                }

                rootconsole->ConsolePrint("Removed bad ent!");
                RemoveEntityNormal(ent, true);
            }
        }
    }
}

void RemoveEntityNormal(uint32_t entity_object, bool validate)
{
    pOneArgProt pDynamicOneArgFunc;

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
                functions.SpawnPlayer(object_verify);
                rootconsole->ConsolePrint("Tried killing player but was protected & respawned!");
                return;
            }
        }

        if(IsMarkedForDeletion(object_verify+offsets.iserver_offset)) return;

        functions.RemoveNormal(object_verify);

        if(IsMarkedForDeletion(object_verify+offsets.iserver_offset))
        {
            if(*(uint32_t*)(fields.RemoveImmediateSemaphore) != 0)
                hooked_delete_counter++;
        }

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