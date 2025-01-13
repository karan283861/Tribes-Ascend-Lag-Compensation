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
#include "NativeHooks.hpp"
#include "Helper.hpp"
#include "UnitTest.hpp"

void OnDLLProcessAttach()
{
	auto baseAddress = reinterpret_cast<unsigned int>(GetModuleHandle(0));

#ifndef PLOG_DISABLE_LOGGING
	//AllocConsole();
	//freopen("CONOUT$", "w", stdout);
	static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::init(plog::verbose, &consoleAppender);
#endif
	PLOG_INFO << "Successfully Injected DLL.";
	PLOG_INFO << std::format("Base address: {0}", reinterpret_cast<void*>(baseAddress));

	auto unitTestResult = PerformUnitTest();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)OriginalProcessEventFunction, ProcessEventHook);
	DetourAttach(&(PVOID&)OriginalProcessInternalFunction, ProcessInternalHook);
	//DetourAttach(&(PVOID&)OriginalCallFunctionFunction, CallFunctionHook);

	//auto allUFunctions = GetInstancesUObjects<UFunction>();
	//for (auto& ufunction : allUFunctions)
	//{
	//	//if (ufunction->iNative)
	//	//{
	//	//	PLOG_DEBUG << std::format("Hooking native: [{0}] {1} ({2})", ufunction->iNative, ufunction->GetFullName(),
	//	//							  static_cast<void*>(ufunction->Func));
	//	//	GNativeFunctionPrototype gNativeFunction = reinterpret_cast<GNativeFunctionPrototype>(ufunction->Func);
	//	//	GNativeFunctionHookInformation gNativeFunctionHookInformation;
	//	//	constexpr int i = __COUNTER__;
	//	//	gNativeFunctionHookInformation.m_Index = i;
	//	//	gNativeFunctionHookInformation.m_OriginalFunction = gNativeFunction;
	//	//	DetourAttach(&(PVOID&)gNativeFunction, GNativeFunctionHook<i>);
	//	//	//OriginalGNativeFunctionFromiNative[ufunction->iNative] = gNativeFunction;
	//	//}

	//	if (ufunction->iNative)
	//	{
	//		PLOG_DEBUG << std::format("Hooking native: [{0}] {1} ({2})", ufunction->iNative, ufunction->GetFullName(),
	//								  static_cast<void*>(ufunction->Func));
	//		GNativeFunctionPrototype gNativeFunction = reinterpret_cast<GNativeFunctionPrototype>(ufunction->Func);
	//		//DetourAttach(&(PVOID&)gNativeFunction, GNativeFunctionHook);
	//	}
	//}

	auto allUFunctions = GetInstancesUObjects<UFunction>();
	for (auto& ufunction : allUFunctions)
	{
		if (ufunction->iNative)
		{
			PLOG_INFO << std::format("{0}->Func Offset = {1}", ufunction->GetFullName(),
									 reinterpret_cast<void*>(reinterpret_cast<unsigned int>(ufunction->Func) - baseAddress));
		}
	}

	DetourAttach(&(PVOID&)OriginalAActorMove, AActor_Move_Hook);
	DetourAttach(&(PVOID&)OriginalAActorSetLocation, AActor_SetLocation_Hook);
	DetourAttach(&(PVOID&)OriginalUWorldMoveActor, UWorld_MoveActor_Hook);
	DetourAttach(&(PVOID&)OriginalUGameEngineTick, UGameEngine_Tick_Hook);
	DetourAttach(&(PVOID&)OriginalUWorldTick, UWorld_Tick_Hook);

	auto error = DetourTransactionCommit();

	ProcessEventHooks = UFunctionHooks<ProcessEventPrototype>(OriginalProcessEventFunction);
	ProcessEventHooks.AddHook("Function Engine.HUD.PostRender", Engine_HUD_PostRender_Hook);
	ProcessEventHooks.AddHook("Function TribesGame.TrGameReplicationInfo.Tick",
							  TribesGame_TrGameReplicationInfo_Tick_Hook,
							  FunctionHookType::kPost);
	ProcessEventHooks.AddHook("Function TribesGame.TrHUD.PostRenderFor", TribesGame_TrHUD_PostRenderFor_Hook);
	/*ProcessEventHooks.AddHook("Function TribesGame.TrProjectile.PostBeginPlay", TribesGame_TrProjectile_PostBeginPlay);*/
	

	ProcessInternalHooks = UFunctionHooks<ProcessInternalPrototype>(OriginalProcessInternalFunction);
	ProcessInternalHooks.AddHook("Function TribesGame.TrProjectile.HurtRadius_Internal", TrProjectile_HurtRadius_Internal_Hook,
								 FunctionHookType::kPre, FunctionHookAbsorb::kAbsorb);
	/*ProcessInternalHooks.AddHook("Function TribesGame.TrProjectile.InitProjectile",
							  TribesGame_TrProjectile_InitProjectile_Hook, FunctionHookType::kPost);*/
	/*ProcessInternalHooks.AddHook("Function TribesGame.TrProjectile.Explode",
								 TribesGame_TrProjectile_Explode_Hook,
								 FunctionHookType::kPre);*/

	CallFunctionHooks = UFunctionHooks<CallFunctionPrototype>(OriginalCallFunctionFunction);

	//auto ufunction_Engine_Actor_Tick{ UObject::FindObject<UFunction>("Function Engine.Actor.Tick") };
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

