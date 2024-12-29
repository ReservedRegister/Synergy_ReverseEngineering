#include "extension.h"
#include "util.h"
#include "core.h"
#include "ext_main.h"
#include "hooks_specific.h"

bool InitExtensionSynergy()
{
	if (loaded_extension)
	{
		rootconsole->ConsolePrint("Attempted to load extension twice!");
		return false;
	}

	if (InitCoreSynergy() == false)
	{
		ClearLoadedModules();
		rootconsole->ConsolePrint("--------- Failed to load Synergy extension ---------");
		return false;
	}

	AllowWriteToModules();

	server_dll = FindModule("synergy\\synergy\\bin\\server.dll");
	engine_dll = FindModule("synergy\\bin\\engine.dll");

	fields.CGlobalEntityList = FILEOFF(server_dll, 0x10A0EED4);
	fields.sv = FILEOFF(engine_dll, 0x105EA370);
	fields.RemoveImmediateSemaphore = FILEOFF(server_dll, 0x10A9C894);

	offsets.classname_offset = 0x6C;
	offsets.abs_origin_offset = 0x290;
	offsets.abs_angles_offset = 0x324;
	offsets.abs_velocity_offset = 0x228;
	offsets.origin_offset = 0x330;
	offsets.mnetwork_offset = 0x28;
	offsets.refhandle_offset = 0x348;
	offsets.iserver_offset = 0x1C;
	offsets.collision_property_offset = 0x15C;
	offsets.ismarked_offset = 0x114;

	//CHECK
	offsets.vphysics_object_offset = 0x208;
	//CHECK

	offsets.m_CollisionGroup_offset = 0x1F0;
	offsets.entity_release_offset = 0x10;
	offsets.updateonremove_offset = 0x1A8;

	functions.RemoveEntityNormal = (pTwoArgProt)(RemoveEntityNormalSynergy);
	functions.InstaKill = (pTwoArgProt)(InstaKillSynergy);
	functions.GetCBaseEntity = (pOneArgProt)(GetCBaseEntitySynergy);

	//CHECK
	functions.SetSolidFlags = (pTwoArgProt)(0);
	functions.DisableEntityCollisions = (pTwoArgProt)(0);
	functions.EnableEntityCollisions = (pTwoArgProt)(0);
	functions.CollisionRulesChanged = (pOneArgProt)(0);
	functions.FindEntityByClassname = (pThreeArgProt)(0);
	functions.CleanupDeleteList = (pOneArgProt)(0);
	//CHECK

	PopulateHookExclusionListsSynergy();

	ApplyPatchesSynergy();
	ApplyPatchesSpecificSynergy();

	HookFunctionsSynergy();
	HookFunctionsSpecificSynergy();

	RestoreModulesMemory();

	rootconsole->ConsolePrint("\n\nServer Map: [%s]\n\n", fields.sv + 0x11);
	rootconsole->ConsolePrint("----------------------  Synergy " SMEXT_CONF_NAME " " SMEXT_CONF_VERSION " loaded  ----------------------");
	loaded_extension = true;

	return true;
}

void ApplyPatchesSynergy()
{

}

void HookFunctionsSynergy()
{
	uint32_t server = (uint32_t)server_dll->module_base;
	uint32_t server_size = (uint32_t)server_dll->module_size;

	HookFunction(server, server_size, (void*)FILEOFF(server_dll, 0x1006B010), (void*)HooksSynergy::InstaKillHook);
	HookFunction(server, server_size, (void*)FILEOFF(server_dll, 0x100DD730), (void*)HooksSynergy::UTIL_RemoveBaseHook);
	HookFunction(server, server_size, (void*)FILEOFF(server_dll, 0x101E4DE0), (void*)HooksSynergy::UTIL_RemoveIserverHook);
}

uint32_t HooksSynergy::InstaKillHook(uint32_t arg0)
{
	functions.InstaKill(arg0, true);
	return 0;
}

uint32_t HooksSynergy::UTIL_RemoveIserverHook(uint32_t arg0)
{
	// THIS IS UTIL_Remove(IServerNetworable*)
	// THIS HOOK IS FOR UNUSUAL CALLS TO UTIL_Remove probably from sourcemod!

	if (arg0 == 0) return 0;
	functions.RemoveEntityNormal(arg0 - offsets.iserver_offset, true);
	return 0;
}

uint32_t HooksSynergy::UTIL_RemoveBaseHook(uint32_t arg0)
{
	pOneArgProt pDynamicOneArgFunc;

	if (arg0 == 0)
	{
		rootconsole->ConsolePrint("Could not kill entity [NULL]");
		return 0;
	}

	char* classname = (char*)(*(uint32_t*)(arg0 + offsets.classname_offset));
	uint32_t m_refHandle = *(uint32_t*)(arg0 + offsets.refhandle_offset);
	uint32_t chk_ref = functions.GetCBaseEntity(m_refHandle);

	if (chk_ref)
	{
		if (classname && strcmp(classname, "player") == 0)
		{
			rootconsole->ConsolePrint(EXT_PREFIX "Tried killing player but was protected!");
			return 0;
		}

		uint32_t isMarked = IsMarkedForDeletion(chk_ref + offsets.iserver_offset);
		if (isMarked) return 0;

		disable_internal_remove_incrementor = true;

		//UTIL_Remove(CBaseEntity*)
		pDynamicOneArgFunc = (pOneArgProt)(FILEOFF(server_dll, 0x100DD730));
		pDynamicOneArgFunc(arg0);

		disable_internal_remove_incrementor = false;

		isMarked = IsMarkedForDeletion(arg0 + offsets.iserver_offset);

		if (isMarked)
		{
			if (*(uint32_t*)(fields.RemoveImmediateSemaphore) != 0)
			{
				hooked_delete_counter++;
			}
		}

		return 0;
	}

	rootconsole->ConsolePrint("INVALID ENT!");
	exit(1);
	return 0;
}