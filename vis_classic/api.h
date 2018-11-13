#ifndef NULLSOFT_API_H
#define NULLSOFT_API_H

#include <api/service/api_service.h>
extern api_service *serviceManager;
#define WASABI_API_SVC serviceManager

#include <api/service/waServiceFactory.h>

#include <Agave/Language/api_language.h>

extern api_language* WASABI_API_LNG;
extern HINSTANCE WASABI_API_LNG_HINST,
				 WASABI_API_ORIG_HINST;

#endif