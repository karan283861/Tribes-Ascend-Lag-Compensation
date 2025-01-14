// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>

#include "Detours/include/detours.h"

#include <format>
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#ifndef _DEBUG
//#define PLOG_DISABLE_LOGGING
#endif

#include "Hook.hpp"
#include "ProcessEventHooks.hpp"
#include "ProcessInternalHooks.hpp"
#include "NativeHooks.hpp"
#include "Helper.hpp"

void OnDLLProcessAttach()
{
    auto baseAddress{ reinterpret_cast<unsigned int>(GetModuleHandle(0)) };

#ifdef _DEBUG
    //AllocConsole();
    //freopen("CONOUT$", "w", stdout);
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);
#else
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("LagCompensationLog.txt");
    plog::init(plog::warning, &fileAppender);
#endif
    PLOG_INFO << "Successfully Injected DLL.";
    PLOG_INFO << std::format("Base address: {0}", reinterpret_cast<void*>(baseAddress));

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

#ifdef _DEBUG
    DetourAttach(&(PVOID&)OriginalProcessEventFunction, ProcessEventHook);
#endif

    DetourAttach(&(PVOID&)OriginalProcessInternalFunction, ProcessInternalHook);

#ifdef _DEBUG
    DetourAttach(&(PVOID&)OriginalCallFunctionFunction, CallFunctionHook);
#endif

    // Hook native functions

    // This is the engine tick function. Hooking here to get the start of each tick
    DetourAttach(&(PVOID&)OriginalUGameEngineTick, UGameEngine_Tick_Hook);
    // The MoveActor function is called within engine tick to move all actors
    // Hook into this to get the latest location of actors and also preform rewinding
    // of projectiles
    DetourAttach(&(PVOID&)OriginalUWorldMoveActor, UWorld_MoveActor_Hook);

    auto error{ DetourTransactionCommit() };

    ProcessEventHooks = UFunctionHooks<ProcessEventPrototype>(OriginalProcessEventFunction);
    ProcessInternalHooks = UFunctionHooks<ProcessInternalPrototype>(OriginalProcessInternalFunction);
    CallFunctionHooks = UFunctionHooks<CallFunctionPrototype>(OriginalCallFunctionFunction);

    // UFunction hook for when a projectile explodes to cause radial (splash) damage
    ProcessInternalHooks.AddHook("Function TribesGame.TrProjectile.HurtRadius_Internal", TrProjectile_HurtRadius_Internal_Hook,
                                 FunctionHookType::kPre, FunctionHookAbsorb::kAbsorb);
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

