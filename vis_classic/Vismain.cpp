// Small WinAmp Window Replacements, loosely based from:
// Winamp Visulization library
// Copyright (C) 1997, Justin Frankel/Nullsoft
// Feel free to base any plugins on this "framework"...

#include <windows.h>
#include "..\winamp\vis\vis.h"
#include "vis_satan.h"

// "member" functions
winampVisModule *getModule(int which);

// Module header, includes version, description, and address of the module retriever function
winampVisHeader hdr = { VIS_HDRVER, CS_MODULE_TITLE, getModule };

#ifdef __cplusplus
extern "C" {
#endif
// this is the only exported symbol. returns our main header.
__declspec( dllexport ) winampVisHeader *winampVisGetHeader()
{
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
