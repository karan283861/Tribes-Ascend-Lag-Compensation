// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>

#include "Detours/include/detours.h"

#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#ifndef _DEBUG
#define PLOG_DISABLE_LOGGING
#endif

#include "Hook.hpp"

void OnDLLProcessAttach()
{
	auto base_address = reinterpret_cast<unsigned int>(GetModuleHandle(0));

#ifdef _DEBUG
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::init(plog::debug, &consoleAppender);
#endif
	PLOG_INFO << "Successfully Injected DLL.";

	// Locate the ProcessEvent function
	UObject* Object = UObject::GObjObjects()->Data[1];
	unsigned long* cObject = (unsigned long*)Object;
	unsigned long* ProcessEventsAddress = (unsigned long*)(*(cObject)+65 * sizeof(unsigned long));
	OriginalProcessEventFunction = reinterpret_cast<ProcessEventPrototype>(*ProcessEventsAddress);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)OriginalProcessEventFunction, ProcessEventHook);
	auto error = DetourTransactionCommit();

	ProcessEventHooks = UFunctionHooks<ProcessEventPrototype>(OriginalProcessEventFunction);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OnDLLProcessAttach, NULL, NULL, NULL);
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

