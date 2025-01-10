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
#include "ProcessEventHooks.hpp"
#include "ProcessInternalHooks.hpp"
#include "UnitTest.hpp"

void OnDLLProcessAttach()
{
	auto base_address = reinterpret_cast<unsigned int>(GetModuleHandle(0));

#ifndef PLOG_DISABLE_LOGGING
	//AllocConsole();
	//freopen("CONOUT$", "w", stdout);
	static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::init(plog::debug, &consoleAppender);
#endif
	PLOG_INFO << "Successfully Injected DLL.";

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)OriginalProcessEventFunction, ProcessEventHook);
	DetourAttach(&(PVOID&)OriginalProcessInternalFunction, ProcessInternalHook);
	auto error = DetourTransactionCommit();

	ProcessEventHooks = UFunctionHooks<ProcessEventPrototype>(OriginalProcessEventFunction);
	ProcessEventHooks.AddHook("Function Engine.HUD.PostRender", Engine_HUD_PostRender_Hook);
	ProcessEventHooks.AddHook("Function TribesGame.TrGameReplicationInfo.Tick",
							  TribesGame_TrGameReplicationInfo_Tick_Hook,
							  FunctionHookType::kPost);
	ProcessEventHooks.AddHook("Function TribesGame.TrProjectile.PostBeginPlay",
							  TribesGame_TrProjectile_PostBeginPlay_Hook, FunctionHookType::kPost);
	ProcessEventHooks.AddHook("Function TribesGame.TrHUD.PostRenderFor", TribesGame_TrHUD_PostRenderFor_Hook);
	

	ProcessInternalHooks = UFunctionHooks<ProcessInternalPrototype>(OriginalProcessInternalFunction);
	ProcessInternalHooks.AddHook("Function TribesGame.TrProjectile.Explode",
								 TribesGame_TrProjectile_Explode_Hook,
								 FunctionHookType::kPre);

	auto unitTestResult = PerformUnitTest();
}

BOOL APIENTRY DllMain(HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved)
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

