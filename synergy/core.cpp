#include "extension.h"
#include "util.h"
#include "core.h"
#include "ext_main.h"

bool disable_internal_remove_incrementor;

bool InitCoreSynergy()
{
	disable_internal_remove_incrementor = false;

	bool loaded = true;
	int module_counter = 0;

	HMODULE server_module = GetModuleHandle("server.dll");
	HMODULE engine_module = GetModuleHandle("engine.dll");

	ModuleMemory* server_memory = ReadModuleMemory(server_module);
	ModuleMemory* engine_memory = ReadModuleMemory(engine_module);

	if (server_memory)
	{
		server_memory->dll_load_base = 0x10000000;
		server_memory->dll_rva = 0xC00;

		modules[module_counter] = server_memory;
		module_counter++;
	}
	else
		loaded = false;

	if (engine_memory)
	{
		engine_memory->dll_load_base = 0x10000000;
		engine_memory->dll_rva = 0xC00;

		modules[module_counter] = engine_memory;
		module_counter++;
	}
	else
		loaded = false;

	return loaded;
}

void PopulateHookExclusionListsSynergy()
{

}

uint32_t GetCBaseEntitySynergy(uint32_t EHandle)
{
	uint32_t EntityList = fields.CGlobalEntityList;
	uint32_t refHandle = (EHandle & 0xFFF) << 4;

	EHandle = EHandle >> 0x0C;

	if (*(uint32_t*)(EntityList + refHandle + 8) == EHandle)
	{
		uint32_t CBaseEntity = *(uint32_t*)(EntityList + refHandle + 4);
		return CBaseEntity;
	}

	return 0;
}

void InstaKillSynergy(uint32_t entity_object, bool validate)
{
	pZeroArgProt pDynamicZeroArgFunc;
	pOneArgProt pDynamicOneArgFunc;
	pTwoArgProt pDynamicTwoArgFunc;

	if (entity_object == 0) return;

	uint32_t refHandleInsta = *(uint32_t*)(entity_object + offsets.refhandle_offset);
	char* classname = (char*)(*(uint32_t*)(entity_object + offsets.classname_offset));
	uint32_t cbase_chk = functions.GetCBaseEntity(refHandleInsta);

	if (cbase_chk == 0)
	{
		if (!validate)
		{
			if (classname)
			{
				rootconsole->ConsolePrint("Warning: Entity delete request granted without validation! [%s]", classname);
			}
			else
			{
				rootconsole->ConsolePrint("Warning: Entity delete request granted without validation!");
			}

			cbase_chk = entity_object;
		}
		else
		{
			rootconsole->ConsolePrint("\n\nFailed to verify entity for fast kill\n\n");
			exit(EXIT_FAILURE);
			return;
		}
	}

	uint32_t isMarked = IsMarkedForDeletion(cbase_chk + offsets.iserver_offset);

	if (isMarked)
	{
		rootconsole->ConsolePrint("Attempted to kill an entity twice in UTIL_RemoveImmediate(CBaseEntity*)");
		return;
	}

	if ((*(uint32_t*)(cbase_chk + offsets.ismarked_offset) & 1) == 0)
	{
		if (*(uint32_t*)(fields.RemoveImmediateSemaphore) == 0)
		{
			// FAST DELETE ONLY

			//if(classname) rootconsole->ConsolePrint("Removed [%s]", classname);

			*(uint32_t*)(cbase_chk + offsets.ismarked_offset) = *(uint32_t*)(cbase_chk + offsets.ismarked_offset) | 1;

			//UpdateOnRemove
			pDynamicOneArgFunc = (pOneArgProt)(*(uint32_t*)((*(uint32_t*)(cbase_chk)) + offsets.updateonremove_offset));
			pDynamicOneArgFunc(cbase_chk);

			//CALL RELEASE
			uint32_t iServerObj = cbase_chk + offsets.iserver_offset;

			pDynamicOneArgFunc = (pOneArgProt)(*(uint32_t*)((*(uint32_t*)(iServerObj)) + offsets.entity_release_offset));
			pDynamicOneArgFunc(iServerObj);
		}
		else
		{
			RemoveEntityNormalSynergy(cbase_chk, validate);
		}
	}
}

void RemoveEntityNormalSynergy(uint32_t entity_object, bool validate)
{
	pOneArgProt pDynamicOneArgFunc;

	if (entity_object == 0)
	{
		rootconsole->ConsolePrint("Could not kill entity [NULL]");
		return;
	}

	char* classname = (char*)(*(uint32_t*)(entity_object + offsets.classname_offset));
	uint32_t m_refHandle = *(uint32_t*)(entity_object + offsets.refhandle_offset);
	uint32_t chk_ref = functions.GetCBaseEntity(m_refHandle);

	if (chk_ref == 0)
	{
		if (!validate)
		{
			if (classname)
			{
				rootconsole->ConsolePrint("Warning: Entity delete request granted without validation! [%s]", classname);
			}
			else
			{
				rootconsole->ConsolePrint("Warning: Entity delete request granted without validation!");
			}

			chk_ref = entity_object;
		}
	}

	if (chk_ref)
	{
		if (classname && strcmp(classname, "player") == 0)
		{
			rootconsole->ConsolePrint(EXT_PREFIX "Tried killing player but was protected!");
			return;
		}

		//Check if entity was already removed!
		uint32_t isMarked = IsMarkedForDeletion(chk_ref + offsets.iserver_offset);

		if (isMarked) return;

		//if(classname) rootconsole->ConsolePrint("Removed [%s]", classname);

		//UTIL_Remove(IServerNetworkable*)
		pDynamicOneArgFunc = (pOneArgProt)(FILEOFF(server_dll, 0x101E4DE0));
		pDynamicOneArgFunc(chk_ref + offsets.iserver_offset);

		//Check if entity was removed!
		isMarked = IsMarkedForDeletion(chk_ref + offsets.iserver_offset);

		if (isMarked)
		{
			if (*(uint32_t*)(fields.RemoveImmediateSemaphore) != 0)
			{
				if (disable_internal_remove_incrementor)
					hooked_delete_counter++;
			}
		}

		return;
	}

	rootconsole->ConsolePrint(EXT_PREFIX "Could not kill entity [Invalid Ehandle] [%X]");
	exit(EXIT_FAILURE);
}