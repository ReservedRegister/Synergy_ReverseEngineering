#include "extension.h"

extern bool InitExtensionBlackMesa();
extern bool InitExtensionSynergy();

extern void InitUtil();

SynergyUtils g_SynUtils;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_SynUtils);

void SynergyUtils::SDK_OnAllLoaded()
{
	InitUtil();

	InitExtensionBlackMesa();
	InitExtensionSynergy();
}