// Small WinAmp Window Replacements, loosely based from:
// Winamp Visulization library
// Copyright (C) 1997, Justin Frankel/Nullsoft
// Feel free to base any plugins on this "framework"...

#include <windows.h>
#include <Winamp/vis.h>
#include "vis_satan.h"
#include "api.h"
#include <nu/ServiceBuilder.h>
#include <loader/hook/get_api_service.h>

// TODO add to lang.h
// {0D1DA5F5-FD41-4934-B75A-B8455EE19F82}
static const GUID VisCSAGUID = 
{ 0xd1da5f5, 0xfd41, 0x4934, { 0xb7, 0x5a, 0xb8, 0x45, 0x5e, 0xe1, 0x9f, 0x82 } };

// use this to get our own HINSTANCE since overriding DllMain(..) causes instant crashes (should see why)
static HINSTANCE GetMyInstance()
{
	MEMORY_BASIC_INFORMATION mbi = {0};
	if(VirtualQuery(GetMyInstance, &mbi, sizeof(mbi)))
		return (HINSTANCE)mbi.AllocationBase;
	return NULL;
}

// "member" functions
winampVisModule *getModule(int which);

// Module header, includes version, description, and address of the module retriever function
winampVisHeader hdr = { VIS_HDRVER, CS_MODULE_TITLE, getModule };

#ifdef __cplusplus
extern "C" {
#endif
// this is the only exported symbol. returns our main header.
__declspec( dllexport ) winampVisHeader *winampVisGetHeader(HWND hwndParent)
{
	if(!WASABI_API_LNG_HINST)
	{
		WASABI_API_SVC = GetServiceAPIPtr();/*/
		// loader so that we can get the localisation service api for use
		WASABI_API_SVC = (api_service*)SendMessage(hwndParent, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
		if (WASABI_API_SVC == (api_service*)1) WASABI_API_SVC = NULL;/**/
		if (WASABI_API_SVC != NULL)
		{
			ServiceBuild(WASABI_API_SVC, WASABI_API_LNG, languageApiGUID);

			// need to have this initialised before we try to do anything with localisation features
			WASABI_API_START_LANG(GetMyInstance(), VisCSAGUID);

			// as we're under a different thread we need to set the locale
			//WASABI_API_LNG->UseUserNumericLocale();
			//g_use_C_locale = WASABI_API_LNG->Get_C_NumericLocale();
		}
	}
	return &hdr;
}
#ifdef __cplusplus
}
#endif

// getmodule routine from the main header. Returns NULL if an invalid module was requested,
// otherwise returns either mod1 or mod2 depending on 'which'.
winampVisModule *getModule(int which)
{
	switch(which)
	{
    case 0: return &AtAnSt_Vis_mod;
	default: return NULL;
	}
}
