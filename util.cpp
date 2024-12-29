#include "extension.h"
#include "util.h"

game_fields fields;
game_offsets offsets;
game_functions functions;

ModuleMemory* modules[512] = {};

bool loaded_extension;
bool firstplayer_hasjoined;
bool player_collision_rules_changed;
bool player_worldspawn_collision_disabled;

int hooked_delete_counter;
int normal_delete_counter;

uint32_t hook_exclude_list_offset[512] = {};
uint32_t hook_exclude_list_base[512] = {};

ModuleMemory* server_dll;
ModuleMemory* engine_dll;

SIZE_T server_dll_size;
SIZE_T engine_dll_size;

uint32_t collisions_entity_list[512] = {};

void InitUtil()
{
	loaded_extension = false;
	firstplayer_hasjoined = false;
	player_collision_rules_changed = false;
	player_worldspawn_collision_disabled = false;
}

void PrintModuleMemoryToScreen(ModuleMemory* module_memory)
{
	MemoryPageSnap* page = module_memory->pages;

	while (page)
	{
		printf("%p %p %p\n", (void*)page->base_address, (void*)page->region_size, (void*)page->prot_flags);
		page = page->next_page;
	}
}

ModuleMemory* FindModule(const char* module_signature)
{
	int module_counter = 0;

	while (true)
	{
		ModuleMemory* module = modules[module_counter];

		if (!module)
			break;

		if (strstr(module->module_name, module_signature))
		{
			printf("Found [%s] at [%p]\n", module->module_name, module->module_base);
			return module;
		}

		module_counter++;
	}

	return NULL;
}

void ClearLoadedModules()
{
	int module_counter = 0;

	while (true)
	{
		ModuleMemory* module = modules[module_counter];

		if (!module)
			break;

		MemoryPageSnap* page = module->pages;

		while (page)
		{
			MemoryPageSnap* nextPage = page->next_page;

			free(page);
			page = nextPage;
		}

		free(module);
		modules[module_counter] = NULL;

		module_counter++;
	}
}

void AllowWriteToModules()
{
	int module_counter = 0;

	while (true)
	{
		ModuleMemory* module = modules[module_counter];

		if (!module)
			break;

		AllowWriteToModule(module);
		module_counter++;
	}
}

void AllowWriteToModule(ModuleMemory* module_memory)
{
	if (!module_memory)
		return;

	MemoryPageSnap* page = module_memory->pages;

	while (page)
	{
		DWORD oldProtect;
		if (!VirtualProtect(page->base_address, page->region_size, PAGE_EXECUTE_READWRITE, &oldProtect))
			printf("Failed to force memory protection!\n");

		printf("Forced access %p %p\n", page->base_address, page->region_size);

		page = page->next_page;
	}
}

void RestoreModulesMemory()
{
	int module_counter = 0;

	while (true)
	{
		ModuleMemory* module = modules[module_counter];

		if (!module)
			break;

		RestoreModuleMemory(module);
		module_counter++;
	}
}

void RestoreModuleMemory(ModuleMemory* module_memory)
{
	if (!module_memory)
		return;

	MemoryPageSnap* page = module_memory->pages;

	while (page)
	{
		DWORD oldProtect;
		if (!VirtualProtect(page->base_address, page->region_size, page->prot_flags, &oldProtect))
			printf("Failed to restore memory protection!\n");

		page = page->next_page;
	}
}

uint32_t FILEOFF(ModuleMemory* memory_module, uint32_t module_offset)
{
	return (uint32_t)memory_module->module_base + module_offset - memory_module->dll_load_base;
}

ModuleMemory* ReadModuleMemory(HMODULE hModule)
{
	if (!hModule)
	{
		printf("Invalid module handle.\n");
		return NULL;
	}

	MEMORY_BASIC_INFORMATION mbi;

	uintptr_t address = (uintptr_t)hModule;
	SIZE_T module_size = 0;

	ModuleMemory* new_list = AllocateModuleMemoryList(hModule);
	GetModuleFileName(hModule, new_list->module_name, MAX_PATH);

	while (VirtualQuery((LPCVOID)address, &mbi, sizeof(mbi)) == sizeof(mbi))
	{
		if (mbi.AllocationBase == hModule && mbi.State == MEM_COMMIT)
		{
			MemoryPageSnap* new_page = CreateMemoryPage(mbi.BaseAddress, mbi.RegionSize, mbi.Protect);
			InsertMemoryPageToList(new_list, new_page);

			module_size += mbi.RegionSize;
		}

		address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
	}

	new_list->module_size = module_size;
	return new_list;
}

ModuleMemory* AllocateModuleMemoryList(HMODULE module_in)
{
	ModuleMemory* new_list = (ModuleMemory*)malloc(sizeof(ModuleMemory));

	new_list->module_base = module_in;
	new_list->module_size = 0;
	new_list->pages = NULL;

	return new_list;
}

MemoryPageSnap* CreateMemoryPage(LPVOID base_address_in, SIZE_T region_size_in, DWORD prot_flags_in)
{
	MemoryPageSnap* page_snap = (MemoryPageSnap*)malloc(sizeof(MemoryPageSnap));

	page_snap->base_address = base_address_in;
	page_snap->region_size = region_size_in;
	page_snap->prot_flags = prot_flags_in;
	page_snap->next_page = NULL;

	return page_snap;
}

void InsertMemoryPageToList(ModuleMemory* pages_list, MemoryPageSnap* page_in)
{
	page_in->next_page = pages_list->pages;
	pages_list->pages = page_in;
}

void* copy_val(void* val, size_t copy_size)
{
	if (val == 0)
		return 0;

	void* copy_ptr = malloc(copy_size);
	memcpy(copy_ptr, val, copy_size);
	return copy_ptr;
}

bool IsAddressExcluded(uint32_t base_address, uint32_t search_address)
{
	for (int i = 0; i < 512; i++)
	{
		if (hook_exclude_list_offset[i] == 0 || hook_exclude_list_base[i] == 0)
			continue;

		uint32_t patch_address = base_address + hook_exclude_list_offset[i];

		if (patch_address == search_address && hook_exclude_list_base[i] == base_address)
			return true;
	}

	return false;
}

void HookFunction(uint32_t base_address, uint32_t size, void* target_pointer, void* hook_pointer)
{
	uint32_t search_address = base_address;
	uint32_t search_address_max = base_address + size;

	while (search_address + 3 < search_address_max)
	{
		uint32_t four_byte_addr = *(uint32_t*)(search_address);

		if (four_byte_addr == (uint32_t)target_pointer)
		{
			if (IsAddressExcluded(base_address, search_address))
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

		if (byte == 0xE8 || byte == 0xE9)
		{
			uint32_t call_address = *(uint32_t*)(search_address + 1);
			uint32_t chk = search_address + call_address + 5;

			if (chk == (uint32_t)target_pointer)
			{
				if (IsAddressExcluded(base_address, search_address))
				{
					rootconsole->ConsolePrint("(unsigned) Skipped patch at [%X]", search_address);
					search_address++;
					continue;
				}

				//rootconsole->ConsolePrint("(unsigned) Hooked address: [%X]", search_address - (uint32_t)base_address);
				uint32_t offset = (uint32_t)hook_pointer - search_address - 5;
				*(uint32_t*)(search_address + 1) = offset;
			}
			else
			{
				//check signed addition
				chk = search_address + (int32_t)call_address + 5;

				if (chk == (uint32_t)target_pointer)
				{
					if (IsAddressExcluded(base_address, search_address))
					{
						rootconsole->ConsolePrint("(signed) Skipped patch at [%X]", search_address);
						search_address++;
						continue;
					}

					rootconsole->ConsolePrint("(signed) Hooked address: [%X]", search_address - base_address);
					uint32_t offset = (uint32_t)hook_pointer - search_address - 5;
					*(uint32_t*)(search_address + 1) = offset;
				}
			}
		}

		search_address++;
	}
}

bool IsMarkedForDeletion(uint32_t arg0)
{
	if (*(uint8_t*)(arg0 + offsets.ismarked_offset) & 1)
		return false;

	return true;
}

void ZeroVector(uint32_t vector)
{
	*(float*)(vector) = 0;
	*(float*)(vector + 4) = 0;
	*(float*)(vector + 8) = 0;
}

bool IsVectorNaN(uint32_t base)
{
	float s0 = *(float*)(base);
	float s1 = *(float*)(base + 4);
	float s2 = *(float*)(base + 8);

	if (s0 != s0 || s1 != s1 || s2 != s2)
		return true;

	return false;
}

bool IsEntityPositionReasonable(uint32_t v)
{
	float x = *(float*)(v);
	float y = *(float*)(v + 4);
	float z = *(float*)(v + 8);

	float r = 16384.0f;

	return
		x > -r && x < r &&
		y > -r && y < r &&
		z > -r && z < r;
}

void InsertEntityToCollisionsList(uint32_t ent)
{
	if (IsEntityValid(ent))
	{
		char* classname = (char*)(*(uint32_t*)(ent + offsets.classname_offset));

		if (classname && strcmp(classname, "player") == 0)
		{
			player_collision_rules_changed = true;
			return;
		}

		for (int i = 0; i < 512; i++)
		{
			if (collisions_entity_list[i] != 0)
			{
				uint32_t refHandle = *(uint32_t*)(ent + offsets.refhandle_offset);

				if (refHandle == collisions_entity_list[i])
					return;
			}
		}

		for (int i = 0; i < 512; i++)
		{
			if (collisions_entity_list[i] == 0)
			{
				uint32_t refHandle = *(uint32_t*)(ent + offsets.refhandle_offset);
				collisions_entity_list[i] = refHandle;

				break;
			}
		}
	}
}

void UpdateAllCollisions()
{
	for (int i = 0; i < 512; i++)
	{
		if (collisions_entity_list[i] != 0)
		{
			uint32_t object = functions.GetCBaseEntity(collisions_entity_list[i]);

			if (IsEntityValid(object))
			{
				functions.CollisionRulesChanged(object);
			}

			collisions_entity_list[i] = 0;
		}
	}

	uint32_t ent = 0;

	while ((ent = functions.FindEntityByClassname(fields.CGlobalEntityList, ent, (uint32_t)"*")) != 0)
	{
		if (IsEntityValid(ent))
		{
			uint32_t m_Network = *(uint32_t*)(ent + offsets.mnetwork_offset);

			if (!m_Network)
			{
				functions.CollisionRulesChanged(ent);
			}
		}
	}

	ent = 0;

	while ((ent = functions.FindEntityByClassname(fields.CGlobalEntityList, ent, (uint32_t)"player")) != 0)
	{
		if (IsEntityValid(ent))
		{
			functions.CollisionRulesChanged(ent);
		}
	}

	RemoveBadEnts();
}

void FixPlayerCollisionGroup()
{
	uint32_t player = 0;

	while ((player = functions.FindEntityByClassname(fields.CGlobalEntityList, player, (uint32_t)"player")) != 0)
	{
		if (IsEntityValid(player))
		{
			uint32_t collision_flags = *(uint32_t*)(player + offsets.m_CollisionGroup_offset);

			if (collision_flags & 4)
			{
				*(uint32_t*)(player + offsets.m_CollisionGroup_offset) -= 4;
			}

			if (!(collision_flags & 2))
			{
				*(uint32_t*)(player + offsets.m_CollisionGroup_offset) += 2;
			}
		}
	}
}

void DisablePlayerWorldSpawnCollision()
{
	uint32_t player = 0;

	while ((player = functions.FindEntityByClassname(fields.CGlobalEntityList, player, (uint32_t)"player")) != 0)
	{
		uint32_t worldspawn = functions.FindEntityByClassname(fields.CGlobalEntityList, 0, (uint32_t)"worldspawn");

		if (IsEntityValid(worldspawn) && IsEntityValid(player))
		{
			float player_velocity_x = *(uint32_t*)(player + offsets.abs_velocity_offset);
			float player_velocity_y = *(uint32_t*)(player + offsets.abs_velocity_offset + 4);
			float player_velocity_z = *(uint32_t*)(player + offsets.abs_velocity_offset + 8);

			if (player_velocity_x > 3260000000.0 || player_velocity_y > 3260000000.0)
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

	while ((current_player = functions.FindEntityByClassname(fields.CGlobalEntityList, current_player, (uint32_t)"player")) != 0)
	{
		if (IsEntityValid(current_player))
		{
			uint32_t other_players = 0;

			while ((other_players = functions.FindEntityByClassname(fields.CGlobalEntityList, other_players, (uint32_t)"player")) != 0)
			{
				if (IsEntityValid(other_players) && other_players != current_player)
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

	while ((ent = functions.FindEntityByClassname(fields.CGlobalEntityList, ent, (uint32_t)"*")) != 0)
	{
		if (IsEntityValid(ent))
		{
			uint32_t abs_origin = ent + offsets.abs_origin_offset;
			uint32_t origin = ent + offsets.origin_offset;
			uint32_t abs_angles = ent + offsets.abs_angles_offset;
			uint32_t abs_velocity = ent + offsets.abs_velocity_offset;

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
				char* classname = (char*)(*(uint32_t*)(ent + offsets.classname_offset));

				if (strcmp(classname, "player") == 0)
				{
					ZeroVector(abs_origin);
					ZeroVector(origin);
					ZeroVector(abs_angles);
					ZeroVector(abs_velocity);

					continue;
				}

				rootconsole->ConsolePrint("Removed bad ent!");
				functions.RemoveEntityNormal(ent, true);
			}
		}
	}
}

uint32_t IsEntityValid(uint32_t entity)
{
	pOneArgProt pDynamicOneArgFunc;
	if (entity == 0) return entity;

	uint32_t object = functions.GetCBaseEntity(*(uint32_t*)(entity + offsets.refhandle_offset));

	if (object)
	{
		uint32_t isMarked = IsMarkedForDeletion(object + offsets.iserver_offset);

		if (isMarked)
		{
			return 0;
		}

		return entity;
	}

	return 0;
}

ValueList AllocateValuesList()
{
	ValueList list = (ValueList)malloc(sizeof(ValueList));
	*list = NULL;
	return list;
}

Value* CreateNewValue(void* valueInput)
{
	Value* val = (Value*)malloc(sizeof(Value));

	val->value = valueInput;
	val->nextVal = NULL;
	return val;
}

int DeleteAllValuesInList(ValueList list, bool free_val, CRITICAL_SECTION* lockInput)
{
	if (lockInput)
	{
		while (TryEnterCriticalSection(lockInput) == FALSE);
	}

	int removed_items = 0;

	if (!list || !*list)
	{

		if (lockInput)
		{
			LeaveCriticalSection(lockInput);
		}

		return removed_items;
	}

	Value* aValue = *list;

	while (aValue)
	{
		Value* detachedValue = aValue->nextVal;
		if (free_val) free(aValue->value);
		free(aValue);
		aValue = detachedValue;

		removed_items++;
	}

	*list = NULL;

	if (lockInput)
	{
		LeaveCriticalSection(lockInput);
	}

	return removed_items;
}

bool IsInValuesList(ValueList list, void* searchVal, CRITICAL_SECTION* lockInput)
{
	if (lockInput)
	{
		while (TryEnterCriticalSection(lockInput) == FALSE);
	}

	Value* aValue = *list;

	while (aValue)
	{
		if ((uint32_t)aValue->value == (uint32_t)searchVal)
		{
			if (lockInput)
			{
				LeaveCriticalSection(lockInput);
			}

			return true;
		}

		aValue = aValue->nextVal;
	}

	if (lockInput)
	{
		LeaveCriticalSection(lockInput);
	}

	return false;
}

bool RemoveFromValuesList(ValueList list, void* searchVal, CRITICAL_SECTION* lockInput)
{
	if (lockInput)
	{
		while (TryEnterCriticalSection(lockInput) == FALSE);
	}

	Value* aValue = *list;

	if (aValue == NULL)
	{
		if (lockInput)
		{
			LeaveCriticalSection(lockInput);
		}

		return false;
	}

	//search at the start of the list
	if (((uint32_t)aValue->value) == ((uint32_t)searchVal))
	{
		Value* detachedValue = aValue->nextVal;
		free(*list);
		*list = detachedValue;

		if (lockInput)
		{
			LeaveCriticalSection(lockInput);
		}

		return true;
	}

	//search the rest of the list
	while (aValue->nextVal)
	{
		if (((uint32_t)aValue->nextVal->value) == ((uint32_t)searchVal))
		{
			Value* detachedValue = aValue->nextVal->nextVal;

			free(aValue->nextVal);
			aValue->nextVal = detachedValue;

			if (lockInput)
			{
				LeaveCriticalSection(lockInput);
			}

			return true;
		}

		aValue = aValue->nextVal;
	}

	if (lockInput)
	{
		LeaveCriticalSection(lockInput);
	}

	return false;
}

int ValueListItems(ValueList list, CRITICAL_SECTION* lockInput)
{
	if (lockInput)
	{
		while (TryEnterCriticalSection(lockInput) == FALSE);
	}

	Value* aValue = *list;
	int counter = 0;

	while (aValue)
	{
		counter++;
		aValue = aValue->nextVal;
	}

	if (lockInput)
	{
		LeaveCriticalSection(lockInput);
	}

	return counter;
}

bool InsertToValuesList(ValueList list, Value* head, CRITICAL_SECTION* lockInput, bool tail, bool duplicate_chk)
{
	if (lockInput)
	{
		while (TryEnterCriticalSection(lockInput) == FALSE);
	}

	if (duplicate_chk)
	{
		Value* aValue = *list;

		while (aValue)
		{
			if ((uint32_t)aValue->value == (uint32_t)head->value)
			{
				if (lockInput)
				{
					LeaveCriticalSection(lockInput);
				}

				return false;
			}

			aValue = aValue->nextVal;
		}
	}

	if (tail)
	{
		Value* aValue = *list;

		while (aValue)
		{
			if (aValue->nextVal == NULL)
			{
				aValue->nextVal = head;

				if (lockInput)
				{
					LeaveCriticalSection(lockInput);
				}

				return true;
			}

			aValue = aValue->nextVal;
		}
	}

	head->nextVal = *list;
	*list = head;

	if (lockInput)
	{
		LeaveCriticalSection(lockInput);
	}

	return true;
}